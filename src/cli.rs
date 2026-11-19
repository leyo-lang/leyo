use crate::debug::config::Config as DebugConfig;
use crate::fatal;

pub enum Command {
    Run,
    Build,
    Check,
    Repl,
}

#[derive(PartialEq)]
pub enum Stage {
    Lexing,
    Newlines,
    Parsing,
    PreBuild,
    None,
}

pub struct Options {
    pub command: Command,
    pub debug: DebugConfig,
    pub stop_at: Stage,
    pub files: Vec<String>,
    pub args_to_script: Vec<String>,
    pub pedantic: bool,
}

pub fn parse() -> Options {
    let mut command = Command::Repl;
    let mut debug = DebugConfig::default();
    let mut stop_at = Stage::None;
    let mut files = Vec::new();
    let mut args_to_script = Vec::new();
    let mut pedantic = false;

    let mut args = std::env::args().skip(1).peekable();

    if let Some(first) = args.peek() {
        match first.as_str() {
            "run" => {
                command = Command::Run;
                args.next();
            }
            "check" => {
                command = Command::Check;
                stop_at = Stage::PreBuild;
                args.next();
            }
            "build" => {
                command = Command::Build;
                args.next();
            }
            _ => {}
        }
    }

    while let Some(arg) = args.next() {
        match arg.as_str() {
            "--" => {
                // rest to script
                args_to_script.extend(args);
                break;
            }

            "--pedantic" => {
                pedantic = true;
            }

            "--debug" => {
                debug.all();
            }

            s if let Some(a) = s.strip_prefix("--stop=") => match a {
                "lex" => stop_at = Stage::Lexing,

                "newlines" => stop_at = Stage::Newlines,

                "parse" => stop_at = Stage::Parsing,

                "prebuild" => stop_at = Stage::PreBuild,

                "none" => stop_at = Stage::None,

                s => {
                    fatal!(
                        format!("Unknown stop-at flag: '{s}'").as_str(),
                        "Valid flags: none, lex, newlines, parse, prebuild"
                    );
                }
            },

            s if let Some(a) = s.strip_prefix("--debug=") => {
                debug.enable_flag(a);
            }

            s => {
                files.push(s.to_string());
            }
        }
    }

    Options {
        command,
        debug,
        stop_at,
        files,
        args_to_script,
        pedantic,
    }
}
