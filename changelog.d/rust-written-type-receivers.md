### Added — Rust receivers typed by their written type: parameters, `let` bindings and struct fields

A Rust member call used to bind every in-tree method of its name, hedged `via="name"` (or, for a standard-member name such
as `iter` or `push`, was declined), because Rust recorded no parameter, local or field type. Now a receiver written as a
type proves that type's method: a parameter (`book: &Book`, `&mut T`, `Box<T>`, a path such as `crate::model::Book`, a
user generic `Stack<u8>`), `let x: T`, a struct literal `T { .. }`, `T::default()`, a closure's typed parameter, and a
field path below such a receiver or below `self` (`self.book.iter()`, `self.inner.book.spin()`), each field by its own
written type. The written name resolves exactly as a declared call result does (the item scopes, `use` declarations —
a `use … as` alias by the path it names, a glob `use super::*` through the module's own `use` — and module paths in force
at the binding), so `use std::fs::File` beside an in-tree `File` proves nothing. A declared `-> Self`
result now types its call too (`let b = Book::new(); b.spin()`) when the call's qualifier, resolved the same way, names
that in-tree type: `fs::File::open()`, `std::path::PathBuf::from()`, a glob's or the prelude's `String::new()` or an
external crate's glob reach an in-tree namesake only by name and type nothing. The same holds for a result written by
the type's own name (`-> PathBuf`, a path, `Result<PathBuf, _>` / `Option<..>` / `Box<..>`): a qualified `T::f()` whose `T`
leads out of the tree types nothing — `std` / `core` / `alloc` (a glob brings such a root only when its module declares
one, so `mod tests { use super::*; }` never does, and a glob cycle ends with that same answer; a named `use` of the name
shadows that module's globs, as in Rust), an external crate, a `use` or glob re-export of one, an inline module's re-export of
one, cfg-gated duplicate `use`s that all lead out — while a `T` the walk cannot follow (cfg-gated `use`s that disagree,
an unparsed region of the file, a file outside every crate's module tree) keeps the earlier call-result rule; a re-export
chain longer than the walk's depth cap is cut, and a cut is hedged, never proven onto an in-tree type, whether it falls in a glob's chain or in the written path itself (an in-tree twin cut the same way is hedged too). A generic result's head names its class. On
rust-lang/mdBook, `--callers=Book::iter` now lists `make_data` (`book: &Book`) and `MDBook::iter` (`self.book.iter()`)
as plain edges. Stated floor (still declined or hedged): a generic bound `T: Trait`, `dyn` / `impl Trait` and a trait
written as a type (which implementation runs is dispatch), a `type` alias, a glob of a module that only globs, a value behind `Option` / `Result` / `Rc` / `Arc` / `RefCell` /
`Cell` / `Mutex` / `RwLock`, a tuple-struct field (`self.0`), and any pattern other than a plain name. A macro that
rebinds a name (`shadow!(b); b.spin()`) is not read: the written type still types the name after it. The wrapper list is
one data row per family (`Box` transparent; `Option` / `Result` one unwrap layer; the pointers and cells hide their argument).
