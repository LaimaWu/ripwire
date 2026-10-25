### Fixed — `--whereis` no longer labels comment, prose and string lines `kind="def"` on the lexical fallback

When the parsed index has no definition of the name (or the checkout drifted from it), HEAD rows are labelled by a lexical shape
test (`head_labels="lexical"`). That test took a declaration word anywhere earlier on the line, so `// a class published its
name`, a help string `"struct (like --lego)"`, a markdown line and every `struct T *data` local read `kind="def"`: 241 false
definition rows over 30 cells (10 words with no definition × this repo, hono, tmux). Now the word must sit directly before the
name (qualifiers, `*`/`&`, template or generic arguments, a qualification chain, a Go receiver, one ALL-CAPS export macro after
`class`/`struct` may sit between; `template<…> RetType NAME(` too); a line that opens a comment (`//`, `#`, `;`, `-- `, ` * `,
`"""`…), a name after a trailing comment opener and a name inside a `"…"`/`` `…` `` literal are never definitions; `#define NAME`
(object-like as well as function-like) is; a typedef defines only its own last declarator; `struct T` followed by a declarator,
`*`, `)` or `,` uses the tag and defines nothing; `using std::x;` and `using namespace x` define nothing (`using X = …` does), nor does Rust's `impl Trait for X`. The
root gains `head_defs="0"` when the lexical labels stand and the index defines no symbol of the name at all. Still a floor in both
directions, and named as one: prose whose declaration word sits right before the name reads as a definition, and an unusual
declarator (a wrapped signature, an enumerator, a field) reads as a reference. The parse chooser for other refs' blobs keeps the
old, looser reading as well, so no blob that used to be parsed goes unparsed. Control: 12 real definitions forced onto the lexical
path (C, C++, TS, Python, Rust, Go, Ruby) keep their `kind="def"` rows. Gate: `test/honestlexcheck.sh` (A, H arms).
