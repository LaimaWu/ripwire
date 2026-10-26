### Fixed — a symbol written the way the source spells it now resolves: `Class.method` in TypeScript, `json.dump`, `batcheval.Put`, `ns::Outer::Inner::f`

Every verb that takes a symbol (`--callers`, `--callees`, `--path`, `--impact`, `--connect`, `--expand` and the MCP twins)
answered "symbol not found" for three spellings that name exactly one definition: a TypeScript/JavaScript method as
`Class.method` (those grammars store no owner scope, so the owner is now read from the enclosing class), a module, crate or
package prefix (`asyncio.run`, `json.dump`, Go `batcheval.Put` and `batcheval.Cmd.Eval`, Rust `crate::buf::Buffer::edit` or
`my_crate::Buffer::edit`; the prefix must name the definition's directories or file stem, in order), and a C++ owner the index
keeps only the innermost label of (an out-of-line `ConnectionManagerImpl::ActiveStream::decodeHeaders`, a namespace-qualified
`HttpConnectionManager::Utility::createSingletons`; read from the source's namespaces and written qualifier). The new rule runs
only after every existing spelling rule found nothing, so a symbol that resolved before resolves exactly as before. It binds only
when the spelling names one definition (a declaration beside its definition counts as one); two same-named classes, a prefix two
crates both satisfy, an import alias (`be.Put`), a type alias, or a module no path names keep the "not found" answer. Measured on
the 15 failing listed argvs of the round-6 tables: 8 of the 11 that name one definition now resolve; the other 3 sit in Rust
files the grammar fails to parse (a separate fix). Gate: `test/writtenspellingcheck.sh`.
