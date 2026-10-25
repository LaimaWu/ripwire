#!/usr/bin/env bash
# mapstemcheck.sh — the map scope reserves one row per shown file for its STEM-NAMESAKE type (card MAP-STEM;
# src/serialize.h addMapStemRows).
#
# THE CONTRACT THIS PINS (arms tagged [P] are RED on the pre-change binary 0b2734a7; [G] arms hold on both binaries):
#   WHAT     on the default map (and --json twin), a shown file whose top-K rows leave out the class/struct/interface with the
#            most name subtokens inside the file's stem gains that type as one more row, after its kept rows, stem="1"
#            (JSON "stem":1), counted in shown=. No kept row is dropped.
#   NOT ON   a file whose kept rows hold a type with as many shared subtokens; a test file (layer="test"); a map that keeps
#            every symbol; RIPWIRE_NO_MAP_STEM=1; a --for answer.
# Arms:
#   A1 [P] --top-k=3: pkg/widgetparsers.py shows WidgetError then WidgetParser stem="1"
#   A2 [P] the stats comment's shown= is 4 (3 kept + 1 stem row)
#   A3 [P] --json twin: the same rows, WidgetParser "stem":1, "shown":4
#   B1 [G] --top-k=4: pkg/helpers.py keeps HelperRegistry (it holds the stem) and gains nothing
#   B2 [G] --top-k=6: tests/test_widgetparsers.py is shown and gains no stem row (TestWidgetParsersSuite stays out)
#   B3 [G] the default top-k keeps every symbol: no stem="1", no clause
#   C1 [G] no row lost: the --top-k=3 answer minus the stem row, its clause and shown= equals RIPWIRE_NO_MAP_STEM=1's
#   L1 [P] the clause rides when a row carries stem="1"; L2 [G] RIPWIRE_NO_MAP_STEM=1 carries neither
#   X1 [G] determinism   X2 [G] xmllint well-formed (when present)
#   D1 [P] --max-tokens=1060 (fix round 1, CI 40206090157 readmedriftcheck G4): the stem rows are the FIRST to go under a budget -
#          the budgeted map prints exactly the ranked rows RIPWIRE_NO_MAP_STEM=1 prints at the same budget (a stem row never displaces
#          one) and no stem="1" row
#   D2 [P] at --max-tokens=1000 (WidgetParser is cut, its stem row would have been added) it says what it left out: stem_cut=1 on the root, defined by its legend clause, shown= = the rows printed
#   D3 [P] --json twin: "stem_cut":1, no "stem":1 row   D4 [G] no budget (A1) and RIPWIRE_NO_MAP_STEM=1 carry no stem_cut
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_MAP_STEM
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "mapstemcheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
FIX="$ROOT/test/mapstemfix"
[ -f "$FIX/pkg/widgetparsers.py" ] || { echo "fixture $FIX missing"; exit 2; }
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
export HOME="$TMP/home"; mkdir -p "$HOME"

run(){
    local name="$1"; shift
    local envs=()
    while [ "$#" -gt 0 ] && [ "$1" != "--" ]; do envs+=( "$1" ); shift; done
    shift
    ( cd "$FIX" || exit 1; env ${envs[@]+"${envs[@]}"} "$BIN" "$FIX" "$@" --cache="$TMP/$name.cache" >"$TMP/$name.out" 2>"$TMP/$name.err" ) \
        || no "$name: ripwire exited non-zero ($( head -c 200 "$TMP/$name.err" | tr '\n' ' ' ))"
}
run a1 -- --top-k=3
run a1off RIPWIRE_NO_MAP_STEM=1 -- --top-k=3
run a3 -- --top-k=3 --json
run b1 -- --top-k=4
run b2 -- --top-k=6
run b3 --
# The budget arms root at `.`, not the absolute fixture path: the path's length is charged against the byte ceiling, so an
# absolute root would move the cut with the checkout's location.
runrel(){
    local name="$1"; shift
    local envs=()
    while [ "$#" -gt 0 ] && [ "$1" != "--" ]; do envs+=( "$1" ); shift; done
    shift
    ( cd "$FIX" || exit 1; env ${envs[@]+"${envs[@]}"} "$BIN" . "$@" --cache="$TMP/$name.cache" >"$TMP/$name.out" 2>"$TMP/$name.err" ) \
        || no "$name: ripwire exited non-zero ($( head -c 200 "$TMP/$name.err" | tr '\n' ' ' ))"
}
runrel d1 -- --max-tokens=1060
runrel d1off RIPWIRE_NO_MAP_STEM=1 -- --max-tokens=1060
runrel d2 -- --max-tokens=1000
runrel d3 -- --max-tokens=1000 --json
run x1 -- --top-k=3

cat > "$TMP/chk.py" <<'PY'
import json, re, sys
cmd, a = sys.argv[1], sys.argv[2:]
t = open(a[0], encoding="utf-8").read() if a else ""
def file_rows(t, p):
    m = re.search(r'<f p="%s"[^>]*>(.*?)</f>' % re.escape(p), t, re.S)
    return None if m is None else re.findall(r'<s [^>]*>', m.group(1))
def name(r): return re.search(r'\bn="([^"]*)"', r).group(1)
if cmd == "a1":
    rows = file_rows(t, "pkg/widgetparsers.py") or []
    got = [(name(r), 'stem="1"' in r) for r in rows]
    if got != [("WidgetError", False), ("WidgetParser", True)]: print(got); sys.exit(1)
elif cmd == "shown":
    m = re.search(r' shown=(\d+) ', t)
    if not m or m.group(1) != a[1]: print("shown=%s" % (m and m.group(1))); sys.exit(1)
