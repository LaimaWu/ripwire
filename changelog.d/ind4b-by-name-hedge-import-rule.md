### Changed — by-name hedges: a free namesake needs an import, an override is a way in


- A by-name hedge (`<near_sink>` `why="name"`, a `<bridge>` `via="namesake"` crossing) to a namesake with no class (a free
  function, or a JavaScript/TypeScript method, whose class the extractor does not record) now needs the caller's file to be,
  or to import, the namesake's file (a direct import edge). A namesake file the search merely touched no longer counts. For
  C, C++ and Objective-C the rule follows the transitive `#include` closure, but no probe of ours produced a by-name hedge
  to a C-family free function: those calls are declined, not bound by name. On the 880-pair seed-18 `--path` draw over 11 in-sample repos this
  withholds the 2 shown by-name sites a hand-check had rejected (koa's `delegate( proto, 'response' ).method( … )` reaching
  `request.js`'s `get method`, and a bundled Bootstrap `this.init( … )` reaching nunjucks' `Parser.init`). The one accepted
  site, flake8's `report()`, stays. 16 rows that already showed a value or declined site keep it and now also count a by-name way in.
- A class method namesake is now also a way in when its class derives from the class of the definition the call was bound
  to, at any depth, and every step of that derivation is bound in its own file: the base name a class writes resolves,
  through that file's own declaration or its import of that name (a package re-export, and for Python a `src/` layout
  import that resolves by package name, count), to the next class. This is override dispatch. A base clause that names a
  different same-named class (one elsewhere in the tree, or an external one: `class Sub(ext.Base)`, `from ext import Base`)
  does not count, however the file's other imports reach the parent's file; neither does a base no import binds. On the
  same draw this shows 6 sites, and a hand-check read all 6 as correct: textual's `driver.start_application_mode()` /
  `stop_application_mode()` → `LinuxDriver`, `widget.render_line( y )` → `TextArea`, and `node.compose()` → three `App`
  subclasses in examples. Like every by-name hedge, each is a hedge and never a proven edge. What stays withheld: a base
  the binding does not read (a dotted `a.b.Base`), and a Python base name that has any binding other than its import in
  the class statement's own scope chain (its class body or function, the enclosing functions, then the module; an
  enclosing class body is skipped, as Python skips it): an assignment of any form (`Base = Other`, `Base: type = Other`),
  a def, class, loop, `with` or `except` target, a parameter, a `global` or `nonlocal` (also in a def that starts the
  file), a `match` capture (`case Base:`, `case P as Base`, `*Base`), a PEP 695 `type Base = …` or a class or def type
  parameter (`class Sub[Base](Base)`), a second import, an import under another name (`from m import Other as Base`, which
  binds m's `Other`, never `Base`), a `try:
  from a import Base / except ImportError: Base = None` fallback, or a star import that could bind it (one from a module
  outside the tree, or from a module that defines, imports or star-imports the name), wherever in the scope it sits and
  in either order. A qualified base (`q.Base`) is also withheld when another function or class imports `q` as a module
  (`import a.b as q`), since the import tables are per file; this over-hedges the same import repeated in two scopes,
  and a star import from an in-tree module whose `__all__` omits the name. `from pkg import other as a` with `a.Base` now
  reads `other` (the member's own name), not `a`; a dotted `import pkg.x` binds the root `pkg`, which no table names, so
  `pkg.Base` is withheld (this also over-hedges `import pkg.x as pkg`). `from pkg import a` (aliased or not) with `a.Base`
  reads the submodule `a` only when the package's `__init__` does not bind `a` itself (an import, def, class or assignment of
  it, a star import that could bind it, or a module `__getattr__`). A re-export chain (`from pkg.x import Base`, where the
  package or a middle module re-exports the name) is followed only through files that bind the name by ONE plain import,
  at every hop: a hop that renames it (`from pkg.x.a import Other as Base`), reassigns it afterwards (`Base = Other`, `Base +=`,
  a `for`, `with` or walrus target, a def or class of it, a `global Base` in any def), star-imports after it, or has no
  record of it ends the chain, and the site is withheld. A hop file that calls `globals()` or `setattr(...)` anywhere, or
  defines a module `__getattr__`, counts as unknown for every name (a dynamic write no table sees; this over-hedges any such
  file, and `sys.modules[__name__].a = x`, which is not a call, stays a floor). The import must be the name's one binding;
  otherwise the site is withheld, never shown. Not read, and not detectable statically: a rebinding made from outside
  the file (`a.Base = Other` on an imported module).
  The chain's endpoint is held to the same standard: the class it finds is the name's meaning only when its own `class`
  statement is the name's one module-scope binding in that file and no star import there could bind the name, so a later
  `Base = Other`, a `global Base` in a def, a second def or class of it, or a trailing `from m import *` now withholds the
  site (this over-hedges a class defined in both arms of an `if`/`try`). Residual: an endpoint file's `globals()` or
  `setattr(...)` write is not read there (a hop file's is), so that shape may still be shown.
