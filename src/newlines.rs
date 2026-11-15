use crate::{fatal, lexer::*, lexer::TokenType::*};

pub fn newlines(tokens: LexerResult, source_id: usize) -> LexerResult {
    let mut result = LexerResult::new();
    let mut in_brackets = 0u16;

    for token in tokens {
        if token.t != Newline {
            match token.t {
                OpenBracket => {
                    let (res, err) = in_brackets.overflowing_add(1);
                    if err { fatal!("Exceeded Maximum Depth", source_id, token.s, token.e) }
                    in_brackets = res;
                },
                _ => {},
            }

            result.push(token);
            continue;
        }


    }

    result
}