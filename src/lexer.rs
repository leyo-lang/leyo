use crate::{error, fatal};

#[derive(Debug, PartialEq, Clone)]
pub enum TokenType {
    Unknown,
    Eof,
    Newline, // removed before parsing

    Integer,
    Float,
    Character,
    String,

    Identifier,

    Equals,

    Plus,
    Minus,
    Multiply,
    Divide,
    Exponent,
    Modulous,

    PlusEquals,
    MinusEquals,
    MultiplyEquals,
    DivideEquals,
    ExponentEquals,
    ModulousEquals,

    GreaterThan,
    LessThan,
    GreaterThanEqual,
    LessThanEqual,
    EqualTo,
    NotEqualTo,

    And,
    Or,
    Not,
    Xor,

    Increment,
    Decrement,

    OpenBracket,
    CloseBracket,
    OpenBrace,
    CloseBrace,
    OpenSquareBracket,
    CloseSquareBracket,

    Semicolon,
    Comma,
    Colon,
    DoubleColon,
    Excalmation,
    FullStop,
}

#[derive(Clone, PartialEq, Debug)]
pub struct Token {
    pub t: TokenType,
    pub v: String,
    pub s: usize,
    pub e: usize,
}

enum LexerMode {
    Normal,
    String,
    Identifier,
    Number,
    Comment,
    LineComment,
}

pub type LexerResult = Vec<Token>;

struct Lexer {
    stream_in: Vec<char>,
    index: usize,
    stream_out: Vec<Token>,
    mode: LexerMode,
    last_was_semicolon: bool,
    source_id: usize,
}

macro_rules! token {
    ($self:expr, $t:expr, $v:expr) => {
        Token {
            t: $t,
            v: $v.into(),
            s: ($self).index,
            e: ($self).index,
        }
    };

    ($self:expr, $t:expr, $v:expr, $start:expr) => {
        Token {
            t: $t,
            v: $v.into(),
            s: $start,
            e: ($self).index,
        }
    };

    ($self:expr, $t:expr, $v:expr, $start:expr, $end:expr) => {
        Token {
            t: $t,
            v: $v.into(),
            s: $start,
            e: $end,
        }
    };
}

impl Lexer {
    fn new(id: usize, stream: String) -> Self {
        Self {
            stream_in: stream.chars().collect(),
            index: 0,
            stream_out: Vec::new(),
            mode: LexerMode::Normal,
            last_was_semicolon: true,
            source_id: id,
        }
    }

    fn current(&self) -> Option<char> {
        self.stream_in.get(self.index).copied()
    }

    fn peek(&self) -> Option<char> {
        self.stream_in.get(self.index + 1).copied()
    }

    fn advance(&mut self) {
        self.index += 1;
        /*if self.current().is_none() {
            fatal!("Out Of Bounds", self.source_id, self.index-2, self.index-2);
        }*/
    }

    fn current_unicode_escaped(&self) -> Option<char> {
        match self.current() {
            Some('\\') => Some('\\'),
            Some('"') => Some('"'),
            Some('n') => Some('\n'),
            Some('r') => Some('\r'),
            Some('t') => Some('\t'),
            Some('0') => Some('\0'),
            Some('b') => Some('\x08'),
            Some('f') => Some('\x0C'),
            Some(_) => {
                error!("Invalid Escape", self.source_id, self.index - 1, self.index);

                Some('\0')
            }
            None => {
                fatal!(
                    "Unterminated Character Escape",
                    self.source_id,
                    self.index,
                    self.index,
                    "Make sure to place an ending '"
                );
            }
        }
    }

    fn push(&mut self, tok: Token) {
        self.last_was_semicolon = tok.t == TokenType::Semicolon;
        self.stream_out.push(tok);
    }

    fn handle_comment(&mut self, line: bool) {
        if line {
            while self.current() != Some('\n') {
                self.advance();
            }
        } else {
            while !(self.current() == Some('*') && self.peek() == Some('/')) {
                self.advance();
            }
            self.advance(); // onto /
            self.advance(); // onto \n
        }

        self.mode = LexerMode::Normal;
    }