elif cmd == "json":
    d = json.loads(t)
    f = [x for x in d["r"] if x["p"] == "pkg/widgetparsers.py"]
    got = [(s["n"], s.get("stem")) for s in f[0]["s"]] if f else None
    if got != [("WidgetError", None), ("WidgetParser", 1)] or d.get("shown") != 4: print(got, d.get("shown")); sys.exit(1)
elif cmd == "budget":
    off = open(a[1], encoding="utf-8").read()
    rows = lambda x: re.findall(r'<s [^>]*\bk="', x)
    if 'stem="1"' in re.sub(r"<!-- .*?-->", "", t, flags=re.S): print("a stem row under a budget"); sys.exit(1)
    if len(rows(t)) != len(rows(off)) or len(rows(t)) < 2: print("rows %d vs the no-stem run's %d" % (len(rows(t)), len(rows(off)))); sys.exit(1)
elif cmd == "cut":
    sh = re.search(r' shown=(\d+) ', t); n = len(re.findall(r'<s [^>]*\bk="', t))
    if ' stem_cut="1"' not in t or "<!-- stem_cut=" not in t: print("no stem_cut=1 / clause"); sys.exit(1)
    if not sh or int(sh.group(1)) != n: print("shown=%s but %d rows" % (sh and sh.group(1), n)); sys.exit(1)
elif cmd == "cutjson":
    d = json.loads(t)
    if d.get("stem_cut") != 1 or any(s.get("stem") for x in d["r"] for s in x["s"]): print(d.get("stem_cut")); sys.exit(1)
elif cmd == "nocut":
    if "stem_cut" in t: print("stem_cut present"); sys.exit(1)
elif cmd == "nostemin":
    rows = file_rows(t, a[1])
    if rows is None: print("%s not shown (vacuous)" % a[1]); sys.exit(1)
    if any('stem="1"' in r for r in rows): print("stem row in %s" % a[1]); sys.exit(1)
elif cmd == "nostem":
    if 'stem="1"' in t or "<!-- stem=" in t: print("stem= present"); sys.exit(1)
elif cmd == "clause":
    if "<!-- stem=" not in t: print("no clause"); sys.exit(1)
elif cmd == "prefix":
    off = open(a[1], encoding="utf-8").read()
    on = re.sub(r'<s [^>]*stem="1"[^>]*></s>', "", t)
    on = re.sub(r"<!-- stem=.*?-->", "", on, flags=re.S)
    mask = lambda s: re.sub(r'est_tokens="?\d+"?', "E", re.sub(r" shown=\d+ ", " shown=N ", s))
    if mask(on) != mask(off): print("rows beyond the stem row differ"); sys.exit(1)
PY
chk(){ python3 "$TMP/chk.py" "$@"; }
if m="$( chk a1 "$TMP/a1.out" )"; then ok "A1 WidgetParser rides after WidgetError, stem=\"1\""; else no "A1 $m"; fi
if m="$( chk shown "$TMP/a1.out" 4 )"; then ok "A2 shown=4 counts the stem row"; else no "A2 $m"; fi
if m="$( chk json "$TMP/a3.out" )"; then ok "A3 --json twin: \"stem\":1, \"shown\":4"; else no "A3 $m"; fi
if m="$( chk nostemin "$TMP/b1.out" pkg/helpers.py )"; then ok "B1 a file whose kept rows hold its stem type gains nothing"; else no "B1 $m"; fi
if m="$( chk nostemin "$TMP/b2.out" tests/test_widgetparsers.py )"; then ok "B2 a test file gains no stem row"; else no "B2 $m"; fi
if m="$( chk nostem "$TMP/b3.out" )"; then ok "B3 every symbol kept: no stem row, no clause"; else no "B3 $m"; fi
if m="$( chk prefix "$TMP/a1.out" "$TMP/a1off.out" )"; then ok "C1 no kept row lost or moved: only the stem row, its clause and shown= differ"; else no "C1 $m"; fi
if m="$( chk budget "$TMP/d1.out" "$TMP/d1off.out" )"; then ok "D1 --max-tokens=1060: no stem row, the same ranked rows as the no-stem run"; else no "D1 $m"; fi
if m="$( chk cut "$TMP/d2.out" )"; then ok "D2 stem_cut=1 + its clause, shown= = rows printed"; else no "D2 $m"; fi
if m="$( chk cutjson "$TMP/d3.out" )"; then ok "D3 --json twin: \"stem_cut\":1, no stem row"; else no "D3 $m"; fi
if m="$( chk nocut "$TMP/a1.out" )" && m="$( chk nocut "$TMP/d1off.out" )"; then ok "D4 no budget / RIPWIRE_NO_MAP_STEM=1: no stem_cut"; else no "D4 $m"; fi
if m="$( chk clause "$TMP/a1.out" )"; then ok "L1 the clause defines stem="; else no "L1 $m"; fi
if m="$( chk nostem "$TMP/a1off.out" )"; then ok "L2 RIPWIRE_NO_MAP_STEM=1: no stem row, no clause"; else no "L2 $m"; fi
if cmp -s "$TMP/a1.out" "$TMP/x1.out"; then ok "X1 determinism"; else no "X1 two runs differ"; fi
if command -v xmllint >/dev/null 2>&1; then
    if xmllint --noout "$TMP/a1.out" 2>/dev/null; then ok "X2 xmllint well-formed"; else no "X2 not well-formed"; fi
else
    echo "  SKIP  X2 xmllint not installed"
fi
[ "$fail" = 0 ] && echo "mapstemcheck: PASS" || echo "mapstemcheck: FAIL"
exit "$fail"
