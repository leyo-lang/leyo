use crate::lexer::LexerResult;

pub fn token_stream(tokens: LexerResult) -> String {
    let mut output = String::new();
    use crate::lexer::TokenType as tt;

    for token in tokens {

        match token.t {

            tt::Semicolon => {
                output.push(';');
                output.push('\n');
            }

            _ => {
                output.push_str(&token.v); 
                output.push(' ');
            }
        }
    }

    output
}