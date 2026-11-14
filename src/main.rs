mod lexer;
mod parser;
mod errors;

#[derive(Clone)]
struct FileData {
    contents: String,
    id: usize,
}

fn register_file(filepath: &str) -> FileData {
    let contents = std::fs::read_to_string(filepath);
    if contents.is_err() {
        fatalns!(format!("Could Not Open File {filepath}").as_str(), "Check there is not a typo in the name and that the file exists.");
    }

    let contents = contents.unwrap();

    let id = errors::add_source(filepath.to_string(), Some(filepath.to_string()), contents.clone());

    FileData{contents, id}
}

fn lex(filedata: FileData) -> lexer::LexerResult {
    let res = lexer::lex(filedata.contents, filedata.id);

    if errors::has_errors() {
        fatalns!("Errors Found After Lexing");
    }

    res
}

fn parse(tokens: lexer::LexerResult, filedata: FileData) -> parser::Program {
    let res = parser::parse(tokens, filedata.id);

    if errors::has_errors() {
        fatalns!("Errors Found After Parsing");
    }

    res
} 

fn main() {
    let filepaths = vec!["example.leyo"];

    for filepath in filepaths {
        let filedata = register_file(filepath); 
        let result = lex(filedata.clone());
        lexer::print_lexer_result(&result);

        let result = parse(result, filedata.clone());

        if errors::has_any() {
            errors::print_all();
        }        
    }
}
