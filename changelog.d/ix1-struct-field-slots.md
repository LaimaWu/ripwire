### Added — a function stored in a struct field is joined to the calls through that field (IX-1, C / C++)

`--callers=cmd_split_window_exec` on tmux listed the two `.exec =` command-table entries with no dispatcher, and
`--callees=cmdq_fire_command` had no row for `entry->exec( cmd, item )`. The value row now names the dispatcher —
`called_by="cmdq_fire_command" called_at="cmd-queue.c:625" via="slot"` — and `--callees` of the dispatcher carries
`slot_calls=N` and one `<vs l= through= slot="cmd_entry.exec" via= targets= shown= next=>` row per call, listing the
functions stored in that field (16 shown, the rest behind next=). The join is by the declared struct type of both
sides (`via="slot"`): a designated `.f = fn`, a positional entry of a struct the file defines, an anonymous struct
table, or a field assignment, against `x->f( … )`, `x->a->f( … )` or `t[ i ].f( … )`, typed field by field through the
struct bodies and typedefs of the tree. When one side's type is not known the field name alone joins (`via="slot-name"`,
the lower tier); two known, different struct types never join. It is a may-call clue, never a proven call: no count=,
reaches= or call-graph edge changes. On the tmux tree 77 of 79 calls through a struct field get a typed receiver. C++
reads a call through a field only when the receiver is declared with `struct` / `union` or its struct declares the field
as a function pointer in the same file. Not yet: typedef'd function-pointer fields copied between structs (htop's
`this->draw = mode->draw`), macro bodies, Go / Rust / TypeScript function-typed fields.
