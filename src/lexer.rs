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
}

pub type LexerResult = Vec<Token>;

struct Lexer {
    stream_in: String,
    index: usize,
    stream_out: Vec<Token>,
    mode: LexerMode,
}

macro_rules! token {
    ($t:expr, $v:expr) => {
        Token { t: $t, v: $v }
    };
}

impl Lexer {
    fn new(stream: String) -> Self {
        Self {
            stream_in: stream,
            index: 0,
            stream_out: Vec::new(),
            mode: LexerMode::Normal,
        }
    }

    fn current(&self) -> char {
        self.stream_in.as_bytes()[self.index] as char
    }

    fn advance(&mut self) {
        self.index += 1;
    }

    fn current_unicode(&self) -> char {
        self.stream_in.as_bytes()[self.index] as char
    }

    fn advance_unicode(&mut self) {
        self.index += 1;
    }
    
    fn push(&mut self, tok: Token) {
        self.stream_out.push(tok);
    }

    fn read_string(&mut self) {
        let stream_iter= self.stream_in.
        while self.current() != '"' {
            
        }
    }

    fn finish(mut self) -> LexerResult {
        self.push(token!(TokenType::Eof, "EOF".to_string()));
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

    lexer.finish()
}