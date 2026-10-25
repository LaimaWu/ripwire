### Fixed — `--lego` stops listing an implementor whose own file imports its base from outside the tree

`--lego` no longer lists a class as an implementor of an in-tree type when the class's own file imports its base from
outside the tree: `class Sub(ext.Base)`, `from ext import Base`, `import { Base } from 'lib'` / `lib.Base`, `use ext::Base;
impl Base for S`, Java/Kotlin `import org.ext.Base` / `extends org.ext.Base` (a package the tree does not declare). A star or
wildcard import, an import the tree cannot place, an import of the tree's own package name (Node self-reference, also on a
multi-root run; a single-root Python repo whose root is the package, by its directory or `[project] name`) and a same-file
declaration still keep the by-name row, and a clause dropped as outside adds no `implementors_floor=`.
For Rust the decision is inverted: a `use` head is decided OUTSIDE only when it is `std`/`core`/`alloc`/`proc_macro`/`test`, or
a registry or git dependency that some Cargo.toml of the tree declares (any dependency table, `[workspace.dependencies]`
included; the key and its `package =` name; a `path =` crate or a key `[patch]`/`[replace]` names is not one), also through
an alias the file itself spells (`use serde as sd;`, `use std::io::{Write as _};` moves no other head). A head that is a
crate of the tree keeps its row: a Cargo `[package]`/`[lib]` name from its tests/, examples/ and main.rs, a module of the
using file's OWN crate (a workspace crate directory of it spelled with `_` for its `-` directory; a name that is only another
member's directory or module, `crates/foo_core/` beside a registry `foo-core = "1"`, is no module of this file's crate), a dependency key with `path =` under the indexed root (only a path puts a key in the tree)
(`core2 = { package = "foo-core", path = "../core" }`, also a `[dependencies.core2]` table or a dotted key), a leading `::`,
an `extern crate foo_core as fc;` or `use foo_core as fc;` the file spells. EVERY other head — an unknown crate, an alias or
re-export this scan did not see (`pub extern crate foo_core as fc;` in lib.rs, `use fc::T` from another file), a rename only
`[patch]`, `[replace]` or `.cargo/config` names, a `path =` crate outside the indexed root — is undecided: it lists no row
and the interface carries `implementors_floor=`, never a confident `implementors="0"`. A tree with no Cargo.toml decides
nothing outside but `std` and its siblings. A key that two members of the tree mean differently (in the tree for one, a registry
crate for another, also when it spells a package name of the tree) and a key a `.cargo/config(.toml)` `[patch]` names — read in
each manifest's directory and above it, within the indexed root, so a config ABOVE the indexed root is not read — are undecided
too, as is a renamed key with no `path =` (`fc = { package = "foo-core", version = "1" }`: a registry crate, or a `[patch]`'s), an
inline table that does not close on its line, and a key one manifest writes as a registry crate in one dependency table and as a
`path =` crate in another; a Cargo file the run cannot read decides nothing outside. This does NOT
cover a C++ `ext::Base` or `using ext::Base`, a C# `using`, a Swift `import`, a PHP `use`, a JS `require` destructure or a
Rust `impl ext::Base for S` with no `use`: those rows are still listed by name. Gate: `test/legotransitivecheck.sh` (XP).
