### Fixed — Windows: `--quality-delta=A..B` extracts its commit trees through stdin; two vendored scanners stop calling `isdigit` on code points (#402, thanks @lennix1337)

GNU tar on MSYS2 / Git for Windows read the colon of a drive path (`C:\…`) given to `-f` as a remote host, so materializing a
commit tree failed there; the archive now reaches `tar -x -f -` on stdin on every platform, and `test/qualitycheck.sh` runs that
arm under a colon-bearing temporary directory. The vendored tree-sitter bash and markdown external scanners called `isdigit()` on
an `int32_t` lookahead (undefined above 255, and an assertion in Microsoft UCRT debug builds); both use an explicit `'0'..'9'`
range check now, recorded as patches under `third_party/patches/`. No output change.
