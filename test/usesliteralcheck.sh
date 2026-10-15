#!/usr/bin/env bash
# usesliteralcheck.sh — regression gate for JS/TS literal receiver built-in member call filtering in --uses.
#
#   test/usesliteralcheck.sh                        # uses build/ripwire on test/usesliteralfix
#   RIPWIRE_BIN=asan/ripwire test/usesliteralcheck.sh
#
# Fixture test/usesliteralfix exercises:
#   - Direct imported calls: replace, map, test
#   - Literal-receiver calls to built-in members: "hello".replace, `template`.replace, [1, 2, 3].map, /re/.test
#   - Casted non-literal receiver call: (x as string).replace
#   - Prototype method polyfill: String.prototype.trimEnd and literal call "abc".trimEnd()
#   - Name with NO in-repo definition: "hello".repeat(3) (must answer with exit 0 and external="1")
#
# Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"          # allow a repo-relative RIPWIRE_BIN
CORPUS="$ROOT/test/usesliteralfix"
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ]   || { echo "no ripwire binary at $BIN — build first"; exit 2; }
[ -d "$CORPUS" ]|| { echo "no test/usesliteralfix dir — fixture missing"; exit 2; }

echo "usesliteralcheck: BIN=$BIN  CORPUS=$CORPUS"

# helper: extract call sites as "role basename:line"
uses_sites(){ "$BIN" "$CORPUS" --uses="$1" --no-cache 2>/dev/null | grep -o 'role="[a-z]*" p="[^"]*"' \
    | sed -E 's/role="([a-z]*)" p="([^"]*\/)?([^"/]*)"/\1 \3/'; }
has_site(){ uses_sites "$1" | grep -qxF "$2"; }

# ── 1) replace: literal calls filtered out, direct + casted receiver kept, declined_literal_calls="2" ──
REPLACE_OUT="$( "$BIN" "$CORPUS" --uses=replace --no-cache 2>/dev/null )"
echo "$REPLACE_OUT" | grep -q 'count="2"' && ok "replace count=2" || no "replace count != 2: $REPLACE_OUT"
echo "$REPLACE_OUT" | grep -q 'declined_literal_calls="2"' && ok "replace declined_literal_calls=2" || no "replace missing declined_literal_calls=2"
has_site replace "call app.ts:4"  && ok "replace direct call kept @app.ts:4" || no "replace direct call missing @app.ts:4"
has_site replace "call app.ts:16" && ok "replace casted receiver call kept @app.ts:16" || no "replace casted receiver missing @app.ts:16"
! has_site replace "call app.ts:8" && ok "replace literal string call dropped @app.ts:8" || no "replace literal string call leaked @app.ts:8"
! has_site replace "call app.ts:9" && ok "replace template literal call dropped @app.ts:9" || no "replace template literal call leaked @app.ts:9"

# ── 2) map: literal array call filtered out, direct call kept, declined_literal_calls="1" ─────────────
MAP_OUT="$( "$BIN" "$CORPUS" --uses=map --no-cache 2>/dev/null )"
echo "$MAP_OUT" | grep -q 'count="1"' && ok "map count=1" || no "map count != 1: $MAP_OUT"
echo "$MAP_OUT" | grep -q 'declined_literal_calls="1"' && ok "map declined_literal_calls=1" || no "map missing declined_literal_calls=1"
has_site map "call app.ts:5" && ok "map direct call kept @app.ts:5" || no "map direct call missing @app.ts:5"
! has_site map "call app.ts:10" && ok "map literal array call dropped @app.ts:10" || no "map literal array call leaked @app.ts:10"

# ── 3) test: literal regex call filtered out, direct call kept, declined_literal_calls="1" ───────────
TEST_OUT="$( "$BIN" "$CORPUS" --uses=test --no-cache 2>/dev/null )"
echo "$TEST_OUT" | grep -q 'count="1"' && ok "test count=1" || no "test count != 1: $TEST_OUT"
echo "$TEST_OUT" | grep -q 'declined_literal_calls="1"' && ok "test declined_literal_calls=1" || no "test missing declined_literal_calls=1"
has_site test "call app.ts:6" && ok "test direct call kept @app.ts:6" || no "test direct call missing @app.ts:6"
! has_site test "call app.ts:11" && ok "test literal regex call dropped @app.ts:11" || no "test literal regex call leaked @app.ts:11"

# ── 4) trimEnd: prototype polyfill keeps literal call row ─────────────────────────────────────────────
TRIM_OUT="$( "$BIN" "$CORPUS" --uses=trimEnd --no-cache 2>/dev/null )"
echo "$TRIM_OUT" | grep -q 'defs="1"' && ok "trimEnd defs=1" || no "trimEnd defs != 1: $TRIM_OUT"
echo "$TRIM_OUT" | grep -q 'count="1"' && ok "trimEnd count=1" || no "trimEnd count != 1: $TRIM_OUT"
has_site trimEnd "call polyfill.js:5" && ok "trimEnd polyfill call kept @polyfill.js:5" || no "trimEnd polyfill call missing @polyfill.js:5"

# ── 5) repeat: no in-repo definition answers with exit 0 and external="1" ────────────────────────────
REPEAT_CODE=0
REPEAT_OUT="$( "$BIN" "$CORPUS" --uses=repeat --no-cache 2>"$TMP/err" )" || REPEAT_CODE=$?
[ "$REPEAT_CODE" -eq 0 ] && ok "repeat exits 0 with no in-repo definition" || no "repeat exited $REPEAT_CODE"
echo "$REPEAT_OUT" | grep -q 'external="1"' && ok "repeat reports external=1" || no "repeat missing external=1: $REPEAT_OUT"
echo "$REPEAT_OUT" | grep -q 'count="1"' && ok "repeat reports count=1" || no "repeat missing count=1: $REPEAT_OUT"
has_site repeat "call app.ts:19" && ok "repeat call kept @app.ts:19" || no "repeat call missing @app.ts:19"

# ── 6) determinism ────────────────────────────────────────────────────────────────────────────────────
"$BIN" "$CORPUS" --uses=replace --no-cache >"$TMP/a" 2>/dev/null
"$BIN" "$CORPUS" --uses=replace --no-cache >"$TMP/b" 2>/dev/null
if diff -q "$TMP/a" "$TMP/b" >/dev/null; then
    ok "determinism (byte-identical run-to-run)"
else
    no "determinism failure"
fi

exit "$fail"
