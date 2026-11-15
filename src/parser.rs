use crate::{error, fatal};

#[derive(PartialEq, Clone)]
pub enum Target {
    Error,
    Object(String),
    Property(Box<Target>, String),
    Index(Box<Target>, Expression)
}

#[derive(PartialEq, Clone, Debug)]
pub enum Expression {
    Error,
    Atom(crate::lexer::Token),
    Binary(Box<Expression>, crate::lexer::Token, Box<Expression>),
    Unary(crate::lexer::Token, Box<Expression>),
    Array(Vec<Expression>),
    Call(Box<Expression>, Vec<Expression>),
    Index(Box<Expression>, Box<Expression>),
}

#[derive(PartialEq, Clone)]
pub enum Statement {
    None,
    End,
    Error,
    Block(Vec<Statement>),
    VariableDeclaration(Target, Expression),
    ConstantDeclaration(Target, Expression),
    Assignment(Target, Expression),
    Expression(Expression),
}

pub type Program = Vec<Statement>;

#[derive(Clone, Copy)]
struct BindingPower {
    l: u8,
    r: u8,
}
impl BindingPower {
    const fn new(l: u8, r: u8) -> Self {
        BindingPower { l: l, r: r }
    }
}
impl Default for BindingPower {
    fn default() -> Self {
        Self::new(0, 0)
    }
}

fn get_binding_power(token: &crate::lexer::Token) -> Option<BindingPower> {
    use crate::lexer::TokenType::*;

    let bp = match token.t {
        Or => BindingPower::new(3, 4),
        Xor => BindingPower::new(5, 6),
        And => BindingPower::new(7, 8),

        EqualTo | NotEqualTo => BindingPower::new(10, 9),

        GreaterThan | LessThan | GreaterThanEqual | LessThanEqual => BindingPower::new(12, 11),

        Plus | Minus => BindingPower::new(14, 15),
        Multiply | Divide | Modulous => BindingPower::new(16, 17),

        Exponent => BindingPower::new(18, 17),

        Not => BindingPower::new(0, 19),
        _ => return None,
    };

    Some(bp)
}

struct Parser {
    tokens: crate::lexer::LexerResult,
    result: Program,
    source_id: usize,
    depth: u16,
    last_token: crate::lexer::Token,
}

impl Parser {
    fn new(tokens: crate::lexer::LexerResult, id: usize) -> Self {
        let mut tokens = tokens;
        tokens.reverse();
        return Parser {
            tokens,
            result: Program::new(),
            source_id: id,
            depth: 0u16,
            last_token: crate::lexer::Token {
                t: crate::lexer::TokenType::Unknown,
                v: String::new(),
                s: 0,
                e: 0,
            },
        };
    }

    fn finish(self) -> Program {
        self.result
    }

    fn is_skipable(&self, t: &crate::lexer::Token) -> bool {
        use crate::lexer::TokenType::*;
        match t.t {
            Semicolon | Eof => true,
            Unknown => fatal!("Unknown Token", self.source_id, t.s, t.e),
            _ => false,
        }
    }

    fn get(&mut self) -> Option<crate::lexer::Token> {
        let t = self.tokens.pop();
        if t.is_some() {
            if t.as_ref().unwrap().t == crate::lexer::TokenType::Eof {
                return None;
            };
            self.last_token = t.clone().unwrap();
        };
        t
    }

    fn peek(&mut self) -> Option<crate::lexer::Token> {
        self.tokens.last().cloned()
    }

    fn parse_target(&mut self) -> Target {
        use crate::lexer::TokenType as tt; 
        let token = self.get();
        if token.is_none() {
            error!("Could Not Find Target", self.source_id, self.last_token.s, self.last_token.e);
            return Target::Error;
        }
        let token = token.unwrap();

        if token.t != tt::Identifier {
            fatal!(
                "Expected identifier",
                self.source_id,
                token.s,
                token.e
            );
        }

        let mut target = Target::Object(token.v);

        while let Some(next) = self.peek() {
            if next.t == tt::FullStop {
                let dot = self.get().unwrap();

                let property = self.get();
                if property.is_none() {
                    error!("Could Not Find Property", self.source_id, dot.s, dot.e);
                    return Target::Error;
                }
                let property = property.unwrap();

                if property.t != tt::Identifier {
                    fatal!(
                        "Expected property name",
                        self.source_id,
                        property.s,
                        property.e
                    );
                }

                target = Target::Property(
                    Box::new(target),
                    property.v,
                );
            } else if next.t == tt::OpenSquareBracket {
                self.get();

                let index = self.parse_expression(BindingPower::default());

                let Some(close) = self.get() else {
                    error!("Unclosed Square Bracket", self.source_id, next.s, next.e);
                    return Target::Error;
                };

                if close.t != tt::CloseSquareBracket {
                    fatal!(
                        "Expected closing square bracket",
                        self.source_id,
                        close.s,
                        close.e
                    );
                }

                target = Target::Index(Box::new(target), index);
            } else {
                break;
            }
        }

        target
    }