    fn handle_identifier(&mut self) {
        let start_pos = self.index;
        let mut res = String::new();

        let c = self.current();
        if c.is_none() {
            fatal!(
                "Out Of Bounds",
                self.source_id,
                self.index - 1,
                self.index - 1
            );
        }
        if !c.unwrap().is_ascii_alphabetic() && c != Some('_') {
            error!(
                "Illegal Identifier",
                self.source_id,
                start_pos,
                start_pos,
                "The first character must be either a letter or an underscore"
            );
        }
        res.push(c.unwrap());
        self.advance();

        loop {
            let c = self.current();

            if c.is_none_or(|x| !x.is_ascii_alphanumeric() && x != '_') {
                let token_type = match res.as_str() {
                    "and" => TokenType::And,
                    "or" => TokenType::Or,
                    "not" => TokenType::Not,
                    "xor" => TokenType::Xor,
                    _ => TokenType::Identifier,
                };
                self.push(token!(self, token_type, res, start_pos));
                self.mode = LexerMode::Normal;
                return;
            }

            res.push(c.unwrap());
            self.advance();
        }
    }

    fn handle_number(&mut self) {
        let start_pos = self.index;
        let mut dot_seen = false;
        let mut num_so_far = String::new();

        if self.current() == Some('.') {
            num_so_far.push('0');
        }

        loop {
            let c = self.current();

            if c == Some('\'') {
                // like 100'000'000
                self.advance();
                continue;
            }

            if c.is_none_or(|x| !x.is_ascii_digit() && x != '.') {
                if dot_seen {
                    self.push(token!(self, TokenType::Float, num_so_far, start_pos));
                    self.mode = LexerMode::Normal;
                    return;
                } else {
                    self.push(token!(self, TokenType::Integer, num_so_far, start_pos));
                    self.mode = LexerMode::Normal;
                    return;
                }
            }

            if c == Some('.') {
                if dot_seen {
                    error!(
                        "Invalid Number",
                        self.source_id,
                        start_pos,
                        self.index,
                        "Check there is only one decimal point."
                    );
                }
                dot_seen = true;
            }

            num_so_far.push(c.unwrap());
            self.advance();
        }
    }

    fn handle_string(&mut self) {
        let start_pos = self.index;
        self.advance();

        let mut res = String::new();

        loop {
            match self.current() {
                Some('"') => {
                    self.advance();
                    self.mode = LexerMode::Normal;
                    self.push(token!(self, TokenType::String, res, start_pos));
                    return;
                }

                Some('\\') => {
                    self.advance();

                    let c = self.current_unicode_escaped();
                    if c.is_none() {
                        fatal!(
                            "Unterminated String",
                            self.source_id,
                            start_pos,
                            self.index,
                            "Make sure to place an ending \""
                        );
                    }
                    res.push(c.unwrap());

                    self.advance();
                }

                Some(c) => {
                    res.push(c);
                    self.advance();
                }

                None => {
                    fatal!(
                        "Unterminated String",
                        self.source_id,
                        start_pos,
                        self.index,
                        "Make sure to place an ending \""
                    );
                }
            }
        }
    }

