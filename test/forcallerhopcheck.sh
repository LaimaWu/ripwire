#!/usr/bin/env bash
# forcallerhopcheck.sh — THE CALLER HOP of a name-exact --for answer about ONE function (lane mf1-nx; src/forcallerhop.h).
#
# THE CONTRACT THIS PINS (arms tagged [P] are RED on the pre-change binary 27eddee1; [G] arms are twins and near-miss negatives
# that hold on both binaries and must keep holding — PROCESS rule 4, no gate erosion):
#
#   TRIGGER  CLI --for (XML, compact and full legends) on the name-exact route whose body candidates are ONE function (every
#            candidate a function or method of the same name and scope), in the DEFAULT regime.
#   WHAT     <callers of= shown= total= capped= [d2= d2_shown=] next="--impact=OF"> after the tail, before <bodies>: the
#            callers verb's 1-hop rows <s t= n= [sc=] p=file:line [via="name"] [sites_at=]>, then d="2" rows (callers of the
#            shown callers, at most 4); 16 first-hop rows at most, the rest counted (total=, capped="1").
#   NOT      a struct, a variable, a conceptual question, --json, --signatures-only, --detail=1, --token-budget=N and
#            RIPWIRE_NO_CALLER_HOP=1 carry none (byte-identical to the hop-off answer).
#
# Arms:
#   P1 [P] --for=start_server: <callers> lists connect_client and run_client with sites_at=, main as d="2", next=--impact=start_server
#   P2 [P] the 20-caller fan: shown="16" total="20" capped="1"
#   P3 [P] a C++ method (Canvas::Repaint) carries its caller, of= qualified; [G] --for=Draw bodies two classes' Draw: no hop
#   P8 [P] a caller matched by name alone (Python pool.handle_job) carries via="name"
#   P4 [P] the clause rides in both legends when the hop does (compact and full), and defines every attribute the element carries
#   P5 [G] prefix identity: the answer minus <callers> and its clause, est_tokens masked, equals RIPWIRE_NO_CALLER_HOP=1 (both legends)
#   P6 [P] est_tokens(hop) > est_tokens(off)
#   P7 [G] next= pasted lists every caller (the --impact answer names connect_client, run_client and main)
#   N1 [G] no hop on a struct (ServerConfig), a variable (g_backlog_limit), a conceptual question
#   N2 [G] no hop, byte-identical to RIPWIRE_NO_CALLER_HOP=1: --json, --signatures-only, --detail=1, --token-budget=3000
#   X1 [G] determinism  X2 [G] xmllint well-formed  X3 [G] no "--" inside a legend comment
#
# Usage:  bash test/forcallerhopcheck.sh [BIN]   |   RIPWIRE_BIN=asan/ripwire bash test/forcallerhopcheck.sh
# BIN is $1, else RIPWIRE_BIN, else build/ripwire; the first output line names the binary used. Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_CALLER_HOP
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "forcallerhopcheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
cd "$ROOT"
FC="test/forcallerhopfix/c"
FX="test/forcallerhopfix/cpp"
[ -f "$FC/server.c" ] && [ -f "$FX/widget.cc" ] || { echo "fixture test/forcallerhopfix missing"; exit 2; }

TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
export HOME="$TMP/home"; mkdir -p "$HOME"

run(){ local dir="$1"; shift; "$BIN" "$dir" --no-cache "$@" 2>/dev/null; }
hop(){ python3 - "$1" <<'PY'
import re, sys
t = open(sys.argv[1], encoding="utf-8", errors="replace").read()
m = re.search(r'<callers [^>]*>.*?</callers>', t, re.S)
print(m.group(0) if m else "")
PY
}
strip(){ python3 - "$1" <<'PY'
import re, sys
t = open(sys.argv[1], encoding="utf-8", errors="replace").read()
t = re.sub(r'<callers [^>]*>.*?</callers>', '', t, flags=re.S)
t = re.sub(r'; callers of=:.*?next=(?: lists)? the (?:whole caller )?radius', '', t, flags=re.S)
t = re.sub(r' est_tokens="\d+"', '', t)
sys.stdout.write(t)
PY
}
est(){ grep -o ' est_tokens="[0-9]*"' "$1" | head -1 | grep -o '[0-9][0-9]*'; }

# ── P1 ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$FC" --for=start_server >"$TMP/p1"
H1="$( hop "$TMP/p1" )"
if [ -s "$TMP/p1" ] && grep -q 'route="name-exact(' "$TMP/p1"; then ok "(P1pre) --for=start_server routes name-exact"; else no "(P1pre) --for=start_server did not route name-exact — the fixture proves nothing"; fi
case "$H1" in
    *'n="connect_client" p="client.c:3" sites_at="client.c:5"'*'next="--impact=start_server"'*|*'next="--impact=start_server"'*'n="connect_client" p="client.c:3" sites_at="client.c:5"'*) ok "(P1a) connect_client is a caller row with its call site" ;;
    *) no "(P1a) no connect_client caller row with sites_at=client.c:5: [$H1]" ;;
