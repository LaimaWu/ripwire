# receiverevidencefix — calls bound by name alone (test/receiverevidencecheck.sh)

Each directory is its own root (the gate indexes one at a time). Every root holds member calls (or, in an
implicit-receiver language, bare calls) whose receiver nothing proves to be the class of the same-named
definition beside them, plus near-miss calls whose receiver IS proven and whose edges must stay plain.

- `js/` — `ctx.onerror()` (plain and optional-chained) in the file that defines `Application.onerror`; a
  constructor-assigned `this.bucket` (a `Schemas`) beside the caller's own `listSchemas`; WeakMap `.set/.get`
  beside same-file accessors; Promise `.then` beside `Reply.prototype.then`; a stream's `.on` beside a test
  double; an outside router's `.all` and a Set's `.delete` beside instance shorthands; URLSearchParams `.append`
  beside `Response.append`; a `done` parameter beside another file's closure; `context.handler()` beside a free
  `handler`; an object-literal module's `this.ctx.get()` beside its own `get` (the true target is the request
  delegate's `get`); an alias `const onerror = ctx.onerror` and a destructured `const { onerror } = ctx`; the
  globals `Reflect.get` / `Atomics.add` beside in-repo `get` / `add`. Kept: `this.onerror()`, a same-file bare call, a relative module receiver, a closure called in its
  own function, `new Application()` / `new Schemas()` deciding between same-named methods, `super.listSchemas()`,
  a class-name receiver `Application.create()`, a relative module bound to the name `Reflect`, `new Reply().send()`;
  `reply.send()` / `reply.code()` on a parameter are the name-only true edges the gate wants kept AND marked.
  `serve` (via respond) and `both` (via direct and respondWith) give `--impact` its d=2 rows.
- `ts/` — a loop variable over `Router<T>[]` beside `SmartRouter.add`; WHATWG `headers.get()` beside
  `Context.get` / `Cache.get`; the global `Atomics.add`; `Date#getTime` beside an in-repo `Clock.getTime`, and the
  global `Response` beside an in-repo `Response` class. Kept: an annotated parameter, construction through an import alias, a
  constructed local.
- `py/` (src layout) — dict/set/list/str receivers in files that import `Styles` / `Content`; an argument's
  `.animate`; an asyncio handle's `.cancel`; an outside package's parser `.write`; `self._send` holding a passed
  callable beside a nested `_send`; non-self receivers in `App`'s own file (`screen` is a property);
  `from json import dumps as stringify` beside in-repo `stringify` / `dumps`; `append` on a list literal; `read = os.read` and
  `feed = parser.feed` called from a nested def; `add_widget = widgets.append` beside a nested `add_widget`.
  Kept: `self.update()`, a constructor-assigned field through the class cone, a typed parameter, construction,
  an import alias, an imported function beside same-named methods, `cls.default_rules()`, `Styles.copy(x)`, a
  constructed `Content`'s `append`, the
  `XTermParser.feed` the alias really reaches; `pump.call_later()` on a parameter is kept and marked.
- `go/` — an item's typed `text util.Chars` field, a typed parameter, a typed local through an aliased import
  and an embedded field, all beside `Merger.Get/Length` in the caller's package; an outside `tcell.Screen`'s
  `Size` and an outside value's `Runes` beside a renderer's methods; a field / local named `log` of an outside type
  beside the in-repo `log.Errorf`; exec's `Cmd.Output` beside an in-repo `Output` (with `fmt.Errorf` and
  `strings.Split`). Kept: `m := &Merger{}`, the method receiver itself, a real import of the in-repo log package,
  a constructed in-repo `Tool`.
- `java/ kt/ cs/ cpp/ swift/ rb/` — a bare call inside a class whose base is outside the tree (or whose name
  comes from an outside import) beside an unrelated class's method of that name. Kept: own members (private
  too), an in-repo superclass's member (and Java `super.flush()`), a free / top-level function, a Ruby top-level
  def; a Ruby included module's method is resolved or hedged (the contract only: Ruby's own lookup is separate work).

Sides of a class lookup (js/lib/sides.js, ts/src/sides.ts, kt/src/app/Sides.kt): a call on the class object reaches
its static (companion) members, a call on an instance the others. A fallback hook only after a miss
(py/src/tui/ghost.py): `__getattr__` leaves a defined member's call resolved.

The names are paraphrases of graded false rows; the code is minimal and is never built.
- `scope/{java,cs,kt,swift,ts,js,py,go}` — review B4: Tank and Barrel both define `spill`, and the class's field (or the
  function's outer parameter / local) named `tank` is a Tank. Each method binds `tank` again — a loop, lambda, catch,
  resource, pattern, `case`, `out var`, destructuring, `if`/`guard`/`while let`, `with`/`except`/walrus/`match`, a Go
  range / if / type-switch variable, a reassignment — and calls `tank.spill()`, which must never prove Tank's spill;
  `nestedBlock` / `lambdaOutside` / `blockOutside` call it past the block that hid it, which must never prove Barrel's. A
  generic whose type parameter is spelled `Tank` (method and class, a field of that type) must never prove class Tank.
  Kept: the field itself, a typed local, a typed loop / lambda variable's own class, a typed parameter.
