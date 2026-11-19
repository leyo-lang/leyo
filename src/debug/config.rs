use crate::fatal;

#[derive(Default)]
pub struct Config {
    pub tokens: bool,
    pub newlines: bool,
    pub ast: bool,
    pub time: bool,
}

impl Config {
    pub fn all(&mut self) {
        self.tokens = true;
        self.newlines = true;
        self.ast = true;
        self.time = true;
    }

    pub fn any(&self) -> bool {
        self.tokens || self.newlines || self.ast || self.time
    }

    pub fn enable_flag(&mut self, name: &str) {
        match name.trim() {
            "all" => self.all(),
            "tokens" => self.tokens = true,
            "asi" | "newlines" => self.newlines = true,
            "ast" => self.ast = true,
            "time" | "timings" => self.time = true,
            other => {
                fatal!(
                    format!("Unknown debug flag: '{other}'").as_str(),
                    "Valid flags: all, tokens, newlines, ast, time"
                );
            }
        }
    }
}
