### Added — `--for` names the members of its shown owner rows on the compact route (`mem=`)

On a conceptual `--for` answer (root `bundle="compact"`), a shown class, struct or interface/trait row now carries
`mem="a,b,…"`: up to 8 names of its direct members by the index (a def whose scope names the owner, such as a Rust `impl` or a
C++ out-of-line method, or whose span lies inside the owner's, such as a C++ in-class declaration or a JS/TS/Python method; never a
closure nested in a method), the names holding the most question words first, then by line. `mem_more="N"` counts the member names
it did not list. A name already shown as a row, and the owner's own name (a constructor), are not repeated. No list on an owner
whose text the answer already carries (the body-tail fold or the body slot serves its body, or the fold served its member slice),
nor on an owner whose name another indexed class shares (its members' scope cannot say which one they belong to). The names are
spliced after the rows are chosen, so they never cost a ranked row, and `est_tokens` prices them. Not under an explicit
`--token-budget`, `--json`, `--signatures-only`, `--auto-bodies`, `--detail=N`, the name-exact route, the `--limit=N` file page,
`--expand` or the map; `RIPWIRE_NO_MEMBER_NAMES=1` turns it off. CLI only, as the fold. Measured on the 163 stored round-5 argv that
may be run (handlebars, helix and waybar never run; base lane/body-fold b112e8ac, frozen round-5 grader): Rule C ✓ 124 → 128
(mdbook-02, nunjucks-02, nunjucks-12, starlette-13), legacy ✓ 149 → 149, 0 items lost under either grading; the 60 `--for` answers
grow +332 B median (max +1,340 B), every other answer is unchanged. Gate: `test/formembernamescheck.sh`.
