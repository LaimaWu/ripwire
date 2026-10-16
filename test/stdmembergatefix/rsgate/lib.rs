// The Rust row of the standard-member gate (test/stdmembergatecheck.sh arm R). Stack and Book are in-tree types whose
// methods are named like Vec / slice / HashMap members; the callers below reach them by NAME unless a rule types the receiver.
pub struct Stack<T> { items: Vec<T> }
impl<T> Stack<T> {
    pub fn push(&mut self, t: T) { self.items.push(t); }
    pub fn len(&self) -> usize { self.items.len() }
    pub fn frobnicate(&self) { }
}
pub struct Holder { inner: Stack<i32> }
impl Holder {
    pub fn add(&mut self, x: i32) { self.inner.push(x); }
}
pub struct Book { sections: Vec<i32> }
pub struct BookItems { at: usize }
impl Book {
    pub fn iter(&self) -> BookItems { BookItems { at: 0 } }
    pub fn first(&self) -> usize { let mut n = 0; for _s in self.iter() { n += 1; } n }
}
pub fn vec_param(v: &mut Vec<i32>) { v.push(1); }
pub fn stack_param(s: &mut Stack<i32>) { s.push(1); }
pub fn unknown_param(x: &mut Unknown) { x.push(1); }
pub fn stack_local() { let mut s = Stack { items: Vec::new() }; s.push(2); }
pub fn not_in_table(s: &Stack<i32>) { s.frobnicate(); }
pub fn vec_iter(v: &Vec<i32>) -> usize { let mut n = 0; for _x in v.iter() { n += 1; } n }
pub fn book_iter(book: &Book) -> usize { let mut n = 0; for _s in book.iter() { n += 1; } n }
pub fn map_insert(m: &mut std::collections::HashMap<String, i32>) { m.insert(String::new(), 1); }
pub trait Pushable { fn push(&mut self, x: i32); }
pub fn ufcs(s: &mut Stack<i32>) { Stack::push(s, 1); }
pub fn generic_bound<P: Pushable>(p: &mut P) { p.push(1); }
