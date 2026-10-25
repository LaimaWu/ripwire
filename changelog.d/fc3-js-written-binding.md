### Fixed — JS/TS calls through a name written after its declaration are no longer read as the declared class

`let { Eng, Foo } = require('a'); Eng = Foo; new Eng()` builds a `Foo`, so a BARE call by a name written in its scope is
unresolved (`unresolved=`), never its declared target nor a same-spelled class. A MEMBER call through such a root (`Eng.make()`,
`tank.spill()`) is not receiver-typed: the root's spelling does not name a class, so the call keeps only the by-name hedge
(`via="name"`, every candidate), with or without a declaration (`let Eng;`, an undeclared global). A write whose value keeps the
name (`Environment = require('x').Environment` over `var Environment;`) and a stated constructor (`let app; app = new Hono()`) are
unchanged. Residual, not fixed: a member call whose receiver is a typed chain the tool does not follow (kysely
`ctx.db.selectFrom('person').selectAll().limit(100)`) still falls to the by-name ladder, and the merged caller row carries the
sites of a bare imported `limit(…)` call in the same file unhedged. Refusing every member call a free function (which JS/TS allow
through an object property) changed about 9,000 census rows on six in-sample repos (hono, kysely, nunjucks, fastify, commander, koa),
so it is left to a later card. Gate: `test/falseedgecheck.sh` arm (S).
