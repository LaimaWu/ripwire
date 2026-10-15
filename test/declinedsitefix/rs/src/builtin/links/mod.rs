use crate::preprocess::{Book, Preprocessor, PreprocessorContext};

pub struct LinkPreprocessor;

impl Preprocessor for LinkPreprocessor {
    fn name(&self) -> &str {
        "links"
    }

    fn run(&self, _ctx: &PreprocessorContext, book: Book) -> Book {
        book
    }
}
