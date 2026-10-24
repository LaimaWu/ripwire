### Changed — source builds: Windows clang-cl PGO builds link, and keep SLP vectorization in the final binary (#410, thanks @lennix1337)

With `clang-cl` + `lld-link`, `RIPWIRE_PGO=generate` crashed Clang 22.1.8's backend (x86_64-v3); the instrumented `generate`
build now turns SLP vectorization off for that phase only (`/clang:-fno-slp-vectorize` on compiles, `/mllvm:-vectorize-slp=false`
on the link, where LTO code generation runs), so the `use` build keeps it. `lld-link` no longer receives the clang driver's
`-fprofile-generate` / `-fprofile-use`, which it rejects (clang-cl links the profile runtime itself). Reproduction and removal
criteria: `docs/WINDOWS-PGO.md`. No other compiler changes; no output change.
