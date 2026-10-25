#!/usr/bin/env bash
# forctorhopcheck.sh — the OWNER'S CONSTRUCTOR on the r=1 method row of a compact-route --for answer (card OWNER-CTOR-HOP;
# src/forctorhop.h).
#
# THE CONTRACT THIS PINS (arms tagged [P] are RED on the pre-change binary 0b2734a7; [G] arms hold on both binaries):
#   WHAT     when the r=1 row is a method whose innermost enclosing class defines a constructor (JS `constructor`, Python
#            `__init__`, ...), that row gains ctor_l="N" (the constructor's line), ctor_calls="a,b" (<= 16 resolved callee names)
#            and ctor_calls_more="N" (names cut). Posture: mem='s (compact route, XML, default regime).
#   NOT ON   a class with no constructor; an r=1 row that is the class itself; a constructor the fold serves (the class body).
# Arms:
#   A1 [P] JS: create (r=1) carries ctor_l="10" ctor_calls="makeStore,makeLogger" (fold off)
#   A2 [P] Python control: Registry.create carries ctor_l="6" ctor_calls="make_store" — the same rule on `__init__` (fold off)
#   B1 [G] a class with no constructor: create carries no ctor_l=
#   B2 [G] fold on: the fold serves Registry's body (it holds the constructor): no ctor_l=
#   B3 [G] r=1 is the class Registry: no ctor_l= on any row
#   F1 [G] RIPWIRE_NO_CTOR_HOP=1: none and no clause; prefix identity (compact and full legends); --json and name-exact none
#   L1 [P] the clause rides in both dialects when a row carries ctor_l=; L2 [G] absent when none does
#   X1 [G] determinism   X2 [G] xmllint well-formed (when present)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_CTOR_HOP RIPWIRE_NO_INIT_SLOTS RIPWIRE_NO_MEMBER_NAMES RIPWIRE_NO_BODY_FOLD RIPWIRE_NO_BODY_SLOT RIPWIRE_NO_OWNER_HOP
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "forctorhopcheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*"; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
FIX="$ROOT/test/ownerctorfix"
for d in js py noctor; do [ -d "$FIX/$d" ] || { echo "fixture $FIX/$d missing"; exit 2; }; done
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
export HOME="$TMP/home"; mkdir -p "$HOME"
Q="how does create make an entry from the store?"
Q_CLS="how does the registry create an entry?"

# run NAME ROOT [ENV=V ...] -- ARGS... : one answer into $TMP/NAME.xml, a fresh cache per run, cwd = the fixture root
run(){
    local name="$1" root="$2"; shift 2
    local envs=()
    while [ "$#" -gt 0 ] && [ "$1" != "--" ]; do envs+=( "$1" ); shift; done
    shift
    ( cd "$root" || exit 1; env ${envs[@]+"${envs[@]}"} "$BIN" "$root" "$@" --cache="$TMP/$name.cache" >"$TMP/$name.xml" 2>"$TMP/$name.err" ) \
        || no "$name: ripwire exited non-zero ($( head -c 200 "$TMP/$name.err" | tr '\n' ' ' ))"
}

cat > "$TMP/chk.py" <<'PY'
import html, re, sys
cmd, args = sys.argv[1], sys.argv[2:]
def load(p): return open(p, "rb").read().decode("utf-8", "replace")
def rows(t): return re.findall(r'<d [^>]*>', t)
def attr(r, a):
    m = re.search(r'\b%s="([^"]*)"' % a, r or "")
    return html.unescape(m.group(1)) if m else None
if cmd == "ctor":      # FILE NAME LINE CALLS
    rs = rows(load(args[0]))
    if not rs or attr(rs[0], "n") != args[1] or attr(rs[0], "r") != "1": print("r=1 is %r" % (rs and attr(rs[0], "n"))); sys.exit(1)
    got = (attr(rs[0], "ctor_l"), attr(rs[0], "ctor_calls"), attr(rs[0], "ctor_calls_more"))
    if got != (args[2], args[3], None): print("got %r" % (got,)); sys.exit(1)
elif cmd == "none":    # FILE [R1NAME]: no row carries ctor_l=; the r=1 row is R1NAME (non-vacuous)
    t = load(args[0]); rs = rows(t)
    if len(args) > 1 and (not rs or attr(rs[0], "n") != args[1]): print("r=1 is %r, want %s" % (rs and attr(rs[0], "n"), args[1])); sys.exit(1)
    if "ctor_l=" in t: print("ctor_l= present"); sys.exit(1)
elif cmd == "clause":  # FILE 0|1
    t = load(args[0]); has = "d ctor_l= ctor_calls=" in t or "; ctor_l= on the r=1" in t
    if has != (args[1] == "1"): print("clause present=%s" % has); sys.exit(1)
    if has and "ctor_calls_more=" not in t.split("-->")[0]: print("clause does not define ctor_calls_more="); sys.exit(1)
