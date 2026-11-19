use std::time::Instant;

mod cli;
mod debug;
mod errors;
mod lexer;
mod newlines;
mod parser;

#[derive(Clone)]
struct FileData {
    contents: String,
    id: usize,
}

fn register_file(filepath: &str) -> FileData {
    let contents = std::fs::read_to_string(filepath);
    if contents.is_err() {
        fatal!(
            format!("Could Not Open File {filepath}").as_str(),
            "Check there is not a typo in the name and that the file exists."
        );
    }

    let contents = contents.unwrap();

    let id = errors::add_source(
        filepath.to_string(),
        Some(filepath.to_string()),
        contents.clone(),
    );

    FileData { contents, id }
}

fn pipeline(o: &cli::Options, filepath: String) {
    let start = std::time::Instant::now();

    let filedata = register_file(&filepath);

    let split = Instant::now();

    let lexer_result = lexer::lex(filedata.contents, filedata.id);

    if o.debug.tokens {
        debug::logging::write_log(&debug::printing::print_lexer_result(&lexer_result));
        debug::logging::write_log(&debug::reconstruct::token_stream(&lexer_result));
    }

    if o.debug.time {
        info!(
            format!(
                "Lexing: Took {}s - Split: {}s",
                debug::printing::DisplayDuration(start.elapsed()),
                debug::printing::DisplayDuration(split.elapsed())
            )
            .as_str()
        );
    }

    if o.stop_at == cli::Stage::Lexing {
        return;
    }

    let split = Instant::now();

    let newlines_result = newlines::newlines(lexer_result, filedata.id);

    if o.debug.time {
        info!(
            format!(
                "Newlines: Took {}s - Split: {}s",
                debug::printing::DisplayDuration(start.elapsed()),
                debug::printing::DisplayDuration(split.elapsed())
            )
            .as_str()
        );
    }

    if o.stop_at == cli::Stage::Newlines {
        return;
    }

    let split = Instant::now();

    let parser_result = parser::parse(newlines_result, filedata.id);

    if o.debug.time {
        info!(
            format!(
                "Parsing: Took {}s - Split: {}s",
                debug::printing::DisplayDuration(start.elapsed()),
                debug::printing::DisplayDuration(split.elapsed())
            )
            .as_str()
        );
    }

    if o.stop_at == cli::Stage::Parsing {
        return;
    }

    let split = Instant::now();

    let semantic_result = todo_semantic(parser_result);

    if o.debug.time {
        info!(
            format!(
                "Semantic: Took {}s - Split: {}s",
                debug::printing::DisplayDuration(start.elapsed()),
                debug::printing::DisplayDuration(split.elapsed())
            )
            .as_str()
        );
    }

    if o.stop_at == cli::Stage::PreBuild {
        return;
    }
}

fn main() {
    let o = cli::parse();

    errors::init(&o); // only needed for warnings (cli::parse only has fatals)

    if o.debug.any() && debug::logging::init_log("leyo_debug.log").is_err() {
        fatal!("Could Not Initialise Log")
    }

    let files = o.files.clone();

    for filepath in files {
        pipeline(&o, filepath);
    }
}
