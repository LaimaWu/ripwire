### Added — calls through an interface-, trait- or abstract-typed receiver name the implementors that can run

A call such as `r.render( 2 )` on `r: &dyn Renderer`, `p.pack( t )` on `p: Packager` or `command_runner_->StartCommand( e )`
on a `std::unique_ptr<CommandRunner>` field used to answer by NAME: every in-tree definition of `render`, including the
implementor of a different `Renderer` trait in another module and an overload of another arity, or a `prov="split"` edge to
each override. A tier of the resolver now reads the receiver's recorded type first. When it is an interface, a trait, or a
class that declares the callee without a body, the candidates are the methods of that name its implementors own — the
`implements` / `extends` / `impl Trait for X` types, at any depth — plus the type's own bodied default:

- **one candidate**, every `implements` edge to it proven, and no implementor possible without a clause: an edge rendered
  `via="impl"` (the only implementor in the tree; one outside it, or one a macro writes, may exist);
- **two or more**: the call is declined with exactly that list, a `<dc ... cands="K" via="declined" gate="iface"/>` row
  (`--legend=full` names the candidates with `cand=`). It is never an edge and never in `count=`.

Receivers read: Rust `&dyn T`, `Box<dyn T>`, `impl T`, `&impl T` parameters, `let x: &dyn T`, a struct field
`Box<dyn T>` (through `self` or a typed local), and a generic parameter bounded by one trait (`R: T` inline or in a `where`
clause; marker traits such as `Send` or `Clone` beside it are ignored), whose rows also carry `bound="R: T"`. C++ a
pure-virtual member through a base pointer, reference or field (a `std::unique_ptr` / `std::shared_ptr` field through `->`).
Java, Kotlin and TypeScript a typed parameter, local or field whose type is an interface or an abstract class. A Rust
`impl Trait for X` is now proven onto the trait its header names when the written path or the file's `use`s resolve it, and
a TypeScript `implements I` onto the interface of the module its import names, so `--lego` lists fewer by-name implementors.

Not read (floors): a Go interface value — Go interfaces are structural and nothing in the source names an implementor (a
method-set cover would); a Rust `for` loop variable over a collection of trait objects (`for p in self.preprocessors.values()`);
a Rust `dyn A + B`, `Option<Box<dyn T>>`, `Rc<dyn T>` / `Arc<dyn T>`, a bound with two non-marker traits; a standard-member
name (`clone`, `iter`) on a trait object, which stays with the standard-member gate; a Python ABC or Protocol; a C# or Swift interface (their written types resolve to no class yet); a TypeScript
interface reached through a re-export barrel. A type with no implementor in the tree, or with an
implements clause the index cannot place (`impl T for &X`, a blanket `impl<U> T for U`, two same-named implementing
types), keeps the answer it had; a single candidate that is not proven does too. A C++ subclass reached through a field
never makes a one-implementor edge, and one subclass of a local or parameter keeps its proven edge unlabelled.

Exact or no row: a type an implementor can join with no clause the index reads keeps the answer it had (the by-name rows) —
a Java or Kotlin interface or class with an anonymous class or object expression in the tree (`new T() { … }`,
`object : T { … }`), a Kotlin `fun interface`, a Java interface with one abstract method (a lambda implements it; the
`equals` / `hashCode` / `toString` it redeclares do not count, and an unrelated same-named interface in another package
does not make it two), a Kotlin interface with one abstract method in a tree that holds Java (a Java lambda converts it), and a
Rust trait under a mock attribute (`#[automock]` in any `cfg_attr` spelling writes `MockT`). TypeScript types structurally
(an object literal or a class with the right members conforms unnamed), so one TypeScript candidate is never an edge, and
a TypeScript `gate="iface"` row's `cands=K` is a floor. A Rust candidate is a method inside an `impl T for X` block or T's
own default; an inherent method of X never is, and a supertrait's method keeps the by-name answer. Still unseen: an
implementor another macro writes (`mock!`, a `macro_rules!` that expands to `impl T for X`, gMock's `MOCK_METHOD` with no
default above it); a `via="impl"` edge may miss it.

A decline replaces by-name rows, and those rows sometimes named the implementor that runs. On ninja-build/ninja,
`command_runner_->StartCommand` (3 implementors) and `file_reader_->ReadFile` (4) are now `gate="iface"` declines:
`--path=Build,Start` no longer routes through the subprocess runner, and the right `ReadFile` is named only under
`--legend=full` (`cand=`). The qualified-member-name and caller-hop entries measured their rows without this tier.
