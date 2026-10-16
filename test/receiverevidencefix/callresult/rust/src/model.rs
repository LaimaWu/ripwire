use std::collections::HashMap;

pub struct Config;

impl Config {
    pub fn get(&self, key: &str) -> u32 { 0 }
    pub fn len(&self) -> usize { 0 }
}

pub struct Other;

impl Other {
    pub fn get(&self, key: &str) -> u32 { 1 }
    pub fn len(&self) -> usize { 1 }
}

pub type Alias = Config;

pub struct Error;

pub fn load(path: &str) -> Result<Config, Error> { Ok(Config) }

pub fn find(key: &str) -> Option<Config> { Some(Config) }

pub fn boxed() -> Box<Config> { Box::new(Config) }

pub fn borrowed(c: &Config) -> &Config { c }

pub fn make() -> Config { Config }

pub fn pair() -> Result<(Config, Other), Error> { Ok((Config, Other)) }

pub fn alias() -> Alias { Config }

pub fn configs() -> Vec<Config> { vec![Config] }

pub fn generic<T: Default>() -> T { T::default() }

pub fn table() -> HashMap<String, u32> { HashMap::new() }

pub struct Book;

impl Book {
    pub fn load(root: &str) -> Result<Book, Error> { Ok(Book) }
    pub fn preprocess(&self) -> Result<(Config, Other), Error> { Ok((Config, Other)) }
    pub fn build(&self) -> Config { Config }
}

pub struct Factory;

impl Factory {
    pub fn produce(&self) -> Config { Config }
}

pub struct Mill;

impl Mill {
    pub fn produce(&self) -> Other { Other }
}

// new: an in-tree namesake of String::new (an outside callee never types through it).
pub fn new() -> Config { Config }

// Config::iter (mdbook's Book::iter beside Option::iter) and a non-unwrap method returning Other.
impl Config {
    pub fn iter(&self) -> usize { 0 }
    pub fn into_other(&self) -> Other { Other }
}

// nested: one `?` strips ONE layer — the binding is Option<Config>, not Config.
pub fn nested() -> Result<Option<Config>, Error> { Ok(Some(Config)) }

// generic2 names its type parameter after the in-tree Config.
pub fn generic2<Config: Default>() -> Config { Config::default() }
