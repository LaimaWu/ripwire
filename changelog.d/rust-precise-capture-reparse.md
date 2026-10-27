### Fixed — Rust files using an empty `+ use<>` bound (precise capturing) no longer lose every impl owner

The bundled Rust grammar predates precise capturing (Rust 1.82). It reads a `use<..>` bound as a generic type named
`use`, and an EMPTY list (`-> impl Future<Output = T> + use<>`) needs a missing type node; around a long enough
function body the parser's recovery then turned the WHOLE file into one error node (`--skipped`: `err_ratio="1.000"`),
so every method in it was indexed as a free `fn` with no owner and `Type::method` selectors did not bind. Measured on
the zed repository: 25 of 2,033 Rust files, including the workspace pane, the workspace itself and the language
buffer. A Rust file whose first parse holds error bytes is now parsed a second time with its precise-capturing bounds
replaced by spaces (every byte offset and line unchanged), and the second parse is kept only when it holds strictly
fewer error bytes, the rule the C-family member-macro re-parse already uses. On zed: 0 files left at err_ratio >= 0.5
(was 25); in those 25 files method rows rose from 1,601 to 2,925 and scope-less `fn` rows fell from 1,502 to 231;
`Pane::save_item`, `Workspace::new_local` and the language crate's `Buffer::edit` now bind. A `use<..>` list names only
generic parameters and lifetimes already in scope, so nothing the index records comes from the blanked bytes.
`--skipped` discloses each such file (`why="use-bounds-blanked"`, `use_bounds_blanked=` on the row,
`use_bounds_blanked_files=` on the root). It does NOT teach the grammar the construct: a file whose first parse has
no error bytes (a short body recovers with a zero-width missing node) is not re-parsed, and nushell, mdbook and the
other in-sample Rust repositories, which do not use the bound, parse byte-for-byte as before.
