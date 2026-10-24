### Changed — the versions this change moves, stated once

`kParserVer` 206 → 224 (file tag 225): the Rust written-type receivers carried 202 (203) on their branch and the struct-field
slot join 204 (205), and its fix rounds changed what 204 extracts, so a cache from that branch would replay the older slot
rows; the interface-dispatch records carried 210 and 212 on theirs; 224 sits above every live branch and pull-request number
(212 to 223). A cache written by 0.6.8 is re-parsed once. `kCacheVersion` 36 → 39: a Rust binding typed only through a trait
(`&dyn T`, `impl T`, a bound) and a Rust `impl T for X` block are new binding records, so a cache written before them is
rejected and rebuilt (the branch that added them used 37 and 38). The quality-snapshot scheme stays 22. The session legend
dictionary is `dictv=a612d2cfd7867548 entries=923`.
