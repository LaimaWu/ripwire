### Added — `--affected` lists tests that construct a reached class (class-new backstop)

A test that builds a class and then drives it through an interface-typed helper (`runTest( () => new TrieRouter() )`, then
`router.add( … )` inside the helper) is a test a reviewer checks, but the call walk cannot prove the dispatch, so `--affected`
never listed it. `--affected` now adds a separate, hedged group after the proven rows: `<g via="class-new" total= shown= capped= next=>`
with one `<new p= l= class=/>` row per test file that constructs a class owning a reached method (or the changed class),
naming the construction line and the class (`l=` only when exact: in TS/JS the earliest construction whose callee the import resolver pins to that class — a named, default or `require` binding, or a namespace member — never one that merely shares its name, and absent when none resolves, e.g. through a re-export barrel, a tsconfig alias or a CommonJS export (`module.exports = { … }`, `exports.X = …`), which the export table does not record; elsewhere absent when two constructions of one name are written differently). The group is never counted in `tests=`/`reached=`, an already-listed test is
never repeated, a class built only in non-test code adds nothing, and more than 8 rows cap with `next=--callers=…`. The
group and its legend clause are absent when empty, so every other answer is byte-identical. Construction is read from call
edges the parser already records: `new T(` (JS/TS, Java, C#), `T(` (Python, Kotlin, C++) and Rust `T::new` (a new
`constructorMethod` column in the per-language facts table). Go `T{…}` literals, C++ `T x;` declarations and Ruby `T.new`
are not recorded as constructions, so they add no row. On the in-sample hono checkout, `--affected=src/utils/url.ts` now
names `src/router/trie-router/router.test.ts` (via `new TrieRouter()` at line 6); `tests=` stays 68. Gate:
`test/affectedcheck.sh` section (9).