    fn handle_normal(&mut self) {
        let s = self.index;
        match self
            .current()
            .expect("Internal FATAL - Main Lexer Loop not defensive")
        {
            // can asume safe due to main lex loop checking for us
            '\n' => {
                self.push(token!(self, TokenType::Newline, "\n"));

                self.advance();
            }

            ' ' | '\t' | '\r' => {
                self.advance();
            }

            '\'' => {
                self.advance();
                if self.current() == Some('\\') {
                    self.advance();
                    let c = self.current_unicode_escaped();
                    if c.is_none() {
                        fatal!(
                            "Unterminated Character",
                            self.source_id,
                            self.index - 4,
                            self.index - 2,
                            "Make sure to place an ending '"
                        );
                    }
                    self.push(token!(self, TokenType::Character, c.unwrap(), s));
                } else {
                    self.push(token!(
                        self,
                        TokenType::Character,
                        self.current().unwrap(),
                        s
                    ));
                }
                self.advance();
                if self.current() != Some('\'') {
                    error!(
                        "Unterminated Character",
                        self.source_id,
                        self.index - 3,
                        self.index,
                        "Make sure to place an ending '"
                    );
                }
                self.advance();
            }

            ';' => {
                self.push(token!(self, TokenType::Semicolon, ';'));
                self.advance();
            }

            ',' => {
                self.push(token!(self, TokenType::Comma, ','));
                self.advance();
            }

            ':' => {
                if self.peek().is_some_and(|x| x == ':') {
                    self.push(token!(
                        self,
                        TokenType::DoubleColon,
                        "::",
                        self.index,
                        self.index + 1
                    ));
                    self.advance();
                    self.advance();
                } else {
                    self.push(token!(self, TokenType::Colon, ':'));
                    self.advance();
                }
            }

            '.' => {
                self.push(token!(self, TokenType::FullStop, '.'));
                self.advance();
            }

            '{' => {
                self.push(token!(self, TokenType::OpenBrace, '{'));
                self.advance();
            }

            '}' => {
                self.push(token!(self, TokenType::CloseBrace, '}'));
                self.advance();
            }

            '(' => {
                self.push(token!(self, TokenType::OpenBracket, '('));
                self.advance();
            }

            ')' => {
                self.push(token!(self, TokenType::CloseBracket, ')'));
                self.advance();
            }

            '[' => {
                self.push(token!(self, TokenType::OpenSquareBracket, '['));
                self.advance();
            }

            ']' => {
                self.push(token!(self, TokenType::CloseSquareBracket, ']'));
                self.advance();
            }

            '*' => {
                match self.peek() {
                    Some('*') => {
                        self.advance();
                        self.advance();
                        if self.current() == Some('=') {
                            self.push(token!(self, TokenType::ExponentEquals, "**=", s));
                        } else {
                            self.push(token!(self, TokenType::Exponent, "**", s));
                        }
                    }
                    Some('=') => {
                        self.push(token!(self, TokenType::MultiplyEquals, "*=", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::Multiply, '*')),
                }
                self.advance();
            }

            '/' => {
                match self.peek() {
                    Some('/') => self.mode = LexerMode::LineComment,
                    Some('*') => self.mode = LexerMode::Comment,
                    Some('=') => {
                        self.push(token!(self, TokenType::DivideEquals, "/=", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::Divide, '/')),
                }
                self.advance();
            }

            '+' => {
                match self.peek() {
                    Some('+') => {
                        self.push(token!(self, TokenType::Increment, "++", s));
                        self.advance()
                    }
                    Some('=') => {
                        self.push(token!(self, TokenType::PlusEquals, "+=", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::Plus, '+')),
                }
                self.advance();
            }

            '-' => {
                match self.peek() {
                    Some('-') => {
                        self.push(token!(self, TokenType::Decrement, "--", s));
                        self.advance()
                    }
                    Some('=') => {
                        self.push(token!(self, TokenType::MinusEquals, "-=", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::Minus, '-')),
                }
                self.advance();
            }

            '%' => {
                match self.peek() {
                    Some('=') => {
                        self.push(token!(self, TokenType::ModulousEquals, "%=", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::Modulous, '%')),
                }
                self.advance();
            }

            '>' => {
                match self.peek() {
                    Some('=') => {
                        self.push(token!(self, TokenType::GreaterThanEqual, ">=", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::GreaterThan, '>')),
                }
                self.advance();
            }

            '<' => {
                match self.peek() {
                    Some('=') => {
                        self.push(token!(self, TokenType::LessThanEqual, "<=", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::LessThan, '<')),
                }
                self.advance();
            }

            '!' => {
                match self.peek() {
                    Some('=') => {
                        self.push(token!(self, TokenType::NotEqualTo, "!=", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::Excalmation, '!')),
                }
                self.advance();
            }

            '=' => {
                match self.peek() {
                    Some('=') => {
                        self.push(token!(self, TokenType::EqualTo, "==", s));
                        self.advance()
                    }
                    _ => self.push(token!(self, TokenType::Equals, '=')),
                }
                self.advance();
            }

            '"' => {
                self.mode = LexerMode::String;
            }

            c => {
                if c.is_ascii_alphabetic() || c == '_' {
                    self.mode = LexerMode::Identifier;
                } else if c.is_ascii_digit()
                    || (c == '.' && self.peek().is_some_and(|x| x.is_ascii_digit()))
                {
                    self.mode = LexerMode::Number;
                } else {
                    error!("Invalid Character", self.source_id, self.index, self.index);
                    self.push(token!(self, TokenType::Unknown, c));
                    self.advance();
                }
            }
        }
    }

    fn finish(mut self) -> LexerResult {
        self.push(token!(self, TokenType::Eof, "EOF"));
        self.stream_out
    }
}

pub fn lex(stream: String, id: usize) -> LexerResult {
    let mut lexer = Lexer::new(id, stream);

    while lexer.current().is_some() {
        match lexer.mode {
            LexerMode::Normal => lexer.handle_normal(),
            LexerMode::String => lexer.handle_string(),
            LexerMode::Comment => lexer.handle_comment(false),
            LexerMode::LineComment => lexer.handle_comment(true),
            LexerMode::Identifier => lexer.handle_identifier(),
            LexerMode::Number => lexer.handle_number(),
        }
    }

    lexer.finish()
}