elif cmd == "prefix":  # ON OFF
    on, off = load(args[0]), load(args[1])
    on = re.sub(r' ctor_l="\d+" ctor_calls="[^"]*"( ctor_calls_more="\d+")?', "", on)
    on = re.sub(r"; d ctor_l= ctor_calls= [^;]*?ctor_calls_more= cut", "", on)
    on = re.sub(r"; ctor_l= on the r=1 method row[^;]*?not a claim they answer", "", on)
    mask = lambda s: re.sub(r'est_tokens="\d+"', 'est_tokens=""', s)
    if mask(on) != mask(off): print("the answer changed beyond ctor_l=/ctor_calls="); sys.exit(1)
PY
chk(){ python3 "$TMP/chk.py" "$@"; }

run a1 "$FIX/js" RIPWIRE_NO_BODY_FOLD=1 -- "--for=$Q"
run a1f "$FIX/js" RIPWIRE_NO_BODY_FOLD=1 -- "--for=$Q" --legend=full
run a2 "$FIX/py" RIPWIRE_NO_BODY_FOLD=1 -- "--for=$Q"
run b1 "$FIX/noctor" RIPWIRE_NO_BODY_FOLD=1 -- "--for=$Q"
run b2 "$FIX/js" -- "--for=$Q"
run b3 "$FIX/js" RIPWIRE_NO_BODY_FOLD=1 -- "--for=$Q_CLS"
run f1 "$FIX/js" RIPWIRE_NO_BODY_FOLD=1 RIPWIRE_NO_CTOR_HOP=1 -- "--for=$Q"
run f1f "$FIX/js" RIPWIRE_NO_BODY_FOLD=1 RIPWIRE_NO_CTOR_HOP=1 -- "--for=$Q" --legend=full
run f2 "$FIX/js" RIPWIRE_NO_BODY_FOLD=1 -- "--for=create"
run f3 "$FIX/js" RIPWIRE_NO_BODY_FOLD=1 -- "--for=$Q" --json
run x1 "$FIX/js" RIPWIRE_NO_BODY_FOLD=1 -- "--for=$Q"

m="$( chk ctor "$TMP/a1.xml" create 10 makeStore,makeLogger )" && ok "A1 JS constructor: ctor_l=\"10\" ctor_calls=\"makeStore,makeLogger\"" || no "A1 $m"
m="$( chk ctor "$TMP/a2.xml" Registry.create 6 make_store )" && ok "A2 Python __init__: ctor_l=\"6\" ctor_calls=\"make_store\"" || no "A2 $m"
m="$( chk none "$TMP/b1.xml" create )" && ok "B1 no constructor: no ctor_l=" || no "B1 $m"
if grep -q '<b t="cls" l="9" p="store.js" n="Registry">' "$TMP/b2.xml"; then
    m="$( chk none "$TMP/b2.xml" create )" && ok "B2 the fold serves the class body: no ctor_l=" || no "B2 $m"
else
    no "B2 vacuous: the fold did not serve Registry"
fi
m="$( chk none "$TMP/b3.xml" Registry )" && ok "B3 r=1 is the class: no ctor_l=" || no "B3 $m"
m="$( chk none "$TMP/f1.xml" )" && m2="$( chk clause "$TMP/f1.xml" 0 )" && ok "F1 RIPWIRE_NO_CTOR_HOP=1: none, no clause" || no "F1 $m ${m2:-}"
m="$( chk prefix "$TMP/a1.xml" "$TMP/f1.xml" )" && ok "F1 prefix identity (compact)" || no "F1 compact $m"
m="$( chk prefix "$TMP/a1f.xml" "$TMP/f1f.xml" )" && ok "F1 prefix identity (full legend)" || no "F1 full $m"
m="$( chk none "$TMP/f2.xml" )" && ok "F1 name-exact --for=create carries none" || no "F1 name-exact $m"
m="$( chk none "$TMP/f3.xml" )" && ok "F1 --json carries none" || no "F1 json $m"
m="$( chk clause "$TMP/a1.xml" 1 )" && m2="$( chk clause "$TMP/a1f.xml" 1 )" && ok "L1 the clause rides in both dialects" || no "L1 $m ${m2:-}"
m="$( chk clause "$TMP/b1.xml" 0 )" && ok "L2 no ctor_l=: no clause" || no "L2 $m"
cmp -s "$TMP/a1.xml" "$TMP/x1.xml" && ok "X1 determinism" || no "X1 two runs differ"
if command -v xmllint >/dev/null 2>&1; then
    bad=0; for f in a1 a1f a2 b1 b2 b3; do xmllint --noout "$TMP/$f.xml" 2>/dev/null || { bad=1; no "X2 $f.xml not well-formed"; }; done
    [ "$bad" = 0 ] && ok "X2 xmllint well-formed"
else
    echo "  SKIP  X2 xmllint not installed"
fi
[ "$fail" = 0 ] && echo "forctorhopcheck: PASS" || echo "forctorhopcheck: FAIL"
exit "$fail"
