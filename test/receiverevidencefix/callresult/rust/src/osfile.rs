// File: an in-tree namesake of std::fs::File — a qualified outside result type never types a binding by its last segment.
pub struct File;

impl File {
    pub fn sync_all(&self) -> u32 { 0 }
}

pub fn open_std() -> std::io::Result<std::fs::File> { std::fs::File::open("x") }

pub fn open_fs() -> std::fs::File { std::fs::File::open("x").unwrap() }
