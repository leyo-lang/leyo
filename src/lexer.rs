use crate::{error, fatal, warn };

#[derive(Debug, PartialEq)]
pub enum TokenType {
    Unknown,
    Eof,

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
    Expodent,
    Modulous,

    PlusEquals,
    MinusEquals,
    MultiplyEquals,
    DivideEquals,
    ExpodentEquals,
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
    Excalmation,
}

pub struct Token {
    t: TokenType,
    v: String,
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
    ($t:expr, $v:expr) => {
        Token { t: $t, v: $v.into() }
    };
}

impl Lexer {
    fn new(id: usize, stream: String) -> Self {
        if !stream.ends_with('\n') {
            warn!("Stream Should End In Newline", id, stream.len()-1, stream.len()-1, "Add a newline to the end of the input.");
        }

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
            Some('\\') => return Some('\\'),
            Some('"')  => return Some('"'),
            Some('n')  => return Some('\n'),
            Some('r')  => return Some('\r'),
            Some('t')  => return Some('\t'),
            Some('0')  => return Some('\0'),
            Some('b')  => return Some('\x08'),
            Some('f')  => return Some('\x0C'),
            Some(_) => {
                error!("Invalid Escape", self.source_id,
                    self.index - 1, self.index);

                return Some('\0');
            }
            None => {
                fatal!("Unterminated Character Escape", self.source_id, self.index, self.index, "Make sure to place an ending '");
            }
        }
    }
    
    fn push(&mut self, tok: Token) {
        if tok.t == TokenType::Semicolon {
            self.last_was_semicolon = true;
        } else {
            self.last_was_semicolon = false;
        }
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
            fatal!("Out Of Bounds", self.source_id, self.index-1, self.index-1);
        }
        if !c.unwrap().is_ascii_alphabetic() && c != Some('_') {
            error!("Illegal Identifier", self.source_id, start_pos, start_pos, "The first character must be either a letter or an underscore");
        }
        res.push(c.unwrap());
        self.advance();

        loop {
            let c = self.current();
            
            if c.is_none_or(|x| !x.is_ascii_alphanumeric() && x != '_') {
                let token_type = match res.as_str() {
                    "and" => TokenType::And,
                    "or"  => TokenType::Or,
                    "not" => TokenType::Not,
                    "xor" => TokenType::Xor,
                    _ => TokenType::Identifier,
                };
                self.push(token!(token_type, res));
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
                    self.push(token!(TokenType::Float, num_so_far));
                    self.mode = LexerMode::Normal;
                    return;
                } else {
                    self.push(token!(TokenType::Integer, num_so_far));
                    self.mode = LexerMode::Normal;
                    return;
                }
            }

            if c == Some('.') {
                if dot_seen {
                    error!("Invalid Number", self.source_id, start_pos, self.index, "Check there is only one decimal point.");
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
                    self.push(token!(TokenType::String, res));
                    return;
                }

                Some('\\') => {
                    self.advance();

                    let c = self.current_unicode_escaped();
                    if c.is_none() {
                        fatal!("Unterminated String", self.source_id, start_pos, self.index, "Make sure to place an ending \"");
                    }
                    res.push(c.unwrap());

                    self.advance();
                }

                Some(c) => {
                    res.push(c);
                    self.advance();
                }

                None => {
                    fatal!("Unterminated String", self.source_id, start_pos, self.index, "Make sure to place an ending \"");
                }
            }
        }

        
    }

    fn handle_normal(&mut self) {
        match self.current().expect("Internal FATAL - Main Lexer Loop not defensive") { // can asume safe due to main lex loop checking for us
            '\n' => {
                if !self.last_was_semicolon {
                    self.push(token!(TokenType::Semicolon, ';'));
                }

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
                        fatal!("Unterminated Character", self.source_id, self.index-4, self.index-2, "Make sure to place an ending '");
                    }
                    self.push(token!(TokenType::Character, c.unwrap()));
                } else {
                    self.push(token!(TokenType::Character, self.current().unwrap()));
                }
                self.advance();
                if self.current() != Some('\'') {
                    error!("Unterminated Character", self.source_id, self.index-3, self.index, "Make sure to place an ending '");
                }
                self.advance();
            }

            ';' => {
                self.push(token!(TokenType::Semicolon, ';'));
                self.advance();
            }

            ',' => {
                self.push(token!(TokenType::Comma, ','));
                self.advance();
            }

            ':' => {
                self.push(token!(TokenType::Colon, ':'));
                self.advance();
            }

            '{' => {
                self.push(token!(TokenType::OpenBrace, '{'));
                self.advance();
            }

            '}' => {
                self.push(token!(TokenType::CloseBrace, '}'));
                self.advance();
            }

            '(' => {
                self.push(token!(TokenType::OpenBracket, '('));
                self.advance();
            }

            ')' => {
                self.push(token!(TokenType::CloseBracket, ')'));
                self.advance();
            }

            '[' => {
                self.push(token!(TokenType::OpenSquareBracket, '['));
                self.advance();
            }

            ']' => {
                self.push(token!(TokenType::CloseSquareBracket, ']'));
                self.advance();
            }

            '*' => {
                match self.peek() {
                    Some('*') => {
                        self.advance();
                        self.advance();
                        if self.current() == Some('=') {
                            self.push(token!(TokenType::ExpodentEquals, "**="));
                        } else {
                            self.push(token!(TokenType::Expodent, "**"));
                        }
                    }
                    Some('=') => {self.push(token!(TokenType::MultiplyEquals, "*=")); self.advance()},
                    _   => self.push(token!(TokenType::Multiply, '*')),
                }
                self.advance();
            }

            '/' => {
                match self.peek() {
                    Some('/') => self.mode = LexerMode::LineComment,
                    Some('*') => self.mode = LexerMode::Comment,
                    Some('=') => {self.push(token!(TokenType::DivideEquals, "/=")); self.advance()},
                    _   => self.push(token!(TokenType::Divide, '/')),
                }
                self.advance();
            }

            '+' => {
                match self.peek() {
                    Some('+') => {self.push(token!(TokenType::Increment, "++")); self.advance()},
                    Some('=') => {self.push(token!(TokenType::PlusEquals, "+=")); self.advance()},
                    _   => self.push(token!(TokenType::Plus, '+')),
                }
                self.advance();
            }

            '-' => {
                match self.peek() {
                    Some('-') => {self.push(token!(TokenType::Decrement, "--")); self.advance()},
                    Some('=') => {self.push(token!(TokenType::MinusEquals, "-=")); self.advance()},
                    _   => self.push(token!(TokenType::Minus, '-')),
                }
                self.advance();
            }

            '%' => {
                match self.peek() {
                    Some('=') => {self.push(token!(TokenType::ModulousEquals, "%=")); self.advance()},
                    _   => self.push(token!(TokenType::Modulous, '%')),
                }
                self.advance();
            }

            '>' => {
                match self.peek() {
                    Some('=') => {self.push(token!(TokenType::GreaterThanEqual, ">=")); self.advance()},
                    _   => self.push(token!(TokenType::GreaterThan, '>')),
                }
                self.advance();
            }

            '<' => {
                match self.peek() {
                    Some('=') => {self.push(token!(TokenType::LessThanEqual, "<=")); self.advance()},
                    _   => self.push(token!(TokenType::LessThan, '<')),
                }
                self.advance();
            }

            '!' => {
                match self.peek() {
                    Some('=') => {self.push(token!(TokenType::NotEqualTo, "!=")); self.advance()},
                    _   => self.push(token!(TokenType::Excalmation, '!')),
                }
                self.advance();
            }

            '=' => {
                match self.peek() {
                    Some('=') => {self.push(token!(TokenType::EqualTo, "==")); self.advance()},
                    _   => self.push(token!(TokenType::Equals, '=')),
                }
                self.advance();
            }

            '"' => {
                self.mode = LexerMode::String;
            }

            c => {
                if c.is_ascii_alphabetic() || c == '_' {
                    self.mode = LexerMode::Identifier;
                } else if c.is_ascii_digit() || (c == '.' && self.peek().is_some_and(|x| x.is_ascii_digit())) {
                    self.mode = LexerMode::Number;
                } else {
                    error!("Invalid Character", self.source_id, self.index, self.index);
                    self.push(token!(TokenType::Unknown, c)); 
                    self.advance(); 
                }

            }
        }
    }

    fn finish(mut self) -> LexerResult {
        self.push(token!(TokenType::Eof, "EOF"));
        self.stream_out
    }
}

pub fn print_lexer_result(result: &LexerResult) {
    for (i, token) in result.iter().enumerate() {
        println!(
            "{:<10} {:<20} {:<20?}",
            i,
            format!("{:?}", token.t),
            token.v
        );
    }
}

pub fn lex(stream: String, id: usize) -> LexerResult {
    let mut lexer = Lexer::new(id, stream);

    while let Some(_) = lexer.current() {
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