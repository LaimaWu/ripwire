### Changed — `--situ` and `--test-gate` start from the changed symbols, not from the changed files (#423, thanks @llvm-x86)

Before: both verbs started the caller walk from every symbol of every changed file. A small edit in a large shared file
gave a very large answer. A 3-line edit in `QuerySet.exists()` (django, `query.py`, 191 symbols) gave 4,748 symbols
in 442 files and 323 tests to run. Most of those tests did not relate to the edit.

Now: the verbs run one `git diff -U0 HEAD` for the whole tree. A function or method is a start symbol when a hunk
overlaps its body. The walk starts from those symbols only. The same edit gives 282 symbols in 74 files and 56 tests.
On sentry (`crashreports_exceeded`) the result goes from 11,534 to 3,036 symbols and from 1,333 to 385 tests.

The change has a safe failure direction. A hunk that no function body fully holds sends its whole file back to the old
file-level treatment. These hunks are a module-level edit, a deletion, a class or variable edit, a decorator edit, and a
hunk that is only partly inside a body (a changed last line plus a module-level line under it). A hunk inside a member that
callers reach through its class also falls back: a constructor, destructor or initializer, a Python dunder, and a
property, getter or setter. Nobody calls `__init__` by name, so narrowing there would walk zero callers and turn the gate
green on an edit every user of the class feels. With no hunk evidence (no git, no HEAD) the result equals the old result,
and an explicit file list (`--situ=F`, `--test-gate=F`, the MCP `diff` argument) is never narrowed by the working-tree diff.
One more effect: an edit that only adds a new function to a tracked file seeds that function alone, and a new function has no
callers yet, so its blast radius is 0 and `--test-gate` can exit 0 where it exited 4 before.

The report names the treatment it used. `--situ` always prints a `change_set:` line. The `--test-gate` XML and JSON add
`change_granularity=`, `changed_symbols=`, `file_symbols=`, `files_narrowed=`, `files_file_granular=` and `diff_hunks*=`
when the diff has hunks. The MCP `situational_awareness` verb carries the same keys. The legend
dictionary gains four entries.

Limit: a name-based graph cannot model readers of a constant. Because of this, a constant edit keeps its whole file.
Measured on two public repositories with one edit each. These are not suite-wide figures.
