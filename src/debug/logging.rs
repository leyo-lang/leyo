use crate::{fatal, warn};

pub fn init() {
    if std::path::Path::new("leyo_debug").exists() {
        let e = std::fs::remove_dir_all("leyo_debug");
        if let Err(e) = e {
            fatal!(&format!("Could Not Clean Debug Folder: {e}"))
        }
    }

    let e = std::fs::create_dir("leyo_debug");
    if let Err(e) = e {
        fatal!(&format!("Could Not Create Debug Folder: {e}"))
    }
}

pub fn write_to(section: &str, message: &str) {
    use std::io::Write;

    let file = std::fs::File::create(format!("leyo_debug/{section}.log"));
    if let Err(e) = file {
        warn!(&format!("Could Not Create Log File: {e}"));
        return;
    }

    let e = writeln!(file.unwrap(), "{message}");
    if let Err(e) = e {
        warn!(&format!("Could Not Write To Log File: {e}"));
    }
}
