### Added — the map shows the type a file is named for when the rank cut left it out (`stem="1"`)

On the default map (also `--json` and the MCP map), a shown file whose top-K rows leave out its stem-namesake type now gains
that type as one more row after its kept rows, marked `stem="1"` (JSON `"stem":1`) and counted in `shown=`. The namesake is the
class, struct or interface of that file with the most name subtokens (3+ bytes) found inside the file's lowercased name, at
least one: starlette's `formparsers.py` showed only `MultiPartException` and `MultipartPart`, and now also shows `FormParser`.
No kept row is dropped. A file whose kept rows already hold a type with as many shared subtokens gets none, and so does a test
file. Measured on starlette's default map: +847 B on 19,056 B (8 files gained a row). The idea (a cluster lists its members
regardless of centrality) was learned from graphify's report; no code was taken. Under `--max-tokens=N` the stem rows are the first to go: none is printed (a stem row is extra bytes that would crowd ranked
rows out of the budgeted cut) and the root says how many it left out, `stem_cut=N` (JSON `"stem_cut":N`). `RIPWIRE_NO_MAP_STEM=1`
turns it off.
Gate: `test/mapstemcheck.sh`.
