### Security — the pattern screen refuses overlapping alternation, one group deeper

A pattern whose group carries a top-level `|` under an unbounded outer quantifier could backtrack catastrophically
on a subject it fails to match. The screen that guards every user-authored pattern surface (`--regex`,
`--graph-query file()`, `--arch` path-rules, `#match?`/`#not-match?`) already refused every unbounded quantifier over
a group containing a quantifier, but it passed a group whose branches merely *overlap* — the classic `(a|aa)+b$`, and
the `(get|getResponse)+` shape that reads as innocent. It now refuses those too, and the refusal states what the
screen actually knows: it **cannot PROVE the branches disjoint**. It does not claim the branches overlap, and it does
not teach a factoring an already-disjoint alternation could not perform. The verdict also travels one group deeper:
`((a|aa))+b$` and `(?:(?:a|aa))+b$` close their alternation under no quantifier of its own, but its quantified
ancestor repeats it all the same, and the same refusal fires there.

A first-character proof narrows the false-positive class (round 6): an alternation the whole-footprint proof cannot
read is now **proven disjoint when every branch is an unquantified sequence of width-one atoms and the branches'
first-character sets are pairwise disjoint** — at most one branch is viable at any position, so the repetition cannot
multiply. The quoted-string matcher `"([^"\\]|\\.)*"`, a CRLF walker `(\r\n|\n)+`, and keyword pairs `(cat|dog)+` and
`(foo|bar)+` are admitted where the shape was refused wholesale. `(a|aa)+` and `(get|getResponse)+` stay refused:
their branches share a first character, which is exactly the split ambiguity no proof can break.

**The per-file match budget this change carried in earlier rounds is gone** (PR #397 review). It counted engine
line-match attempts per file, so it refused fast linear answers — `^9\d\d$` under `--no-prefilter`,
`\d{4}-\d{2}-\d{2}` over a log — while bounding nothing new: one `std::regex` call is atomic to its caller, so a
counter between calls cannot see the work inside one. A stopped `--regex` scan has ONE cause again — **the engine
abandoning a match** — and its refusal is one sentence: the engine's own reason, the whole stopped count, the first
(lowest-fileId) stopped file, and rewritten advice. The advice no longer suggests rewriting the pattern "so no two of
its alternatives can match the same text": its own example of what to avoid, `(a|a)+`, is refused by the screen at
admission now, so a reader reaching this message can no longer have written it — the reason sentence and the advice
were both changed on purpose, not carried over.

Honest patterns are untouched: the 17-pattern golden set is byte-identical on stdout, stderr and exit code, and the
corpus sweep over 99,845 literals newly refuses 33 and newly admits 0 (round 1's rule). Round 6 re-ran the same
instrument over the grown tree — 100,065 distinct literals, round-5 head against the round-6 screen: the only
newly refused rows are this round's own two paren-bypass gate fixtures, and the 22 newly admitted rows are the
disjoint-first shapes the first-character proof reads, every one of them verified to still terminate on a 4000-byte
pathological line under `--no-prefilter` (`corpus-sweep-round6.txt` in the spec's evidence bundle).

**A single pathological match remains unbounded on libstdc++, tracked by `regex-screen-multiquantifier` (follow-on
spec).** The new rule targets a group with a top-level `|`; a pattern carrying several quantified atoms and no such
group — `a*a*a*a*a*b`, `^(a)*(a)*(a)*(a)*(a)*$` — is admitted, and on libstdc++ a subject that holds its required
literal and then a long run is unbounded work: one `std::regex` call is atomic to its caller, and its backtracking
never throws. libc++ returns only because its engine throws `error_complexity`. The follow-on spec owns it; this
change does not claim it closed. The same holds for a bounded quantifier over a group that itself repeats —
`((a|aa){2,5}){2,5}b$` is admitted (both quantifiers are bounded, by design) and is exponential on libstdc++ — so the
refusal's advice to make the outer quantifier a small explicit bound is safe only when nothing inside the group
repeats; it is the same class as `(a|aa){1,100}b$`.

Contributed by **@pt-act** in #397.
