### Changed — the versions this change moves, stated once

`kParserVer` 206 → 226 (file tag 227): the Rust written-type receivers carried 202 (203) on their branch and the struct-field
slot join 204 (205), and its fix rounds changed what 204 extracts, so a cache from that branch would replay the older slot
rows; the interface-dispatch records carried 210 and 212 on theirs; the JS/TS written-binding records carried 218 and the
Python rebind-withholding records a 224 placeholder on theirs; 226 sits above every live branch and pull-request number
(212 to 225). A cache written by 0.6.8 is re-parsed once. `kCacheVersion` 36 → 40: a Rust binding typed only through a trait
(`&dyn T`, `impl T`, a bound), a Rust `impl T for X` block and a JS/TS binding written after its declaration are new binding
records, so a cache written before them is rejected and rebuilt (the branches that added them used 37, 38 and 39). The
quality-snapshot scheme stays 22. The session legend dictionary is `dictv=8c941c6c17d27549 entries=932`.
