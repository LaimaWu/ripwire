### Added — `--for` names the functions a shown table initializer binds (`slots=`)

On a conceptual `--for` answer (root `bundle="compact"`), a shown variable or constant row whose initializer stores functions
now carries `slots="k=fn,…"`: up to 16 slot=function pairs in source order, where `k` is the slot as `--callers`/`--callees`
`into=` spells it less the row's own name (`.command = window_copy_command` → `command=window_copy_command`; a positional C
entry stays `[0]`; a Python key stays `["run"]`), and `slots_more="N"` counts the pairs it did not list. A pair is a
reference-as-value binding by the index, not a call and not a relevance claim. A row whose body the answer already serves (the
body fold, the body slot) carries none, and neither does a struct, class or function row. Measured on tmux's "orient me on copy
mode" question: the `window_copy_mode` row now names `command=window_copy_command` (+352 B on an 18,401 B answer).
The mechanism (a constant initializer treated as a node whose bindings are shown) was learned from codegraph's explore tool;
no code was taken. `RIPWIRE_NO_INIT_SLOTS=1` turns it off. Gate: `test/forinitslotscheck.sh`.
