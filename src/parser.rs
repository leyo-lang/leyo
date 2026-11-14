use crate::{error, fatal};

#[derive(PartialEq, Clone)]
pub enum Expression {
    Error,
    Atom(crate::lexer::Token),
    Binary(Box<Expression>, crate::lexer::Token, Box<Expression>),
}

#[derive(PartialEq, Clone)]
pub enum Statement {
    None,
    End,
    Error,
    Block(Vec<Statement>),
    VariableDeclaration(String, Expression),
    ConstantDeclaration(String, Expression),
    Assignment(String, Expression),
}

pub type Program = Vec<Statement>;

#[derive(Clone, Copy)]
struct BindingPower {
    l: u8,
    r: u8,
} impl BindingPower {
    const fn new(l: u8, r: u8) -> Self {
        BindingPower { l: l, r: r }
    }
} impl Default for BindingPower {
    fn default() -> Self {
        Self::new(0, 0)
    }
}

fn get_binding_power(token: &crate::lexer::Token) -> Option<BindingPower> {
    use crate::lexer::TokenType::*;

    let bp = match token.t {
        Equals
        | PlusEquals
        | MinusEquals
        | MultiplyEquals
        | DivideEquals
        | ExponentEquals
        | ModulousEquals => BindingPower::new(1, 2),

        Or => BindingPower::new(3, 4),
        Xor => BindingPower::new(5, 6),
        And => BindingPower::new(7, 8),

        EqualTo | NotEqualTo => BindingPower::new(10, 9),

        GreaterThan | LessThan
        | GreaterThanEqual | LessThanEqual => BindingPower::new(12, 11),

        Plus | Minus => BindingPower::new(14, 13),
        Multiply | Divide | Modulous => BindingPower::new(16, 15),

        Exponent => BindingPower::new(17, 18),

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
        return Parser { tokens, result: Program::new(), source_id: id, depth: 0u16, last_token: crate::lexer::Token {t:crate::lexer::TokenType::Unknown, v:String::new(), s:0, e:0} }
    }

    fn finish(self) -> Program {
        self.result
    }

    fn is_skipable(&self, t: &crate::lexer::Token) -> bool {
        use crate::lexer::TokenType::*;
        match t.t {
            Semicolon | Eof => true,
            Unknown => fatal!("Unknown Token", self.source_id, t.s, t.e),
            _ => false
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

    fn parse_no_sides(&self, item: crate::lexer::Token) -> Expression {
        use crate::lexer::TokenType::*;
        match item.t {
            Identifier | Integer | Float | Character | String => Expression::Atom(item),
            _ => fatal!("Token Unimplemented", self.source_id, item.s, item.e),
        }
    }

    fn parse_left_side(&mut self, lhs: Expression, op: crate::lexer::Token) -> Expression {
        match get_binding_power(&op) {
            Some(c) => {
                let rhs = self.parse_expression(c);

                Expression::Binary(Box::new(lhs), op, Box::new(rhs))
            },
            None => {error!("Unexpexted Token", self.source_id, op.s, op.e); Expression::Error},
        }
    }

    fn parse_expression(&mut self, bp: BindingPower) -> Expression {
        let lhs = self.get();
        if lhs.is_none() {
            error!(format!("Expected Token After This \"{}\"", self.last_token.v).as_str(), self.source_id, self.last_token.s, self.last_token.e);
        }
        let mut lhs = self.parse_no_sides(lhs.unwrap());


        loop {
            let op = self.get();
            if op.is_none() { break; };
            let op = op.unwrap();
            let op_bp = get_binding_power(&op);
            if op_bp.is_none_or(|o| o.l <= bp.r) {break};
            
            lhs = self.parse_left_side(lhs, op);
        }

        lhs
    }

    fn parse_statement(&mut self) -> Statement {
        use crate::lexer::TokenType::*;
        let ot = self.get();
        if ot.is_none() {
            if self.depth > 0 {
                fatal!("Unclosed Block", self.source_id, self.last_token.s, self.last_token.e)
            }
            return Statement::End 
        };
        let t = ot.unwrap();
        if self.is_skipable(&t) { return Statement::None };

        match t.v.as_str() {
            "new" => {
                let c = self.get();
                if c.is_none() {fatal!("Keyword Followed By EOF", self.source_id, t.s, t.e, "Consider Removing This Keyword")}
                let c = c.unwrap();

                let name = self.get();
                if name.is_none() {fatal!("Keyword Followed By EOF", self.source_id, c.s, c.e, "Consider Removing This Keyword")}
                let name = name.unwrap();
                let should_be_equals = self.get();
                if should_be_equals.is_none() {fatal!("Constant Declaration Followed By EOF", self.source_id, name.s, name.e)}
                let should_be_equals = should_be_equals.unwrap();

                if c.v == "const" {
                    if should_be_equals.t != Equals {
                        error!("Constant Not Defined", self.source_id, name.s, should_be_equals.e);
                        return Statement::Error;
                    }
                    return Statement::ConstantDeclaration(name.v, self.parse_expression(BindingPower::default()))
                } else {
                    if should_be_equals.t != Equals {
                        error!("Variable Not Defined", self.source_id, name.s, should_be_equals.e);
                        return Statement::Error;
                    }
                    return Statement::VariableDeclaration(name.v, self.parse_expression(BindingPower::default()))
                }
            },

            "{" => {
                let (new_depth, err) = self.depth.overflowing_add(1);
                if err { fatal!("Exceeded Maximum Depth", self.source_id, t.s, t.e) }
                self.depth = new_depth;
                return Statement::Block(self.parse_block());
            }

            "}" => {
                let (new_depth, err) = self.depth.overflowing_sub(1);
                if err { fatal!("Does Not Close Any Block", self.source_id, t.s, t.e, "Consider Removing This \"}\"") }
                self.depth = new_depth;
                return Statement::End;
            }

            _ => {
                let p = self.peek();
                if p.is_some() {
                    let p = p.unwrap();
                    match p.t {
                        Equals => {self.get(); return Statement::Assignment(t.v, self.parse_expression(BindingPower::default()))},
                        _ => {},
                    };
                }
            }
        };

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
                while self.get().is_some_and(|x| x.t != crate::lexer::TokenType::Semicolon) {};
            }

            result.push(stmt);
        }
        result
    }
}

pub fn parse(tokens: crate::lexer::LexerResult, id: usize) -> Program {
    let mut parser = Parser::new(tokens, id);
    parser.parse_block();
    parser.finish()
}