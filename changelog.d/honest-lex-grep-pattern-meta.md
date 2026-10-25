### Fixed — `--grep` says when a literal pattern reads as a regex, and prints one row per line

A literal `--grep='compose|HTTPException'` answered `hits="0" complete="1"` with `next=` the conceptual lens, while 31 hono
files hold one of the names; `--grep='CHANGELOG\.md'` found the one line spelling the backslash. The root now carries
`pattern_meta="…"` (the regex-shaped pieces: `|` between words, a backslash escape, `.*`/`.+` inside the pattern, a leading `^`
or trailing `$`, a `[^…]`/`[a-z]` class; a lone `.` as in `CHANGELOG.md` is not one) and `next=` is `--regex=<pattern>`
whatever the hit count (a basic-grep `a\|b` is offered as `a|b`; nothing is offered when `--regex` would refuse it). The literal
answer itself, and its `complete=`, are unchanged: the scan still matched the text as written. Folded in: a line holding the
pattern twice used to print as two `<hit>` rows and count twice; it is now one row with `occ="2"`, and `hits=`, `shown=` and the
window count lines, not occurrences (the unindexed list too). The MCP `grep` verb carries `pattern_meta`, the same `next` and
`occ` per row. Gate: `test/honestlexcheck.sh` (B, M arms).
