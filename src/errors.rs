use std::{process::exit, sync::{Mutex, OnceLock}};

#[derive(Debug, Clone, PartialEq)]
pub enum Severity {
    Error,
    Warning,
    Info,
    Fatal,
}
impl std::fmt::Display for Severity {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        let severity = match self {
            Severity::Error   => "\x1b[1;38;2;239;68;68mERROR\x1b[0m",   // #EF4444
            Severity::Warning => "\x1b[1;38;2;245;158;11mWARNING\x1b[0m", // #F59E0B
            Severity::Info    => "\x1b[1;38;2;59;130;246mINFO\x1b[0m",   // #3B82F6
            Severity::Fatal   => "\x1b[1;38;2;220;38;38mFATAL\x1b[0m",   // #DC2626
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

pub fn has_any() -> bool {
    error_pool()
        .lock()
        .unwrap()
        .iter()
        .any(|_| true)
}

fn char_width(c: char) -> usize {
    let cp = c as u32;

    match cp {
        // Combining marks (zero-width).
        0x0300..=0x036F |
        0x1AB0..=0x1AFF |
        0x1DC0..=0x1DFF |
        0x20D0..=0x20FF |
        0xFE20..=0xFE2F => 0,

        // Wide characters and common emoji.
        0x1100..=0x115F |
        0x2E80..=0xA4CF |
        0xAC00..=0xD7A3 |
        0xF900..=0xFAFF |
        0xFE10..=0xFE6F |
        0xFF00..=0xFF60 |
        0xFFE0..=0xFFE6 |
        0x1F300..=0x1FAFF |
        0x20000..=0x3FFFF => 2,

        _ => 1,
    }
}

fn display_width(s: &str) -> usize {
    s.chars().map(char_width).sum()
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

        let start = diagnostic.start;
        let end = diagnostic.end;

        let char_count = source.source.chars().count();

        if start > end || end > char_count {
            continue;
        }

        // Convert character indices to byte offsets.
        let char_to_byte = |index: usize| -> usize {
            source.source
                .char_indices()
                .nth(index)
                .map_or(source.source.len(), |(byte, _)| byte)
        };

        let start_byte = char_to_byte(start);
        let end_byte = char_to_byte(end);

        // Find containing line.
        let line_start = source.source[..start_byte]
            .rfind('\n')
            .map_or(0, |i| i + 1);

        let line_end = source.source[end_byte..]
            .find('\n')
            .map_or(source.source.len(), |i| end_byte + i);

        let line = &source.source[line_start..line_end];

        let line_number = source.source[..line_start]
            .matches('\n')
            .count() + 1;

        println!("{:4} | {}", line_number, line);

        let prefix = &source.source[line_start..start_byte];
        let highlighted = &source.source[start_byte..end_byte];

        let prefix_width = display_width(prefix);
        let underline_width = display_width(highlighted).max(1);

        println!(
            "     | {}{}",
            " ".repeat(prefix_width),
            "^".repeat(underline_width)
        );

        if let Some(help) = diagnostic.help {
            println!("     = help: {}", help);
        }

        println!();
    }
}
