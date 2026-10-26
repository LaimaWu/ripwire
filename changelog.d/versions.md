### Changed — the versions this change moves, stated once

`kParserVer` 206 → 231 (file tag 232): the Rust written-type receivers carried 202 (203) on their branch and the struct-field
slot join 204 (205), and its fix rounds changed what 204 extracts, so a cache from that branch would replay the older slot
rows; the interface-dispatch records carried 210 and 212 on theirs; the JS/TS written-binding records carried 218 and the
Python rebind-withholding records a 224 placeholder on theirs; train 34 took 226; the Kotlin trailing-lambda capture (#155)
carried a 228 placeholder and another branch holds 229, so 231 sits above every live branch and pull-request number. A cache
written by 0.6.8 is re-parsed once. `kCacheVersion` 36 → 43: a Rust binding typed only through a trait
(`&dyn T`, `impl T`, a bound), a Rust `impl T for X` block and a JS/TS binding written after its declaration are new binding
records, and a Kotlin call's trailing-lambda byte (#155) widens the reference record, so a cache written before them is
rejected and rebuilt (the branches that added them used 37, 38, 39 and 41). The quality-snapshot scheme moves 22 → 24 (#228: Django and
unittest hooks and closure self-dispatch change the dead set; old snapshots are not used; the branch used 23). The session legend dictionary is `dictv=8c941c6c17d27549 entries=932`.
