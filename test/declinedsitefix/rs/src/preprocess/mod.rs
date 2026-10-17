pub struct Book {
    pub chapters: Vec<String>,
}

pub struct PreprocessorContext {
    pub root: String,
}

impl PreprocessorContext {
    pub fn new(root: String) -> PreprocessorContext {
        PreprocessorContext { root }
    }
}

pub trait Preprocessor {
    fn name(&self) -> &str;
    fn run(&self, ctx: &PreprocessorContext, book: Book) -> Book;
}
