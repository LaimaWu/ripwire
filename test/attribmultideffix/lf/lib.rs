// lang-facts pins: a Rust trait requirement is a contract, a definition of its own.
trait Bar { fn baz(&self); }
struct S;
impl Bar for S { fn baz(&self) { } }
fn use_bar(b: &dyn Bar) { b.baz(); }
