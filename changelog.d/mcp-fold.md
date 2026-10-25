### Changed — MCP `quality_delta` and `edit_check` fold their bookkeeping rows like the CLI (`show_stale`)

- MCP `quality_delta`: a regression-free answer with stale acks now drops its `sa` rows and carries `stale_by_kind` (the
  stale count split by ack kind, the same string the CLI prints) and `stale_next` (`show_stale=true`), when that saves at
  least a quarter of the bytes. Every count stays on the answer. The new boolean argument `show_stale=true` (the CLI
  `--show-stale` twin) returns the full document unchanged. Measured on ripwire's own tree: 15,015 B to 567 B; on the eight
  other sweep repos the answer has no stale acks and is byte-identical.
- MCP `edit_check`: an unchanged contract at git HEAD with no flagged caller answers the verdict and `rows_next=` (the
  MCP call, `edit_check symbol=SYM limit=N`, that lists the callers), as `--edit-check` already does; `callers=` keeps the count. Median 1.72x smaller on the
  repos where it applies (1.62x to 3.46x). A dirty tree, a flagged caller, `new_body` previews and an explicit `limit` or
  `offset` still list every row.
- It does NOT drop a regression row, a flagged caller or any count: the folded rows are one argument away.