- `superroot/{py,js,ts,go,java,kt,swift,cs,cpp,rb}` — CR 5469474915: `Panel` extends `Base`; `Base` and `Widget` both
  define `render`. `typed` binds a local spelled `base` (C#: `super`) to a `Widget`, `untyped` to a value nothing types
  (Python and Go also a local `super`), then calls `render` on it: never Base's render through the bases-only lookup.
  Kept: `super.render()`, Python `super().render()`, Go's promoted `p.Render()`, the typed local's own class. C#
  `base.Render()` is visible only (no C# tree reads `base` as a receiver root yet).
- `callresult.txt` — arm (L), call-result typing: six source trees (`go`, `rust`, `cpp`, `java`, `kt`, `ts`) in ONE text
  bundle that the gate unpacks into its temp dir (committed as source trees they would join the repo's own indexed map): a
  local initialised by a call to an in-tree function whose DECLARED return type names an in-tree class (`Config`), beside a
  same-named method on another class (`Other`), so the call on the local is name-only until the binding takes the callee's
  return type. Shapes that RESOLVE: Go `(T, error)` with named results, `(*T, error)`, `*T`, a positional pair, a method
  callee, a reassignment, the package func `model.Get` beside the method `Config.Get`; Rust `Result<T,_>` + `?`,
  `Option<T>` + `?`, `Box<T>`, `&T`, `T`, a tuple element, an associated fn `Book::load(..)?`, a `self` method callee,
  `.unwrap()`/`.expect()`; C++ `T`, `const T&`, `T*`, `std::unique_ptr<T>`, a method callee; Java `var`, Kotlin
  `val`/`var`, TS `const` and `await`; and a CHAIN (`h = make(); c = h.get(); c.m()`, every language). Shapes typed but
  proving NO member (floors, not resolutions): a Go INTERFACE result (its bodyless signature is the ladder's) and a C++
  `using` alias result (Rule 2 reads no member through an alias). Near misses (must stay unproven): two-def / overloaded
  callees, outside callees, wrappers outside the list (Go `Ref[T]`, Rust `Vec<T>` / `HashMap`, C++
  `std::reference_wrapper`, Java `Optional<T>`), type-parameter results, Go/Rust aliases, no written return type, a
  shadowing inner binding; fix round 1: a nested same-named call (`h.get().get()`), an outside import or qualifier naming an
  in-tree namesake (Java/Kotlin `import java.io.File`, TS `ext-config` / `import type` / the global `Response`, Rust `use
  std::fs::File`, C++ `ext::Config`), an interface in one Go package beside a same-named struct in another, a commit by name
  alone (`ext::onlyOne()`) or by a written type's last name (`ext::Factory f`), a Go field `f *os.File`, and a chain hop that
  binds by name alone or splits. `UseWritten`/`useWritten` twins write the type (green before the lane).
  Fix round 3: `rust3` / `cpp3` / `java4` / `kt4` are the delta review's D4–D6 shapes (a Rust inline `mod` scope, a C++
  alias / typedef, a JVM on-demand import — receiver and result).
  Fix round 4: `java5` / `kt5` / `cpp4` / `rust4` are the delta-3 review's E1–E3 probes (a Java nested `record` /
  `@interface` / single-static-import; C++ union / forward declaration / macro / block-scope using beside a local class;
  a Rust macro in an enclosing fn body) with their guards (a nested enum, Kotlin's nested forms, a forward declaration of
  the in-tree class, a local class with nothing in its block, a nested fn with no macro).
  Fix round 5: `cpp5` / `rust5` are the delta-4 review's B1–B4 probes — macros the parser reads as well-formed code (a C++
  export macro between the class-key and the name, a class-body `MK(S);`, a function-body macro beside a local class; a
  Rust `impl` method in a fn body holding a macro) — with guards (constructors / a destructor, a function-body macro with
  no local class, the own-namespace forward declaration reached through a using-declaration).
  Fix round 6: `rust6` is the delta-5 review's B3 — an `impl` in a Rust `const` / `static` item's value block (a macro or
  `use` there decides), with a guard (a const block with neither). C++ call-result typing is OFF (model.h
  `callResultFamilyOf`): the C++ trees stay, their [P] arms and the matrix's C++ U cells are FLOORS that must hedge.
- `callresult_matrix.txt` — arm (L)'s BINDING-FORM MATRIX (Python, kept as text for the same reason): it writes one tree per
  language (`mx_rust`, `mx_cpp`, `mx_java`, `mx_kt`, `mx_ts`, `mx_go`) and lists every cell — language × binding form ×
  scope level × target (in-tree unique / outside / unresolved / two in-tree), and (fix round 4) × declaration kind
  (recorded class / unrecorded type-binding node / macro-unparsed) — with its expected verdict, plus the N/A rows a
  language cannot write. Its header defines the cells.
- `rsrecv.txt` — arm (RS), Rust receiver evidence (lane rust-receivers): one Rust crate in a text bundle (unpacked like
  `callresult.txt`). `model.rs` defines Book / Other (both `spin`, `iter`), Cfg / Knob (both `tune`), a generic Stack, and
  Shelf / Inner / Holder / Pair whose fields are typed Book, `Box<Book>`, `Option<Book>`, `Rc<Book>`, u32, `T` and a tuple
  field; `other.rs` a second Book, an in-tree File and a same-named Shelf whose `book` is a Vec; `kinds.rs` an enum with
  `spin` and a trait with a default `spin`; `app.rs` one function per receiver shape; `nest/` and `glob2/` a glob of a
  module's own `use` and a glob of a glob.