    fn parse_no_sides(&mut self, item: crate::lexer::Token) -> Expression {
        use crate::lexer::TokenType::*;
        match item.t {
            Identifier | Integer | Float | Character | String => {
                let Some(p) = self.peek() else {
                    return Expression::Atom(item);
                };


                loop {
                    match p.t {
                        
                        _ => return Expression::Atom(item),
                    }  
                }
                
            },
            OpenBracket => {
                let expr = self.parse_expression(BindingPower::default());

                match self.get() {
                    Some(token) if token.t == CloseBracket => expr,

                    Some(token) => {
                        error!(
                            "Expected Closing Brackets",
                            self.source_id, token.s, token.e
                        );
                        return Expression::Error;
                    }

                    None => {
                        error!("Unclosed Brackets", self.source_id, item.s, item.e);
                        return Expression::Error;
                    }
                }
            }
            OpenSquareBracket => {
                let mut array = Vec::new();

                loop {
                    let next = self.peek();
                    if next.is_none() {
                        fatal!("Unclosed Square Brackets", self.source_id, item.s, item.e);
                    }
                    let next = next.unwrap();
                    if next.t == CloseSquareBracket {
                        break;
                    } else if next.t == Comma {
                        self.get();
                    }

                    array.push(self.parse_expression(BindingPower::default()));
                }
                self.get();
                Expression::Array(array)
            }
            Minus => Expression::Unary(
                item,
                Box::new(self.parse_expression(BindingPower::new(0, 20))),
            ),
            Not => Expression::Unary(
                item,
                Box::new(self.parse_expression(BindingPower::new(0, 19))),
            ),
            _ => {
                error!("Token Unimplemented", self.source_id, item.s, item.e);
                return Expression::Error;
            }
        }
    }

    fn parse_left_side(&mut self, lhs: Expression, op: crate::lexer::Token) -> Expression {
        match get_binding_power(&op) {
            Some(c) => {
                let rhs = self.parse_expression(c);

                Expression::Binary(Box::new(lhs), op, Box::new(rhs))
            }
            None => {
                error!("Unexpexted Token", self.source_id, op.s, op.e);
                Expression::Error
            }
        }
    }

    fn parse_postfix(&mut self, mut lhs: Expression) -> Expression {
        use crate::lexer::TokenType::*;

        loop {
            let Some(next) = self.peek() else {
                break;
            };

            match next.t {
                OpenBracket => {
                    self.get();

                    let mut args = Vec::new();

                    if self.peek().is_some_and(|x| x.t != CloseBracket) {
                        loop {
                            args.push(self.parse_expression(BindingPower::default()));

                            match self.peek() {
                                Some(t) if t.t == Comma => {
                                    self.get();
                                }
                                _ => break,
                            }
                        }
                    }

                    match self.get() {
                        Some(t) if t.t == CloseBracket => {}
                        Some(t) => {
                            error!(
                                "Expected Closing Bracket",
                                self.source_id,
                                t.s,
                                t.e
                            );
                            return Expression::Error;
                        }
                        None => {
                            error!(
                                "Unclosed Function Call",
                                self.source_id,
                                next.s,
                                next.e
                            );
                            return Expression::Error;
                        }
                    }

                    lhs = Expression::Call(Box::new(lhs), args);
                }

                OpenSquareBracket => {
                    self.get();
                    let index = self.parse_expression(BindingPower::default());

                    let Some(close) = self.get() else {
                        error!("Unclosed Square Bracket", self.source_id, next.s, next.e);
                        return Expression::Error;
                    };

                    if close.t != CloseSquareBracket {
                        fatal!(
                            "Expected closing square bracket",
                            self.source_id,
                            close.s,
                            close.e
                        );
                    }

                    lhs = Expression::Index(Box::new(lhs), Box::new(index))
                }

                _ => break,
            }
        }

        lhs
    }

