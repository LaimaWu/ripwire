use crate::preprocess::{Book, Preprocessor, PreprocessorContext};

pub struct MDBook {
    pub root: String,
    pub preprocessors: Vec<Box<dyn Preprocessor>>,
}

fn preprocessor_should_run(root: &str) -> bool {
    !root.is_empty()
}

impl MDBook {
    pub fn preprocess_book(&self, book: Book) -> Book {
        let ctx = PreprocessorContext::new(self.root.clone());
        let mut out = book;
        for preprocessor in self.preprocessors.iter() {
            if preprocessor_should_run(&self.root) {
                out = preprocessor.run(&ctx, out);
            }
        }
        out
    }
}
