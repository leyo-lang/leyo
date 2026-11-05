use std::{process::exit, sync::{Mutex, OnceLock}};

#[derive(Debug, Clone, PartialEq)]
enum Severity {
    Error,
    Warning,
    Info,
    Fatal,
}
impl std::fmt::Display for Severity {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let severity = match self {
            Severity::Error => "ERROR",
            Severity::Warning => "WARNING",
            Severity::Info => "INFO",
            Severity::Fatal => "FATAL",
        };

        write!(f, "{severity}")
    }
}

#[derive(Debug, Clone)]
pub struct Diagnostic {
    pub severity: Severity,
    pub message: String,

    pub file_id: usize,
    pub start: usize,
    pub end: usize,

    pub help: Option<String>,
}

#[derive(Debug, Clone)]
pub struct SourceFile {
    pub id: usize,
    pub name: String,
    pub path: Option<String>,
    pub source: String,
}

static ERRORS: OnceLock<Mutex<Vec<Diagnostic>>> = OnceLock::new();
static SOURCES: OnceLock<Mutex<Vec<SourceFile>>> = OnceLock::new();

fn error_pool() -> &'static Mutex<Vec<Diagnostic>> {
    ERRORS.get_or_init(|| Mutex::new(Vec::new()))
}

pub fn add(diagnostic: Diagnostic) {
    error_pool().lock().unwrap().push(diagnostic);
}

pub fn get() -> Vec<Diagnostic> {
    error_pool().lock().unwrap().clone()
}

fn source_pool() -> &'static Mutex<Vec<SourceFile>> {
    SOURCES.get_or_init(|| Mutex::new(Vec::new()))
}

pub fn add_source(name: String, path: Option<String>, source: String) -> usize {
    let mut files = source_pool().lock().unwrap();

    let id = files.len();

    files.push(SourceFile {
        id,
        name,
        path,
        source,
    });

    id
}

pub fn get_source(id: usize) -> Option<SourceFile> {
    source_pool().lock().unwrap().get(id).cloned()
}

pub fn clear() {
    error_pool().lock().unwrap().clear();
    source_pool().lock().unwrap().clear();
}

pub fn error(message: &str, file_id: usize, start: usize, end: usize, help: Option<&str>,) {
    add(Diagnostic {
        severity: Severity::Error,
        message: message.to_string(),
        file_id,
        start,
        end,
        help: help.map(String::from),
    });
}

pub fn warn(message: &str, file_id: usize, start: usize, end: usize, help: Option<&str>,) {
    add(Diagnostic {
        severity: Severity::Warning,
        message: message.to_string(),
        file_id,
        start,
        end,
        help: help.map(String::from),
    });
}

pub fn info(message: &str, file_id: usize, start: usize, end: usize, help: Option<&str>,) {
    add(Diagnostic {
        severity: Severity::Info,
        message: message.to_string(),
        file_id,
        start,
        end,
        help: help.map(String::from),
    });
}

pub fn fatal(message: &str, file_id: usize, start: usize, end: usize, help: Option<&str>,) -> ! {
    add(Diagnostic {
        severity: Severity::Fatal,
        message: message.to_string(),
        file_id,
        start,
        end,
        help: help.map(String::from),
    });

    print_all();
    exit(1);
}

pub fn fatal_no_source(message: &str, help: Option<&str>,) -> ! {
    println!(
        "{}: {}",
        Severity::Fatal,
        message
    );

    if let Some(help) = help {
        println!("  Help: {}", help);
    }

    print_all();
    exit(1);
}

#[macro_export]
macro_rules! error {
    ($msg:expr, $file:expr, $start:expr, $end:expr) => {
        $crate::errors::error($msg, $file, $start, $end, None)
    };
    ($msg:expr, $file:expr, $start:expr, $end:expr, $help:expr) => {
        $crate::errors::error($msg, $file, $start, $end, Some($help))
    };
}

#[macro_export]
macro_rules! warn {
    ($msg:expr, $file:expr, $start:expr, $end:expr) => {
        $crate::errors::warn($msg, $file, $start, $end, None)
    };
    ($msg:expr, $file:expr, $start:expr, $end:expr, $help:expr) => {
        $crate::errors::warn($msg, $file, $start, $end, Some($help))
    };
}

#[macro_export]
macro_rules! info {
    ($msg:expr, $file:expr, $start:expr, $end:expr) => {
        $crate::errors::info($msg, $file, $start, $end, None)
    };
    ($msg:expr, $file:expr, $start:expr, $end:expr, $help:expr) => {
        $crate::errors::info($msg, $file, $start, $end, Some($help))
    };
}

#[macro_export]
macro_rules! fatal {
    ($msg:expr, $file:expr, $start:expr, $end:expr) => {
        $crate::errors::fatal($msg, $file, $start, $end, None)
    };
    ($msg:expr, $file:expr, $start:expr, $end:expr, $help:expr) => {
        $crate::errors::fatal($msg, $file, $start, $end, Some($help))
    };
}

#[macro_export]
macro_rules! fatalns {
    ($msg:expr) => {
        $crate::errors::fatal_no_source($msg, None)
    };
    ($msg:expr, $help:expr) => {
        $crate::errors::fatal_no_source($msg, Some($help))
    };
}

pub fn has_errors() -> bool {
    error_pool()
        .lock()
        .unwrap()
        .iter()
        .any(|d| d.severity == Severity::Error)
}

pub fn print_all() {
    for diagnostic in get() {
        let Some(source) = get_source(diagnostic.file_id) else {
            continue;
        };

        println!(
            "{}: {}",
            diagnostic.severity,
            diagnostic.message
        );

        println!(" --> {}", source.name);

        // Find the line containing the error.
        let start = diagnostic.start;
        let end = diagnostic.end;

        let line_start = source.source[..start]
            .rfind('\n')
            .map_or(0, |i| i + 1);

        let line_end = source.source[end..]
            .find('\n')
            .map_or(source.source.len(), |i| end + i);

        let line = &source.source[line_start..line_end];

        let line_number = source.source[..line_start]
            .matches('\n')
            .count() + 1;

        println!("{:4} | {}", line_number, line);

        let prefix = &source.source[line_start..start];

        let underline_length = source.source[start..end]
            .chars()
            .count()
            .max(1);

        println!(
            "     | {}{}",
            " ".repeat(prefix.chars().count()),
            "^".repeat(underline_length)
        );

        if let Some(help) = diagnostic.help {
            println!("     = help: {}", help);
        }

        println!();
    }
}