esac
printf '%s' "$H1" | grep -q 'n="run_client" p="client.c:8" sites_at="client.c:13"' && ok "(P1b) run_client is a caller row" || no "(P1b) no run_client caller row: [$H1]"
printf '%s' "$H1" | grep -q '<s t="fn" n="main" p="main.c:3" d="2"/>' && ok "(P1c) main is the d=2 row" || no "(P1c) main is not a d=2 row: [$H1]"
printf '%s' "$H1" | grep -q '<callers of="start_server" shown="2" total="2" capped="0" d2="1" d2_shown="1" next="--impact=start_server">' \
    && ok "(P1d) the element's counts and next=" || no "(P1d) the element's open tag: [${H1%%>*}>]"
printf '%s' "$H1" | grep -q 'n="listen_socket"' && no "(P1e) a CALLEE (listen_socket) rode as a caller" || ok "(P1e) no callee in the caller rows"
# placement: after the tail, before the bodies
python3 - "$TMP/p1" <<'PY' && ok "(P1f) <callers> sits after <tail> and before <bodies>" || no "(P1f) <callers> is not between <tail> and <bodies>"
import sys; t = open(sys.argv[1]).read(); c = t.find("<callers "); b = t.find("<bodies "); tl = t.find("</tail>")
sys.exit(0 if 0 <= tl < c < b else 1)
PY

# ── P2: the fan ──────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$FC" --for=fan_target >"$TMP/p2"
H2="$( hop "$TMP/p2" )"
printf '%s' "$H2" | grep -q '<callers of="fan_target" shown="16" total="20" capped="1" next="--impact=fan_target">' \
    && ok "(P2) 20 callers: shown=16 total=20 capped=1, next= serves the rest" || no "(P2) the fan's open tag: [${H2%%>*}>]"
n2="$( printf '%s' "$H2" | grep -o '<s ' | wc -l | tr -d ' ' )"
[ "$n2" = "16" ] && ok "(P2b) exactly 16 rows printed" || no "(P2b) $n2 rows printed, expected 16"

# ── P3: entity identity — a same-named method of ANOTHER class is a different function ─────────────────────────────────────
run "$FX" --for=Repaint >"$TMP/p3"
H3="$( hop "$TMP/p3" )"
printf '%s' "$H3" | grep -q '<callers of="Canvas::Repaint" shown="1" total="1"' && printf '%s' "$H3" | grep -q 'n="RefreshCanvas" p="canvas.cc:10" sites_at="canvas.cc:12"' \
    && ok "(P3a) a C++ method (Canvas::Repaint): its caller RefreshCanvas rides, of= qualified" || no "(P3a) Canvas::Repaint's hop: [$H3]"
run "$FX" --for=Draw >"$TMP/p3d"
if grep -q 'route="name-exact(Draw)' "$TMP/p3d" && [ "$( grep -o '<b t="method"' "$TMP/p3d" | wc -l | tr -d ' ' )" = "2" ]; then
    grep -q '<callers ' "$TMP/p3d" && no "(P3b) --for=Draw bodies Widget::Draw AND Gadget::Draw (two functions) yet a hop rode" || ok "(P3b) two same-named methods of two classes are not ONE function: no hop"
else
    no "(P3b-pre) --for=Draw no longer bodies the two Draw methods — re-author the arm"
fi
# ── P8: the by-name hedge, as the callers verb decides it ──────────────────────────────────────────────────────────────────────
run "test/forcallerhopfix/py" --for=handle_job >"$TMP/p8"
H8="$( hop "$TMP/p8" )"
printf '%s' "$H8" | grep -q 'n="dispatch_jobs" p="worker.py:6" via="name" sites_at="worker.py:8"' && ok "(P8) a receiver-unproven caller carries via=\"name\"" || no "(P8) the by-name caller row: [$H8]"

# ── P4: legend coverage, both dialects ───────────────────────────────────────────────────────────────────────────────────────
run "$FC" --for=start_server --legend=full >"$TMP/p4f"
for f in "$TMP/p1" "$TMP/p4f"; do
    python3 - "$f" <<'PY' && ok "(P4) $(basename "$f"): the clause rides and defines every attribute of <callers> and its rows" || no "(P4) $(basename "$f"): clause missing or an attribute undefined"
