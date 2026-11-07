mod lexer;
mod errors;

fn lex(filepath: &str) -> lexer::LexerResult {
    let contents = std::fs::read_to_string(filepath);
    if contents.is_err() {
        fatalns!(format!("Could Not Open File {filepath}").as_str(), "Check there is not a typo in the name and that the file exists.");
    }

    let contents = contents.unwrap();

    let id = errors::add_source(filepath.to_string(), Some(filepath.to_string()), contents.clone());
    
    let res = lexer::lex(contents, id);

    if errors::has_errors() {
        fatalns!("Errors Found After Lexing");
    }

    res
}

fn main() {
    let result = lex("example.leyo");
    lexer::print_lexer_result(&result);

    if errors::has_any() {
        errors::print_all();
    }
}
