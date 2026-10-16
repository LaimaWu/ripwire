use crate::model::{self, Book, Config};

// use_try: `let x = f()?` through Result<T, _>.
pub fn use_try(path: &str) -> Result<u32, model::Error> {
    let c = model::load(path)?;
    Ok(c.get("a"))
}

// use_option: Option<T> with `?`.
pub fn use_option() -> Option<u32> {
    let c = model::find("k")?;
    Some(c.get("b"))
}

// use_box: Box<T>.
pub fn use_box() -> u32 {
    let c = model::boxed();
    c.get("c")
}

// use_ref: &T.
pub fn use_ref(base: &Config) -> u32 {
    let c = model::borrowed(base);
    c.get("d")
}

// use_plain: T.
pub fn use_plain() -> u32 {
    let c = model::make();
    c.get("e")
}

// use_tuple: the second element of a tuple result (mdbook's `let (book, _) = self.preprocess_book(..)?`).
pub fn use_tuple() -> Result<u32, model::Error> {
    let (_, o) = model::pair()?;
    Ok(o.get("f"))
}

// use_path: an associated function on an in-tree type (`MDBook::load(..)?`).
pub fn use_path() -> Result<u32, model::Error> {
    let book = Book::load("root")?;
    let c = book.build();
    Ok(c.get("g"))
}

impl Book {
    // use_self: the callee is a method on self.
    pub fn use_self(&self) -> Result<u32, model::Error> {
        let (c, _) = self.preprocess()?;
        Ok(c.get("h"))
    }
}

// near_external: an outside callee.
pub fn near_external() -> usize {
    let s = String::new();
    s.len()
}

// near_std_get: a std container result whose method shares an in-tree name.
pub fn near_std_get() -> Option<u32> {
    let m = model::table();
    m.get("k").copied()
}

// near_wrapper: Vec<T> is outside the list: `v.get(0)` is the slice's get.
pub fn near_wrapper() -> usize {
    let v = model::configs();
    v.len()
}

// near_two_defs: the callee binds two definitions (an untyped receiver).
pub fn near_two_defs<F>(f: F) -> u32 {
    let x = f.produce();
    x.get("i")
}

// near_alias: a Rust alias emits no alias record today.
pub fn near_alias() -> u32 {
    let a = model::alias();
    a.get("j")
}

// near_generic: a type-parameter result.
pub fn near_generic() -> u32 {
    let g = model::generic::<Config>();
    g.get("k")
}

// near_shadow: a later `let` rebinds the name to an outside value.
pub fn near_shadow() -> usize {
    let c = model::make();
    let c = String::from("x");
    c.len()
}

// use_written (twin): the same call with the type written.
pub fn use_written() -> u32 {
    let c: Config = model::make();
    c.get("w")
}

// use_unwrap: `.unwrap()` / `.expect(..)` on a Result/Option result (the test-suite shape: `MDBook::load(..).unwrap()`).
pub fn use_unwrap(path: &str) -> u32 {
    let c = model::load(path).unwrap();
    let d = model::find("k").expect("present");
    c.get("u") + d.get("v")
}

// near_qualified: an in-tree callee returning std's File (written qualified) beside the in-tree osfile::File.
pub fn near_qualified() -> std::io::Result<()> {
    let f = crate::osfile::open_std()?;
    f.sync_all()?;
    let g = crate::osfile::open_fs();
    g.sync_all()?;
    Ok(())
}

// near_no_try: Option<T> without `?` — `r.iter()` is Option::iter, never Config::iter.
pub fn near_no_try() -> usize {
    let r = model::find("k");
    r.iter().count()
}

// near_nested: Result<Option<T>, _> with ONE `?` leaves Option<T>: `c.iter()` is Option::iter.
pub fn near_nested() -> Result<usize, model::Error> {
    let c = model::nested()?;
    Ok(c.iter().count())
}

// near_method_tail: a trailing method other than unwrap/expect is not stripped: o is Other (one level further: floor).
pub fn near_method_tail() -> u32 {
    let o = model::make().into_other();
    o.get("o")
}

// near_type_param_named: a type parameter named after the in-tree Config, instantiated with Other.
pub fn near_type_param_named() -> u32 {
    let g = model::generic2::<model::Other>();
    g.get("p")
}
