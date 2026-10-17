pub struct Book { pub items: Vec<u32> }
impl Book {
    pub fn iter(&self) -> std::slice::Iter<'_, u32> { self.items.iter() }
}
pub struct Shelf { pub books: Vec<Book> }
impl Shelf {
    pub fn iter(&self) -> std::slice::Iter<'_, Book> { self.books.iter() }
}
pub fn count_items(b: &Book) -> usize { b.iter().count() }
pub fn count_books(s: &Shelf) -> usize { s.iter().count() }
impl Book {
    pub fn walk(&self) -> usize { self.items.len() }
}
impl Shelf {
    pub fn walk(&self) -> usize { self.books.len() }
}
pub fn walk_items(b: &Book) -> usize { b.walk() }
pub fn walk_books(s: &Shelf) -> usize { s.walk() }
