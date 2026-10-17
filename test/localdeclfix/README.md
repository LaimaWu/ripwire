# localdeclfix — fixture for test/localdeclcheck.sh

A declaration inside a function body is local to it and must not become an indexed symbol; the same name at
package / item / script scope must stay one. `go/` (var, `var ( … )` group, `x :=`, const, a func literal, a nested
block, a same-named package var), `rust/` (fn / method / closure `const` and `static`, and a const beside a `mod` declared in a fn, vs crate, mod, fn-nested mod, impl, trait and
extern items), `gd/` (func `const` vs script and inner-class `const`).
