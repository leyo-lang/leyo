#[derive(Debug)]
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

    Increment,
    Decrement,

    OpenBracket,
    CloseBracket,
    OpenBrace,
    CloseBrace,

    Semicolon,
    Comma,
    Colon,
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
    stream_in: String,
    index: usize,
    stream_out: Vec<Token>,
    mode: LexerMode,
    last_was_semicolon: bool,
}

macro_rules! token {
    ($t:expr, $v:expr) => {
        Token { t: $t, v: $v.into() }
    };
}

impl Lexer {
    fn new(stream: String) -> Self {
        Self {
            stream_in: stream,
            index: 0,
            stream_out: Vec::new(),
            mode: LexerMode::Normal,
            last_was_semicolon: false,
        }
    }

    fn check_out_bounds(&self, modifier: Option<usize>) -> bool {
        (self.index + modifier.unwrap_or(0)) >= self.stream_in.len()
    }

    fn current(&self) -> char {
        self.stream_in.as_bytes()[self.index] as char
    }

    fn peek(&self) -> char {
        self.stream_in.as_bytes()[self.index+1] as char
    }

    fn advance(&mut self) {
        self.index += 1;
    }

    fn current_unicode(&self) -> Option<char> {
        self.stream_in.chars().nth(self.index)
    }

    fn peek_unicode(&self) -> Option<char> {
        self.stream_in.chars().nth(self.index+1)
    }

    fn advance_unicode(&mut self) {
        self.index += 1;
    }
    
    fn push(&mut self, tok: Token) {
        self.stream_out.push(tok);
    }

    fn read_string(&mut self) -> String {
        let mut res = String::new();

        while let Some(c) = self.current_unicode() {
            if c == '"' { break; }
            res.push(c);
            self.advance_unicode();
        }

        self.advance();

        res
    }

    fn handle_comment(&mut self, line: bool) {
        if line {
            while self.current_unicode() != Some('\n') {
                self.advance_unicode();
            }
        } else {
            while !(self.current_unicode() == Some('*') && self.peek_unicode() == Some('/')) {
                self.advance_unicode();
            }
            self.advance(); // onto /
            self.advance(); // onto \n
        }
    }

    fn handle_normal(&mut self) {
        let c = self.current();

        if c.is_whitespace() {
            if c == '\n' && !self.last_was_semicolon {
                self.push(token!(TokenType::Semicolon, ';'));
            }
            return;
        }

        if c == '/' {
            if self.peek() == '/' {
                self.mode = LexerMode::LineComment;
                return;
            } else if self.peek() == '*' {
                self.mode = LexerMode::Comment;
                return;
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

pub fn lex(stream: String) -> LexerResult {
    let lexer = Lexer::new(stream);

    while lexer.index >= lexer.stream_in.len() {
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