import re, sys
t = open(sys.argv[1]).read()
leg = " ".join(re.findall(r'<!--(.*?)-->', t, re.S))
m = re.search(r'(; callers of=:.*?)(?:;|$)', leg, re.S)
if not m or "<callers " not in t: sys.exit(1)
clause = leg[leg.find("; callers of="):]
el = re.search(r'<callers [^>]*>.*?</callers>', t, re.S).group(0)
names = set(re.findall(r'\s(\w+)="', el))
missing = [a for a in names if not re.search(r'(?<![\w])' + a + r'=', clause)]
sys.exit(1 if missing else 0)
PY
done
run "$FC" --for=g_backlog_limit >"$TMP/n1v"
grep -q '; callers of=' "$TMP/n1v" && no "(P4n) the clause rode without the element" || ok "(P4n) no clause on an answer without the hop"

# ── P5 / P6: prefix identity and pricing ─────────────────────────────────────────────────────────────────────────────────────
for lg in compact full; do
    run "$FC" --for=start_server --legend=$lg >"$TMP/on.$lg"
    RIPWIRE_NO_CALLER_HOP=1 run "$FC" --for=start_server --legend=$lg >"$TMP/off.$lg"
    strip "$TMP/on.$lg" >"$TMP/on.s"; strip "$TMP/off.$lg" >"$TMP/off.s"
    if [ -s "$TMP/off.s" ] && cmp -s "$TMP/on.s" "$TMP/off.s"; then ok "(P5) $lg: the answer minus the hop equals RIPWIRE_NO_CALLER_HOP=1"; else no "(P5) $lg: the hop changed bytes outside itself"; fi
done
e1="$( est "$TMP/on.compact" )"; e0="$( est "$TMP/off.compact" )"
if [ -n "$e1" ] && [ -n "$e0" ] && [ "$e1" -gt "$e0" ]; then ok "(P6) est_tokens prices the hop ($e0 -> $e1)"; else no "(P6) est_tokens did not grow: off=$e0 on=$e1"; fi

# ── P7: next= pasted ─────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$FC" --impact=start_server >"$TMP/p7"
if grep -q 'n="connect_client"' "$TMP/p7" && grep -q 'n="run_client"' "$TMP/p7" && grep -q 'n="main"' "$TMP/p7"; then ok "(P7) next=--impact=start_server lists every caller"; else no "(P7) the next= call does not list every caller"; fi

# ── N1: near misses ──────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$FC" --for=ServerConfig >"$TMP/n1s"
for f in n1s n1v; do
    if [ -s "$TMP/$f" ] && ! grep -q '<callers ' "$TMP/$f"; then ok "(N1) no hop: $f"; else no "(N1) a hop rode (or no answer): $f"; fi
done
run "$FC" --for="how does the client start the server and listen on a socket" >"$TMP/n1c"
if [ -s "$TMP/n1c" ] && ! grep -q '<callers ' "$TMP/n1c"; then ok "(N1c) no hop on a conceptual question"; else no "(N1c) a hop rode on a conceptual question"; fi

# ── N2: postures that never carry it ─────────────────────────────────────────────────────────────────────────────────────────
for extra in --json --signatures-only --detail=1 --token-budget=3000; do
    run "$FC" --for=start_server "$extra" >"$TMP/n2.on"
    RIPWIRE_NO_CALLER_HOP=1 run "$FC" --for=start_server "$extra" >"$TMP/n2.off"
    if [ -s "$TMP/n2.on" ] && cmp -s "$TMP/n2.on" "$TMP/n2.off" && ! grep -q '<callers \|"callers"' "$TMP/n2.on"; then ok "(N2) $extra: no hop, byte-identical"; else no "(N2) $extra: a hop rode or bytes moved"; fi
done

# ── X ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
run "$FC" --for=start_server >"$TMP/x2"
cmp -s "$TMP/p1" "$TMP/x2" && ok "(X1) deterministic" || no "(X1) two runs differ"
if command -v xmllint >/dev/null 2>&1; then
    for f in p1 p2 p3 p4f p8; do xmllint --noout "$TMP/$f" 2>/dev/null && ok "(X2) well-formed: $f" || no "(X2) xmllint rejects $f"; done
else
    printf '  SKIP  (X2) xmllint not installed\n'
fi
python3 - "$TMP/p1" "$TMP/p4f" <<'PY' && ok "(X3) no -- inside a legend comment" || no "(X3) a legend comment holds --"
import re, sys
for p in sys.argv[1:]:
    for c in re.findall(r'<!--(.*?)-->', open(p).read(), re.S):
        if "--" in c: sys.exit(1)
PY

exit $fail
