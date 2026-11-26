use crate::{error, fatal};
use ExpressionKind as Ek;
use StatementKind as Sk;

#[derive(PartialEq, Clone, Debug)]
pub enum ExpressionKind {
    Error,
    Atom(crate::lexer::Token),
    Binary(Box<Expression>, crate::lexer::Token, Box<Expression>),
    Unary(crate::lexer::Token, Box<Expression>),
    Array(Vec<Expression>),
    Call(Box<Expression>, Vec<Expression>),
    Index(Box<Expression>, Box<Expression>),
    Property(Box<Expression>, Box<Expression>),
    NamespaceProperty(Box<Expression>, Box<Expression>),
}

#[derive(PartialEq, Clone, Debug)]
pub struct Expression {
    pub k: ExpressionKind,
    pub s: usize,
    pub e: usize,
}

impl Expression {
    fn new(k: ExpressionKind, s: usize, e: usize) -> Self {
        Expression { k, s, e }
    }

    const fn err() -> Self {
        Expression {
            k: Ek::Error,
            s: 0,
            e: 0,
        }
    }
}

#[derive(PartialEq, Clone)]
pub enum StatementKind {
    None,
    End,
    Error,
    Block(Option<String>, Vec<Statement>),
    VariableDeclaration(Expression, Expression),
    ConstantDeclaration(Expression, Expression),
    Assignment(Expression, Expression),
    Evaluation(Expression),
    Loop(Expression, Box<Statement>),
    If(Expression, Box<Statement>, Option<Box<Statement>>),
    Repeat(Expression, Option<Expression>, Box<Statement>),
    Break,
    Continue,
    Function(String, Vec<Expression>, Box<Statement>),
    Return(Option<Expression>),
}

#[derive(PartialEq, Clone)]
pub struct Statement {
    pub k: StatementKind,
    pub s: usize,
    pub e: usize,
}

impl Statement {
    fn new(k: StatementKind, s: usize, e: usize) -> Self {
        Statement { k, s, e }
    }

    const fn err() -> Self {
        Statement {
            k: Sk::Error,
            s: 0,
            e: 0,
        }
    }
}

pub type Program = Vec<Statement>;

#[derive(Clone, Copy)]
struct BindingPower {
    l: u8,
    r: u8,
}
impl BindingPower {
    const fn new(l: u8, r: u8) -> Self {
        BindingPower { l, r }
    }
}
impl Default for BindingPower {
    fn default() -> Self {
        Self::new(0, 0)
    }
}

fn is_comparison(token: &crate::lexer::Token) -> bool {
    use crate::lexer::TokenType::*;

    matches!(
        token.t,
        EqualTo | NotEqualTo | GreaterThan | LessThan | GreaterThanEqual | LessThanEqual
    )
}