    fn parse_expression(&mut self, bp: BindingPower) -> Expression {
        let lhs = self.get();
        if lhs.is_none() {
            error!(
                format!("Expected Token After This \"{}\"", self.last_token.v).as_str(),
                self.source_id, self.last_token.s, self.last_token.e
            );
            return Expression::Error;
        }
        let mut lhs = self.parse_no_sides(lhs.unwrap());
        lhs = self.parse_postfix(lhs);

        loop {
            let op = match self.peek() {
                Some(op) => op,
                None => break,
            };

            let op_bp = match get_binding_power(&op) {
                Some(bp) => bp,
                None => break,
            };

            if op_bp.l <= bp.r {
                break;
            }

            self.get();
            lhs = self.parse_left_side(lhs, op);
            lhs = self.parse_postfix(lhs);
        }

        lhs
    }

    fn parse_statement(&mut self) -> Statement {
        use crate::lexer::TokenType::*;

        let ot = self.get();
        if ot.is_none() {
            if self.depth > 0 {
                fatal!(
                    "Unclosed Block",
                    self.source_id,
                    self.last_token.s,
                    self.last_token.e
                )
            }
            return Statement::End;
        };
        let t = ot.unwrap();

        if self.is_skipable(&t) {
            return Statement::None;
        };

        match t.v.as_str() {
            "new" => {
                let c = self.peek();
                if c.is_none() {
                    fatal!(
                        "Keyword Followed By EOF",
                        self.source_id,
                        t.s,
                        t.e,
                        "Consider Removing This Keyword"
                    )
                }
                let c = c.unwrap();

                if c.v == "const" {
                    self.get();
                    let name = self.parse_target();

                    let should_be_equals = self.get();
                    if should_be_equals.is_none() {
                        fatal!(
                            "Constant Declaration Followed By EOF",
                            self.source_id,
                            c.s,
                            self.last_token.e
                        )
                    }
                    let should_be_equals = should_be_equals.unwrap();

                    if should_be_equals.t != Equals {
                        error!(
                            "Constant Not Defined",
                            self.source_id, c.e+1, should_be_equals.e
                        );
                        return Statement::Error;
                    }
                    return Statement::ConstantDeclaration(
                        name,
                        self.parse_expression(BindingPower::default()),
                    );
                } else {
                    let name = self.parse_target();
                    let should_be_equals = self.get();
                    if should_be_equals.is_none() {
                        fatal!(
                            "Variable Declaration Followed By EOF",
                            self.source_id,
                            c.s,
                            self.last_token.e
                        )
                    }
                    let should_be_equals = should_be_equals.unwrap();

                    if should_be_equals.t != Equals {
                        error!(
                            "Variable Not Defined",
                            self.source_id, c.s, should_be_equals.e
                        );
                        return Statement::Error;
                    }
                    return Statement::VariableDeclaration(
                        name,
                        self.parse_expression(BindingPower::default()),
                    );
                }
            }

            "{" => {
                let (new_depth, err) = self.depth.overflowing_add(1);
                if err {
                    fatal!("Exceeded Maximum Depth", self.source_id, t.s, t.e)
                }
                self.depth = new_depth;
                return Statement::Block(self.parse_block());
            }

            "}" => {
                let (new_depth, err) = self.depth.overflowing_sub(1);
                if err {
                    fatal!(
                        "Does Not Close Any Block",
                        self.source_id,
                        t.s,
                        t.e,
                        "Consider Removing This \"}\""
                    )
                }
                self.depth = new_depth;
                return Statement::End;
            }

            _ => {
                let p = self.peek();
                if p.is_some() {
                    let p = p.unwrap();
                    match p.t {
                        Equals => {
                            let target = self.parse_target();
                            self.get();
                            return Statement::Assignment(
                                target,
                                self.parse_expression(BindingPower::default()),
                            );
                        }
                        _ => {
                            return Statement::Expression(
                                self.parse_expression(BindingPower::default()),
                            );
                        }
                    };
                }
            }
        };

        error!("Unknown Statement", self.source_id, t.s, t.e);
        Statement::Error
    }

    fn parse_block(&mut self) -> Vec<Statement> {
        let mut result = Vec::new();
        loop {
            let stmt = self.parse_statement();

            if stmt == Statement::End {
                break;
            } else if stmt == Statement::None {
                continue;
            } else if stmt == Statement::Error {
                // fast forward
                while self
                    .get()
                    .is_some_and(|x| x.t != crate::lexer::TokenType::Semicolon)
                {}
            }

            let p = self.peek();
            if p.as_ref().is_some_and(|x| x.t != crate::lexer::TokenType::Semicolon) {
                let p = p.unwrap();
                error!("Expected Semicolon", self.source_id, p.s, p.e);
            }

            result.push(stmt);
        }
        result
    }
}

pub fn parse(tokens: crate::lexer::LexerResult, id: usize) -> Program {
    let mut parser = Parser::new(tokens, id);
    parser.result = parser.parse_block();
    parser.finish()
}
