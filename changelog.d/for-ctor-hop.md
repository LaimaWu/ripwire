### Added — `--for` names what the top method's class constructor calls (`ctor_l=`, `ctor_calls=`)

On a conceptual `--for` answer (root `bundle="compact"`), when the first row (r=1) is a method whose innermost enclosing class
defines a constructor (JS/TS `constructor`, Python `__init__`, Ruby `initialize`, PHP `__construct`, Swift `init`, a
Java/C#/C++/Kotlin/Dart member named as its class), that row now carries `ctor_l="N"` (the constructor's line) and
`ctor_calls="a,b"`: up to 16 distinct callee names the index resolved from the constructor, in name order (so the 16 kept do not depend on which file defines each), with `ctor_calls_more="N"` counting the
rest. Names only, not a claim they answer. Not on a class with no constructor, an r=1 class row, or a constructor whose text the
answer already serves (the body fold, the body slot). Measured on koa's "how does Koa create the per-request ctx" question:
`createContext` now names `ctor_calls="getAsyncLocalStorage"` (+176 B on a 12,850 B answer). The constructor's `this.f =`
field writes are not listed. The idea (a one-hop walk from the top seed over containment) was learned from code-review-graph's
graph traversal; no code was taken. `RIPWIRE_NO_CTOR_HOP=1` turns it off. Gate: `test/forctorhopcheck.sh`.
