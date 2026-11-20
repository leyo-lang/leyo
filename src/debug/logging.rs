use std::sync::{Mutex, OnceLock};

static FILE: OnceLock<Mutex<Option<std::fs::File>>> = OnceLock::new();

pub fn init_log(path: &str) -> std::io::Result<()> {
    let file = std::fs::File::create(path)?;

    *FILE.get_or_init(|| Mutex::new(None)).lock().unwrap() = Some(file);

    Ok(())
}

pub fn write_log(message: &str) {
    use std::io::Write;

    if let Some(file) = FILE.get() {
        if let Some(file) = file.lock().unwrap().as_mut() {
            let _ = writeln!(file, "{message}");
        }
    }
}

pub fn write_to(section: &str, message: &str) -> Result<(), std::io::Error> {
    use std::io::Write;

    let mut file = std::fs::File::create(format!("leyo_debug/{section}.log"))?;
    writeln!(file, "{message}")?;
    Ok(())
}
