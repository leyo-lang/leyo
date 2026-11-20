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

fn output_timings(o: &cli::Options, stage: &str, total: Instant, split: Instant) {
    if o.debug.time {
        info!(format!("{}: {}s (Total: {}s)", stage, debug::printing::DisplayDuration(split.elapsed()), debug::printing::DisplayDuration(total.elapsed())).as_str());        
    }
}

fn pipeline(o: &cli::Options, filepath: String) {
    let start = std::time::Instant::now();

    let filedata = register_file(&filepath);

    let split = Instant::now();

    let lexer_result = lexer::lex(filedata.contents, filedata.id);

    if o.debug.tokens {
        debug::logging::write_to("lexer_result", &debug::printing::print_lexer_result(&lexer_result));
        debug::logging::write_to("lexer_reconstruct", &debug::reconstruct::token_stream(&lexer_result));
    }

    output_timings(o, "Lexing", start, split);

    if errors::has_errors() {
        fatal!("Errors Found After Lexing")
    }

    if o.stop_at == cli::Stage::Lexing {
        if errors::has_any() {
            errors::print_all();
        }
        return;
    }

    let split = Instant::now();

    let newlines_result = newlines::newlines(lexer_result, filedata.id);

    if o.debug.newlines {
        debug::logging::write_to("newlines_result", &debug::printing::print_lexer_result(&newlines_result));
        debug::logging::write_to("newlines_reconstruct", &debug::reconstruct::token_stream(&newlines_result));
    }

    output_timings(o, "Newlines", start, split);

    if errors::has_errors() {
        fatal!("Errors Found After Newlines")
    }

    if o.stop_at == cli::Stage::Newlines {
        if errors::has_any() {
            errors::print_all();
        }
        return;
    }

    let split = Instant::now();

    let parser_result = parser::parse(newlines_result, filedata.id);

    if o.debug.ast {
        debug::logging::write_to("parser_result", &debug::printing::ast_pretty_print(&parser_result));
    }

    output_timings(o, "Parsing", start, split);

    if errors::has_errors() {
        fatal!("Errors Found After Parsing")
    }

    if o.stop_at == cli::Stage::Parsing {
        if errors::has_any() {
            errors::print_all();
        }
        return;
    }

    let split = Instant::now();

    let semantic_result = todo_semantic(parser_result);

    output_timings(o, "Semantic", start, split);

    if errors::has_errors() {
        fatal!("Errors Found After Semantic")
    }

    if o.stop_at == cli::Stage::PreBuild {
        if errors::has_any() {
            errors::print_all();
        }
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