fn get_binding_power(token: &crate::lexer::Token) -> Option<BindingPower> {
    use crate::lexer::TokenType::*;

    let bp = match token.t {
        Or => BindingPower::new(3, 4),
        Xor => BindingPower::new(5, 6),
        And => BindingPower::new(7, 8),

        EqualTo | NotEqualTo | GreaterThan | LessThan | GreaterThanEqual | LessThanEqual => {
            BindingPower::new(10, 10)
        }

        Plus | Minus => BindingPower::new(14, 15),
        Multiply | Divide | Modulous => BindingPower::new(16, 17),

        Exponent => BindingPower::new(19, 17),

        Not => BindingPower::new(0, 20),
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
        Parser {
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
        }
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
        if let Some(t) = &t {
            if t.t == crate::lexer::TokenType::Eof {
                return None;
            };
            self.last_token = t.clone();
        };
        t
    }

    fn peek(&mut self) -> Option<crate::lexer::Token> {
        self.tokens.last().cloned()
    }

    fn reverse(&mut self, t: crate::lexer::Token) {
        self.tokens.push(t);
    }

    fn has_equals_before_semicolon(&self) -> bool {
        use crate::lexer::TokenType::*;

        for token in self.tokens.iter().rev() {
            match token.t {
                Equals => return true,
                Semicolon | OpenBrace | CloseBrace | Eof => return false,
                _ => {}
            }
        }
        false
    }

    fn parse_no_sides(&mut self, item: crate::lexer::Token) -> Expression {
        use crate::lexer::TokenType::*;
        let start = item.s;
        let end = item.e;
        match item.t {
            Identifier | Integer | Float | Character | String => {
                Expression::new(Ek::Atom(item), start, end)
            }
            OpenBracket => {
                let mut expr = self.parse_expression(BindingPower::default());

                match self.get() {
                    Some(token) if token.t == CloseBracket => {
                        expr.s = start;
                        expr.e = token.e;
                        expr
                    }

                    Some(token) => {
                        error!(
                            "Expected Closing Brackets",
                            self.source_id, token.s, token.e
                        );
                        Expression::err()
                    }

                    None => {
                        error!("Unclosed Brackets", self.source_id, item.s, item.e);
                        Expression::err()
                    }
                }
            }
            OpenSquareBracket => {
                let mut array = Vec::new();

                if self.peek().is_some_and(|x| x.t == CloseSquareBracket) {
                    let csb = self.get().unwrap();
                    return Expression::new(Ek::Array(array), item.s, csb.e);
                }

                loop {
                    array.push(self.parse_expression(BindingPower::default()));

                    let next = self.peek();
                    if next.is_none() {
                        fatal!("Unclosed Square Brackets", self.source_id, item.s, item.e);
                    }
                    let next = next.unwrap();
                    if next.t == CloseSquareBracket {
                        break;
                    } else if next.t == Comma {
                        self.get();
                    } else {
                        error!(
                            "Expected Comma or a Closing Square Bracket",
                            self.source_id, next.s, next.e
                        );
                        return Expression::err();
                    }
                }
                let csb = self.get().unwrap();
                Expression::new(Ek::Array(array), item.s, csb.e)
            }
            Minus => Expression::new(
                Ek::Unary(
                    item,
                    Box::new(self.parse_expression(BindingPower::new(0, 18))),
                ),
                start,
                self.last_token.e,
            ),
            Not => Expression::new(
                Ek::Unary(
                    item,
                    Box::new(self.parse_expression(BindingPower::new(0, 20))),
                ),
                start,
                self.last_token.e,
            ),
            _ => {
                error!("Token Unimplemented", self.source_id, item.s, item.e);
                Expression::err()
            }
        }
    }

    fn parse_left_side(&mut self, lhs: Expression, op: crate::lexer::Token) -> Expression {
        match get_binding_power(&op) {
            Some(c) => {
                let rhs = self.parse_expression(c);
                let rhse = rhs.e;
                let lhss = lhs.s;

                Expression::new(Ek::Binary(Box::new(lhs), op, Box::new(rhs)), lhss, rhse)
            }
            None => {
                error!("Unexpexted Token", self.source_id, op.s, op.e);
                Expression::err()
            }
        }
    }

    fn parse_postfix(&mut self, mut lhs: Expression) -> Expression {
        use crate::lexer::TokenType::*;

        let lhss = lhs.s;

        while let Some(next) = self.peek() {
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

                    let end = match self.get() {
                        Some(t) if t.t == CloseBracket => t.e,
                        Some(t) => {
                            error!("Expected Closing Bracket", self.source_id, t.s, t.e);
                            return Expression::err();
                        }
                        None => {
                            error!("Unclosed Function Call", self.source_id, next.s, next.e);
                            return Expression::err();
                        }
                    };

                    lhs = Expression::new(Ek::Call(Box::new(lhs), args), lhss, end);
                }

                OpenSquareBracket => {
                    self.get();
                    let index = self.parse_expression(BindingPower::default());

                    let Some(close) = self.get() else {
                        error!("Unclosed Square Bracket", self.source_id, next.s, next.e);
                        return Expression::err();
                    };

                    if close.t != CloseSquareBracket {
                        fatal!(
                            "Expected closing square bracket",
                            self.source_id,
                            close.s,
                            close.e
                        );
                    }

                    lhs = Expression::new(Ek::Index(Box::new(lhs), Box::new(index)), lhss, close.e)
                }

                FullStop => {
                    self.get();
                    let post = self.get();
                    if post.is_none() {
                        fatal!(
                            "FullStop Followed By EOF",
                            self.source_id,
                            next.s,
                            next.e + 1
                        );
                    }

                    let post = post.unwrap();
                    if post.t != Identifier {
                        error!(
                            "Unexpected Token After FullStop",
                            self.source_id, post.s, post.e
                        );
                        return Expression::err();
                    }

                    let posts = post.s;
                    let poste = post.e;

                    lhs = Expression::new(
                        Ek::Property(
                            Box::new(lhs),
                            Box::new(Expression::new(Ek::Atom(post), posts, poste)),
                        ),
                        lhss,
                        poste,
                    )
                }

                DoubleColon => {
                    self.get();
                    let t = self.peek();
                    if t.is_none() {
                        fatal!(
                            "Namespace Access Followed By EOF",
                            self.source_id,
                            next.s,
                            next.e
                        )
                    }

                    let t = t.unwrap();
                    if t.t != Identifier {
                        error!(
                            "Unexpected Token After Namespace Access",
                            self.source_id, t.s, t.e
                        );
                        return Expression::err();
                    }
                    self.get();

                    let ts = t.s;
                    let te = t.e;

                    lhs = Expression::new(
                        Ek::NamespaceProperty(
                            Box::new(lhs),
                            Box::new(Expression::new(Ek::Atom(t), ts, te)),
                        ),
                        lhss,
                        te,
                    );
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
            return Expression::err();
        }
        let mut lhs = self.parse_no_sides(lhs.unwrap());
        lhs = self.parse_postfix(lhs);

        while let Some(op) = self.peek() {
            let op_bp = match get_binding_power(&op) {
                Some(bp) => bp,
                None => break,
            };

            if op_bp.l <= bp.r {
                break;
            }

            if is_comparison(&op)
                && matches!(
                    &lhs.k,
                    Ek::Binary(_, previous, _) if is_comparison(previous)
                )
            {
                error!(
                    "Comparison Operators Cannot Be Chained",
                    self.source_id, op.s, op.e
                );

                return Expression::err();
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
            return Statement::new(Sk::End, 0, 0);
        };
        let t = ot.unwrap();

        if self.is_skipable(&t) {
            return Statement::new(Sk::None, 0, 0);
        };

        match if t.t != String { t.v.as_str() } else { "_" } {
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
                    let name = self.parse_expression(BindingPower::default());

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
                            self.source_id,
                            c.e + 1,
                            should_be_equals.e
                        );
                        return Statement::err();
                    }
                    Statement::new(
                        Sk::ConstantDeclaration(
                            name,
                            self.parse_expression(BindingPower::default()),
                        ),
                        t.s,
                        self.last_token.e,
                    )
                } else {
                    let name = self.parse_expression(BindingPower::default());
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
                        return Statement::err();
                    }
                    Statement::new(
                        Sk::VariableDeclaration(
                            name,
                            self.parse_expression(BindingPower::default()),
                        ),
                        t.s,
                        self.last_token.e,
                    )
                }
            }

            "{" => {
                let (new_depth, err) = self.depth.overflowing_add(1);
                if err {
                    fatal!("Exceeded Maximum Depth", self.source_id, t.s, t.e)
                }
                self.depth = new_depth;
                Statement::new(Sk::Block(None, self.parse_block()), t.s, self.last_token.e)
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
                Statement::new(Sk::End, 0, 0)
            }

            "while" => {
                let expr = self.parse_expression(BindingPower::default());
                let opening = self.peek();
                if opening.is_none() {
                    fatal!(
                        "While Loop Unopened",
                        self.source_id,
                        self.last_token.e - 1,
                        self.last_token.e
                    )
                }
                let opening = opening.unwrap();
                if !matches!(opening.t, OpenBrace | Colon | Semicolon) {
                    error!(
                        "Expected While Loop Body",
                        self.source_id, opening.s, opening.e
                    );
                    return Statement::err();
                }
                if opening.t == Colon {
                    self.get();
                }
                let stmt = self.parse_statement();
                let stmte = stmt.e;

                Statement::new(Sk::Loop(expr, Box::new(stmt)), t.e, stmte)
            }

            "if" => {
                let expr = self.parse_expression(BindingPower::default());
                let opening = self.peek();
                if opening.is_none() {
                    fatal!(
                        "If Statement Unopened",
                        self.source_id,
                        self.last_token.e - 1,
                        self.last_token.e
                    )
                }
                let opening = opening.unwrap();

                if opening.t == Colon {
                    self.get();
                } else if opening.t != OpenBrace {
                    error!(
                        "Expected If Statement Body",
                        self.source_id, opening.s, opening.e
                    );
                    return Statement::err();
                }

                let stmt = self.parse_statement();

                let had_semicolon = if self.peek().is_some_and(|t| t.t == Semicolon) {
                    self.get()
                } else {
                    None
                };

                let else_clause = if self
                    .peek()
                    .is_some_and(|x| x.t == Identifier && x.v == "else")
                {
                    let err = self.get().unwrap();

                    match self.peek() {
                        Some(x) if x.t == Colon => {
                            self.get();
                            Some(Box::new(self.parse_statement()))
                        }
                        Some(x) if x.t == OpenBrace => Some(Box::new(self.parse_statement())),
                        Some(x) if x.t == Identifier && x.v == "if" => {
                            Some(Box::new(self.parse_statement()))
                        }
                        Some(x) => {
                            error!(
                                "Expected ':', '{', or 'if' after 'else'",
                                self.source_id, x.s, x.e
                            );
                            return Statement::err();
                        }
                        None => {
                            fatal!("Else Followed By EOF", self.source_id, err.s, err.e);
                        }
                    }
                } else {
                    if let Some(hs) = had_semicolon {
                        self.reverse(hs);
                    }
                    None
                };

                Statement::new(
                    Sk::If(expr, Box::new(stmt), else_clause),
                    t.s,
                    self.last_token.e,
                )
            }

            "repeat" => {
                let count = self.parse_expression(BindingPower::default());

                let target = if self
                    .peek()
                    .is_some_and(|x| x.t == Identifier && x.v == "as")
                {
                    self.get();
                    Some(self.parse_expression(BindingPower::default()))
                } else {
                    None
                };

                let opening = self.peek();
                if opening.is_none() {
                    fatal!(
                        "Repeat Statement Unopened",
                        self.source_id,
                        self.last_token.e - 1,
                        self.last_token.e
                    )
                }
                let opening = opening.unwrap();

                if opening.t == Colon {
                    self.get();
                } else if opening.t != OpenBrace {
                    error!(
                        "Expected Repeat Statement Body",
                        self.source_id, opening.s, opening.e
                    );
                    return Statement::err();
                }

                let stmt = self.parse_statement();
                let stmte = stmt.e;

                Statement::new(Sk::Repeat(count, target, Box::new(stmt)), t.s, stmte)
            }

            "break" => Statement::new(Sk::Break, t.s, t.e),

            "continue" => Statement::new(Sk::Continue, t.s, t.e),

            "return" => {
                let value = if self
                    .peek()
                    .is_some_and(|x| x.t != Semicolon || x.t != CloseBrace)
                {
                    Some(self.parse_expression(BindingPower::default()))
                } else {
                    None
                };

                Statement::new(Sk::Return(value), t.s, self.last_token.e)
            }

            "ns" => {
                let name = self.peek();
                let name = match name {
                    None => None,
                    Some(c) if c.t == OpenBrace => None,
                    Some(c) if c.t == Identifier => {
                        self.get();
                        Some(c.v)
                    }
                    Some(c) => {
                        error!("Expected Identifier or \"{\"", self.source_id, c.s, c.e);
                        return Statement::err();
                    }
                };

                let next = self.peek();
                if next.as_ref().is_none() {
                    fatal!(
                        "Expected \"{\"",
                        self.source_id,
                        self.last_token.s,
                        self.last_token.e
                    );
                }

                let next = next.unwrap();
                if next.t != OpenBrace {
                    error!("Expected \"{\"", self.source_id, next.s, next.e);
                }
                self.get();

                let (new_depth, err) = self.depth.overflowing_add(1);
                if err {
                    fatal!("Exceeded Maximum Depth", self.source_id, t.s, t.e)
                }
                self.depth = new_depth;
                Statement::new(Sk::Block(name, self.parse_block()), t.s, next.e)
            }

            "fn" => {
                let name = self.get();
                if name.as_ref().is_none_or(|x| x.t != Identifier) {
                    return Statement::err();
                }

                let next = self.peek();
                if next.as_ref().is_none_or(|x| x.t != OpenBracket) {
                    return Statement::err();
                }
                let next = next.unwrap();
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
                        error!("Expected Closing Bracket", self.source_id, t.s, t.e);
                        return Statement::err();
                    }
                    None => {
                        error!(
                            "Unclosed Function Parameters",
                            self.source_id, next.s, next.e
                        );
                        return Statement::err();
                    }
                }

                match self.peek() {
                    Some(t) if t.t == OpenBrace => {}
                    Some(t) if t.t == Colon => {
                        self.get();
                    }
                    Some(t) => {
                        error!("No Function Body", self.source_id, t.s, t.e);
                        return Statement::err();
                    }
                    None => {
                        fatal!("No Function Body", self.source_id, t.s, t.e);
                    }
                }

                let stmt = self.parse_statement();
                let stmte = stmt.e;

                Statement::new(
                    Sk::Function(name.unwrap().v, args, Box::new(stmt)),
                    t.s,
                    stmte,
                )
            }

            _ => {
                let ts = t.s;
                self.reverse(t);

                if self.has_equals_before_semicolon() {
                    let target = self.parse_expression(BindingPower::default());

                    match self.get() {
                        Some(eq) if eq.t == Equals => {
                            let value = self.parse_expression(BindingPower::default());
                            let valuee = value.e;
                            Statement::new(Sk::Assignment(target, value), ts, valuee)
                        }
                        Some(other) => {
                            error!(
                                "Expected '=' after assignment target",
                                self.source_id, other.s, other.e
                            );
                            Statement::err()
                        }
                        None => {
                            error!(
                                "Unexpected EOF in assignment",
                                self.source_id, self.last_token.s, self.last_token.e
                            );
                            Statement::err()
                        }
                    }
                } else {
                    let expr = self.parse_expression(BindingPower::default());
                    let exprs = expr.s;
                    let expre = expr.e;
                    Statement::new(Sk::Evaluation(expr), exprs, expre)
                }
            }
        }

        //error!("Unknown Statement", self.source_id, t.s, t.e);
        //Statement::err()
    }

    fn parse_block(&mut self) -> Vec<Statement> {
        let mut result = Vec::new();
        loop {
            let stmt = self.parse_statement();

            if stmt.k == Sk::End {
                break;
            } else if stmt.k == Sk::None {
                continue;
            } else if stmt.k == Sk::Error {
                // fast forward
                while self
                    .get()
                    .is_some_and(|x| x.t != crate::lexer::TokenType::Semicolon)
                {}
            }

            let p = self.peek();
            if p.as_ref()
                .is_some_and(|x| x.t != crate::lexer::TokenType::Semicolon)
            {
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
