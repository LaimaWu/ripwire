### Fixed — a symbol written the way the source spells it now resolves: `Class.method` in TypeScript, `json.dump`, `batcheval.Put`, `ns::Outer::Inner::f`

Every verb that takes a symbol (`--callers`, `--callees`, `--path`, `--impact`, `--connect`, `--expand` and the MCP twins)
answered "symbol not found" for three spellings that name exactly one definition: a TypeScript/JavaScript method as
`Class.method` (those grammars store no owner scope, so the owner is now read from the enclosing class), a module, crate or
package prefix (`asyncio.run`, `json.dump`, Go `batcheval.Put` and `batcheval.Cmd.Eval`, Rust `crate::buf::Buffer::edit` or
`my_crate::Buffer::edit`; in Python and Rust every prefix component must name a path component, in order, and a skipped path component is the re-export, in
Go the prefix names the file's own directory and nothing else, in Swift, Kotlin, Java and C# it names directories only: a file there
is never a namespace), and a C++ owner the index keeps only the innermost label of
(an out-of-line `ConnectionManagerImpl::ActiveStream::decodeHeaders`; read from the source's namespaces and written qualifier,
and the written prefix must be the tail of that chain). The new rule runs only after every existing spelling rule found nothing,
so a symbol that resolved before resolves exactly as before. It binds only when the spelling names one definition (a declaration
beside its definition on the same owner chain counts as one); two same-named classes, a prefix two crates both satisfy, an
import alias (`be.Put`), a type alias, a module no path names, a definition nested in a function, method, object literal or
class expression, a spelling a TypeScript `namespace` member or an object-literal member could carry (`Zone.f`, `NSX.Inner.act` and `Api.run` are refused even
when a `Zone.ts`, `NSX.ts` or `Api.ts` has its own; an object-literal member only counts against a spelling whose head is a file or directory name), and a C++ chain it cannot read, and a C++ definition inside an unnamed namespace (`namespace A { namespace { struct C {...}; } }`, which `A::C` also names) keep the "not found" answer. Known floors, all refusals: a C++ file with a
scope-macro line at namespace scope (`FOO_REGISTRATION(x);`) is unreadable, so `HttpConnectionManager::Utility::createSingletons`
(envoy) still answers "not found", and so does `Router::Filter::decodeHeaders` (envoy), because a same-named
`Filter::decodeHeaders` in a macro-line file makes the spelling unknown; an anonymous or inline namespace in the middle of a
C++ chain; a TypeScript interface and an unrelated same-named class in another file bind together as a disclosed `defs=2`. An object literal named like the class (`const Widget = { run() {} }` beside `class Widget { run() {} }` in another file) is not seen: the class method binds. Measured
on the 15 failing listed argvs of the round-6 tables: of the 11 that name one definition, 6 now resolve (one Go, two Python, three
TypeScript, one of them a `--connect`), 2 stay refused behind the C++ macro-line floor, and the 3 that sat in Rust files the grammar
failed to parse now resolve through the Rust precise-capture re-parse (zed `Pane::save_item`, `Editor::backspace`, `Workspace::new_local`). Gate: `test/writtenspellingcheck.sh`.
