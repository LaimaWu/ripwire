### Changed — a member row names its owner: `n="Owner.name"` on `--callers`, `--callees`, `--impact`, `--path` and signature rows

A list of `run`, `run`, `run` made the reader work out which class each one belongs to. A row for a member now prints its
owner from the parser's stored scope (the `sc=` other rows print), in the language's own spelling: `Builder::Build` in C,
C++ and Rust, `Task#execute` in Ruby (a `def self.x` singleton prints `#` too, because the index does not tell the two
apart), `Pattern.match` elsewhere. Every spelling is one the selectors already resolve, so a row can be pasted back as
`--callers=Owner.name`. The owner is printed only when the index defines it as a class, struct, interface, named type or
module in that language; otherwise the row stays bare: a free function, a type row, a closure, an extent-suspect symbol, a
C++ namespace-scope function (unless a class of the same name exists in that language, when it prints `Name::f`), and
every TypeScript, JavaScript, Java, C#, Swift and Kotlin `object` member, whose definitions store no scope today. On a
signature `<d>` row the owner folds into `n=` and `sc=` is dropped. A `--for` answer about one function lists its callers
the same way. `--json` keys, `<c>` callee rows, tail rows and the map are unchanged. Measured on the 163 non-sealed
round-5 first calls (frozen grader): Rule C +12 items, −7 (rake-21, where the grader reads `Task#execute` as no match for
`execute`), 106 answers byte-identical, net −217 B; on the 39 MF1-DEV questions +12 items, 0 lost. Those figures are this change alone: composed with the interface-dispatch change (a call on an interface-typed receiver declines instead of listing every same-named method), `mf1-ninja-02` and `ninja-14` lose required items (outcomes unchanged). Gate:
`test/qualnamecheck.sh`.
