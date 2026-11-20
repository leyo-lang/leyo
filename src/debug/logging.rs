use crate::{fatal, warn};

pub fn init() {
    if std::path::Path::new("leyo_debug").exists() {
        let e = std::fs::remove_dir_all("leyo_debug");
        if e.is_err() {
            fatal!(&format!("Could Not Clean Debug Folder: {}", e.unwrap_err()))
        }
    }

    let e = std::fs::create_dir("leyo_debug");
    if e.is_err() {
        fatal!(&format!(
            "Could Not Create Debug Folder: {}",
            e.unwrap_err()
        ))
    }
}

pub fn write_to(section: &str, message: &str) {
    use std::io::Write;

    let file = std::fs::File::create(format!("leyo_debug/{section}.log"));
    if file.is_err() {
        warn!(&format!("Could Not Create Log File: {}", file.unwrap_err()));
        return;
    }

    let e = writeln!(file.unwrap(), "{message}");
    if e.is_err() {
        warn!(&format!("Could Not Write To Log File: {}", e.unwrap_err()));
    }
}
