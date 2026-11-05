mod lexer;
mod errors;

fn lex(filepath: &str) -> lexer::LexerResult {
    let contents = std::fs::read_to_string(filepath).expect("File Does Not Exist");
    errors::add_source(filepath.to_string(), Some(filepath.to_string()), contents.clone());
    
    lexer::lex(contents)
}

fn main() {
    let result = lex("example.leyo");
    lexer::print_lexer_result(&result);
}
