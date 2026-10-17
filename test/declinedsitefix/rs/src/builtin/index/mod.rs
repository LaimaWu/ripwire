use crate::preprocess::{Book, Preprocessor, PreprocessorContext};

pub struct IndexPreprocessor;

impl Preprocessor for IndexPreprocessor {
    fn name(&self) -> &str {
        "index"
    }

    fn run(&self, _ctx: &PreprocessorContext, book: Book) -> Book {
        book
    }
}
