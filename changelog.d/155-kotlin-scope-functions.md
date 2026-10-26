### Fixed — Kotlin scope functions no longer bind Java methods

A Kotlin call with a trailing lambda to `let`, `run`, `with`, `apply` or `also` now stays off the Java
method of the same name. Before this change, `s.run { }` bound `Job.run`, and `with( sb ) { }` bound
`Builder.with`. Ripwire now reads the call as the stdlib scope function. It drops the Java candidates and
keeps the Kotlin ones. A call with no Kotlin target reports `external=` with that proof.

Calls without a trailing lambda keep their edges. `j.run()` still binds `Job.run`, and `b.with( "x" )` still
binds `Builder.with`.

Known floors:

- A parenthesised lambda, such as `s.let( { } )`, is not a trailing lambda. Its edge is unchanged.
- A Java method called from Kotlin with a trailing lambda is dropped when its name is a scope-function name.
  The spelling alone does not tell the two apart.
- A receiver of a Java type is not checked. A receiver whose type is an in-tree Kotlin class that defines the method
  keeps its own binding (`t.run { }` on a `t: Task` binds `Task.run`).
- A qualified `kotlin.run { }` can still bind an in-tree Kotlin `run`, hedged.
