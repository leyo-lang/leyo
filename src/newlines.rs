use crate::lexer::TokenType::{self as tt, Semicolon};
use crate::{
    fatal,
    lexer::{LexerResult, Token},
    warn,
};

fn can_end_statement(token: &tt) -> bool {
    matches!(
        token,
        tt::Identifier
            | tt::Integer
            | tt::Float
            | tt::String
            | tt::CloseBracket
            | tt::CloseSquareBracket
            | tt::CloseBrace
            | tt::Character
    )
}

fn continues_statement(token: &tt) -> bool {
    matches!(
        token,
        tt::Plus
            | tt::Minus
            | tt::Multiply
            | tt::Divide
            | tt::Modulous
            | tt::EqualTo
            | tt::NotEqualTo
            | tt::LessThan
            | tt::GreaterThan
            | tt::LessThanEqual
            | tt::GreaterThanEqual
            | tt::And
            | tt::Not
            | tt::Or
            | tt::Xor
            | tt::FullStop
            | tt::DoubleColon
            | tt::Exponent
            | tt::Equals
    )
}

pub fn newlines(tokens: LexerResult, source_id: usize) -> LexerResult {
    let mut result = LexerResult::new();
    let mut in_brackets = 0u16;
    let mut ends_in_newline = false;
    let mut last = Token {
        t: tt::Unknown,
        v: String::new(),
        s: 0,
        e: 0,
    };

    let mut iter = tokens.into_iter().peekable();

    while let Some(token) = iter.next() {
        if token.t != tt::Newline {
            match token.t {
                tt::OpenBracket | tt::OpenSquareBracket => {
                    let (res, err) = in_brackets.overflowing_add(1);
                    if err {
                        fatal!("Exceeded Maximum Depth", source_id, token.s, token.e)
                    }
                    in_brackets = res;
                }
                tt::CloseBracket | tt::CloseSquareBracket => {
                    in_brackets = in_brackets.saturating_sub(1);
                }
                tt::Eof => {
                    if last.t != Semicolon && can_end_statement(&last.t) {
                        result.push(Token {
                            t: Semicolon,
                            v: "\n".to_string(),
                            s: token.s,
                            e: token.e,
                        });
                    }

                    if !ends_in_newline && last.t != tt::Unknown {
                        warn!(
                            "Stream Should End In Newline",
                            source_id,
                            token.e - 1,
                            token.e,
                            "Add a newline to the end of the input."
                        );
                    }
                }
                _ => {}
            }

            ends_in_newline = false;
            last = token.clone();
            result.push(token);
            continue;
        }

        ends_in_newline = true;

        if in_brackets != 0 {
            continue;
        }
        if last.t == tt::Semicolon {
            continue;
        }
        if last.t == tt::Newline {
            continue;
        }
        if !can_end_statement(&last.t) {
            continue;
        }
        if iter
            .peek()
            .is_some_and(|x| continues_statement(&x.t) || matches!(x.v.as_str(), "else"))
        {
            continue;
        }

        last = token.clone();
        result.push(Token {
            t: tt::Semicolon,
            v: "\n".to_string(),
            s: token.s,
            e: token.e,
        });
    }

    result
}
