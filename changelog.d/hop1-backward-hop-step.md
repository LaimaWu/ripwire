### Added — one backward hop step for `--path` and `--impact` (slot and declined callers)

`--callers=X` already names, one hop per call, who MAY call X without a graph edge: a `<vr via="slot">` row's
`called_by=` (a call through a struct field X is stored in) and a `<dc>` row (a declined call whose candidate list holds
X). The answers that join hops now read the same rows, from one provider (`src/backhops.h`):

- `--path`'s hedged `<bridge>` crosses a slot hop (`via="slot"`, `via="slot-name"` for a field-name-only join) while the
  join names at most 16 calling functions (`slot_callers=`), and `slot_fns=` says how many functions the field holds
  (not capped: a wide table such as tmux's 92-entry `cmd_entry.exec` is still crossed, and the row says how wide it is); a
  chain whose every crossing is a slot may take three crossings, any other chain keeps two. On tmux, `--path=cmdq_fire_command,window_copy_cmd_cancel` now prints the
  three-field-call chain (`cmd_entry.exec`, `window_mode.command`, the command table's `.f`); before, no chain.
- A `reachable="0"` path answer whose search met gaps and whose `to=` has such a hop ends with `next="--callers=<to>"`
  (CLI and MCP `path_between`) instead of the gap rows' bodies.
- `--impact` (XML, `--json`, columnar count, MCP `impact`) adds `reaches_hedged=N` beside `reaches=`: the functions that
  may reach the symbol through one or two calls through struct fields, with the two-hop ones listed as
  `<ring d="2"><rh called_by= called_at= via= calls=/></ring>` (16 shown, no width bound on the d=1 set; `next=` pages the callers of the first unshown row's `calls=`, not every hidden row). Never counted in
  `reaches=`, `by_depth=` or the radius counts.

Every crossing stays a hedge: `proven="0"`, `via=` named, its width beside it (a slot crossing's `slot_callers=` is bounded at 16, its `slot_fns=` is printed, not capped), and never in `reachable=`/`hops=`/`reaches=`.
