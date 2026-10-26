#!/usr/bin/env bash
# epmanifestcheck.sh — MANIFEST ENTRY POINTS on a --for answer (lane ep-manifest; src/epmanifest.h; CONTRIBUTING family 10 slice 10a).
#
# THE CONTRACT THIS PINS (arms tagged [P] are RED on the pre-change binary 681b49e5; [G] arms are twins and near-miss negatives
# that hold on both binaries and must keep holding — PROCESS rule 4, no gate erosion):
#
#   TRIGGER  CLI --for (XML, default regime) whose ranked head shows a Python def that a manifest entry point names.
#   WHAT     <ep p= l= group= [used_at= used_in= used_total=]><e n= target= def=|external=1|unresolved=1/>…</ep> after the tail.
#   NOT      a non-manifest file, --json, --token-budget=N, --signatures-only, RIPWIRE_NO_EP_MANIFEST=1 carry none.
#
# Arms:
#   C1 [P] setup.cfg inline group `x.report = fmt = pkg.mod:Fmt`: <ep p="setup.cfg" l="5" group="x.report"> with
#          e n="fmt" target="pkg.mod:Fmt" def="pkg/mod.py:1"
#   C2 [P] two consumers, both listed, hedged by text: used_at = pkg/loader.py:6 and pkg/second.py:2, used_in = load reload_reports
#   C3 [G] near-miss consumers are NOT listed: a comment, `name == "x.report"` (not a .group), a producer `_ep(group="x.report")`
#   C4 [P] entry_points(group="y.other") is a consumer (load_other); duplicate entry names across groups: two groups each with n="fmt",
#          distinct targets (pkg.mod:Fmt / pkg.mod:Other)
#   C5 [P] unresolved: `bad = pkg.mod:Missing` carries unresolved="1"; external: `ext = otherpkg.thing:Thing` carries external="1"
#   C6 [P] a group with no consumer (z.quiet) has a row and NO used_at= (no guess)
#   P1 [P] pyproject variant (src layout): [project.entry-points."x.report"] → def="src/pkg/mod.py:1", used_at=src/pkg/loader.py:5
#          (`.select(group=…)`); [project.scripts] → group="console_scripts", def="src/pkg/cli.py:1", no used_at=
#   S1 [P] siblings: a `key: value` group line, a dotted method target (plug.mod:Fmt.render → the method, not the same-named
#          top-level render), a package target (plug/sub/__init__.py), iter_entry_points("g"), entry_points()["g"] and a later group= keyword (entry_points(name=, group=)) consumers;
#          [G] m.get("g") on a non-entry_points receiver is not a consumer
#   K1 [P] the runaway guards disclose: a 13-entry group shows 12 with entries_capped="1" entries_total="13"; 4 consumers show 3
#          with used_capped="1" used_total="4"
#   N1 [G] a non-manifest .cfg (config.cfg, same text): no <ep>
#   N2 [G] no <ep>, byte-identical to RIPWIRE_NO_EP_MANIFEST=1: --json, --token-budget=3000, --signatures-only
#   N3 [G] prefix identity: the answer minus <ep> and its clause, est_tokens masked, equals RIPWIRE_NO_EP_MANIFEST=1
#   L1 [P] the clause rides (compact and full) and defines every attribute <ep>/<e> carry
#   X1 [G] determinism  X2 [G] xmllint well-formed (when xmllint exists)  X3 [G] no "--" inside the legend comment
#
# Usage:  bash test/epmanifestcheck.sh [BIN]   |   RIPWIRE_BIN=asan/ripwire bash test/epmanifestcheck.sh
# BIN is $1, else RIPWIRE_BIN, else build/ripwire; the first output line names the binary used. Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_EP_MANIFEST
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "epmanifestcheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
verdict(){ if [ "$1" -eq 0 ]; then ok "$2"; else no "$3"; fi; }   # rc, the PASS line, the FAIL line

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
FX="$ROOT/test/epmanifestfix"
[ -f "$FX/cfg/setup.cfg" ] && [ -f "$FX/pyp/pyproject.toml" ] && [ -f "$FX/negcfg/config.cfg" ] && [ -f "$FX/sib/setup.cfg" ] || { echo "fixture test/epmanifestfix missing"; exit 2; }

TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
export HOME="$TMP/home"; mkdir -p "$HOME"

Q='--for=how is the Fmt report formatter registered and loaded'
# run DIR OUT [args...]: the answer (stdout) and its exit code (OUT.rc); a run that fails is a FAIL, never an absence
run(){ local dir="$1" out="$2"; shift 2; ( cd "$dir" || exit 97; "$BIN" "$dir" --no-cache "$@" >"$out" 2>"$out.err" ); echo $? >"$out.rc"; }
okrun(){ [ "$(cat "$1.rc")" = "0" ] && [ -s "$1" ]; }

run "$FX/cfg" "$TMP/cfg" "$Q"
run "$FX/cfg" "$TMP/cfg2" "$Q"
run "$FX/cfg" "$TMP/cfgfull" "$Q" --legend=full
RIPWIRE_NO_EP_MANIFEST=1 run "$FX/cfg" "$TMP/cfgoff" "$Q"
run "$FX/pyp" "$TMP/pyp" '--for=how is the Fmt report formatter and the pkgtool main command registered'
run "$FX/negcfg" "$TMP/neg" "$Q"
run "$FX/sib" "$TMP/sib" '--for=how are the report formatter render and init hooks registered'
for a in cfg cfg2 cfgfull cfgoff pyp neg sib; do okrun "$TMP/$a" || no "run $a failed (rc=$(cat "$TMP/$a.rc"))"; done

# eps FILE -> one JSON line: [{attrs..., "e": [{attrs}]}] of every <ep> element
eps(){ python3 - "$1" <<'PY'
import json, re, sys
t = open(sys.argv[1], encoding="utf-8", errors="replace").read()
t = re.sub(r"<!--.*?-->", "", t, flags=re.S)
out = []
for m in re.finditer(r"<ep( [^>]*)>(.*?)</ep>", t, flags=re.S):
    d = dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))
    d["e"] = [dict(re.findall(r'(\w+)="([^"]*)"', e)) for e in re.findall(r"<e( [^>]*)/>", m.group(2))]
    out.append(d)
print(json.dumps(out))
PY
}
CFG="$( eps "$TMP/cfg" )"; PYP="$( eps "$TMP/pyp" )"; NEG="$( eps "$TMP/neg" )"; SIB="$( eps "$TMP/sib" )"
has(){ python3 -c "import json,sys; d=json.loads(sys.argv[1]); sys.exit(0 if ($2) else 1)" "$1"; }
grp(){ echo "[g for g in d if g.get('group')=='$1']"; }

# C1
if has "$CFG" "any(g['p']=='setup.cfg' and g['l']=='5' and any(e=={'n':'fmt','target':'pkg.mod:Fmt','def':'pkg/mod.py:1'} for e in g['e']) for g in $(grp x.report))"; then
    ok "C1 setup.cfg inline group links fmt -> pkg/mod.py:1 Fmt"; else no "C1 setup.cfg x.report row: $CFG"; fi
# C2 + C3
if has "$CFG" "any(g.get('used_at')=='pkg/loader.py:6 pkg/second.py:2' and g.get('used_in')=='load reload_reports' and 'used_total' not in g for g in $(grp x.report))"; then
    ok "C2/C3 both real consumers listed; comment, name==, producer call not listed"; else no "C2/C3 x.report used_at: $CFG"; fi
# C4
if has "$CFG" "any(g.get('used_at')=='pkg/loader.py:22' and g.get('used_in')=='load_other' for g in $(grp y.other)) and [e['target'] for g in d for e in g['e'] if e['n']=='fmt']==['pkg.mod:Fmt','pkg.mod:Other']"; then
    ok "C4 entry_points(group=) consumer; duplicate name fmt in two groups, distinct targets"; else no "C4 y.other / duplicate names: $CFG"; fi
# C5
if has "$CFG" "any(e=={'n':'bad','target':'pkg.mod:Missing','unresolved':'1'} for g in d for e in g['e']) and any(e=={'n':'ext','target':'otherpkg.thing:Thing','external':'1'} for g in d for e in g['e'])"; then
    ok "C5 unresolved=1 for a missing attribute, external=1 for a module outside the tree"; else no "C5 unresolved/external: $CFG"; fi
# C6
if has "$CFG" "len($(grp z.quiet))==1 and 'used_at' not in $(grp z.quiet)[0] and 'used_in' not in $(grp z.quiet)[0]"; then
    ok "C6 a group with no consumer carries no used_at= (no guess)"; else no "C6 z.quiet: $CFG"; fi
# P1
if has "$PYP" "any(g['p']=='pyproject.toml' and g.get('used_at')=='src/pkg/loader.py:5' and g.get('used_in')=='load_reports' and g['e']==[{'n':'fmt','target':'pkg.mod:Fmt','def':'src/pkg/mod.py:1'}] for g in $(grp x.report)) and any(g['e']==[{'n':'pkgtool','target':'pkg.cli:main','def':'src/pkg/cli.py:1'}] and 'used_at' not in g for g in $(grp console_scripts))"; then
    ok "P1 pyproject entry-points + project.scripts (src layout), select(group=) consumer"; else no "P1 pyproject: $PYP"; fi
# S1
if has "$SIB" "any(g['l']=='2' and g.get('used_at')=='plug/use.py:5' and g['e']==[{'n':'m','target':'plug.mod:Fmt.render','def':'plug/mod.py:3'}] for g in $(grp x.hooks)) and any(g.get('used_at')=='plug/use.py:7 plug/use.py:31' and g.get('used_in')=='go by_name' and {'n':'i','target':'plug.sub:Init','def':'plug/sub/__init__.py:1'} in g['e'] for g in $(grp x.pkg))"; then
    ok "S1 key: group line, dotted method target, package target, iter_entry_points / entry_points()[] consumers, m.get not"; else no "S1 siblings: $SIB"; fi
# K1
if has "$SIB" "any(len(g['e'])==12 and g.get('entries_capped')=='1' and g.get('entries_total')=='13' and g.get('used_capped')=='1' and g.get('used_total')=='4' and len(g['used_at'].split())==3 for g in $(grp x.many))"; then
    ok "K1 entry and consumer caps disclose *_capped=1 with the total"; else no "K1 caps: $SIB"; fi
# N1
[ "$NEG" = "[]" ]; verdict $? "N1 a non-manifest .cfg yields no <ep>" "N1 config.cfg produced rows: $NEG"
# N2
for v in --json --token-budget=3000 --signatures-only; do
    run "$FX/cfg" "$TMP/n2on" "$Q" "$v"; RIPWIRE_NO_EP_MANIFEST=1 run "$FX/cfg" "$TMP/n2off" "$Q" "$v"
    if okrun "$TMP/n2on" && okrun "$TMP/n2off" && cmp -s "$TMP/n2on" "$TMP/n2off" && ! grep -q '<ep ' "$TMP/n2on"; then ok "N2 $v: no <ep>, byte-identical to off"; else no "N2 $v differs or carries <ep>"; fi
done
# N3 prefix identity (compact legend)
python3 - "$TMP/cfg" "$TMP/cfgoff" <<'PY'; verdict $? "N3 answer minus <ep> and its clause equals RIPWIRE_NO_EP_MANIFEST=1 (est_tokens masked)" "N3 prefix identity broken"
import re, sys
a = open(sys.argv[1]).read(); b = open(sys.argv[2]).read()
a = re.sub(r"<ep [^>]*>.*?</ep>", "", a, flags=re.S)
a = re.sub(r"; ep p= l= group=:.*?groups_total= if cut", "", a, flags=re.S)
m = lambda s: re.sub(r'est_tokens="\d+"', 'est_tokens="N"', s)
sys.exit(0 if m(a) == m(b) and a != open(sys.argv[1]).read() else 1)
PY
# L1
for f in cfg cfgfull; do
python3 - "$TMP/$f" <<'PY'; verdict $? "L1 $f: the clause rides and defines every <ep>/<e> attribute" "L1 $f: clause missing or an attribute undefined"
import re, sys
t = open(sys.argv[1]).read()
leg = " ".join(re.findall(r"<!--(.*?)-->", t, flags=re.S))
if "; ep p= l= group=:" not in leg: sys.exit(1)
attrs = set()
for el in re.findall(r"<(?:ep|e) ([^>]*?)/?>", re.sub(r"<!--.*?-->", "", t, flags=re.S)):
    attrs |= set(re.findall(r'(\w+)="', el))
sys.exit(0 if all((a + "=") in leg for a in attrs) else 1)
PY
done
# X1 X2 X3
cmp -s "$TMP/cfg" "$TMP/cfg2"; verdict $? "X1 deterministic" "X1 two runs differ"
if command -v xmllint >/dev/null 2>&1; then
    for f in cfg cfgfull pyp; do xmllint --noout "$TMP/$f" 2>/dev/null; verdict $? "X2 $f well-formed" "X2 $f not well-formed"; done
else
    echo "  SKIP  X2 xmllint not installed"
fi
python3 - "$TMP/cfg" "$TMP/cfgfull" <<'PY'; verdict $? "X3 no '--' inside a legend comment" "X3 '--' inside a legend comment"
import re, sys
for f in sys.argv[1:]:
    for c in re.findall(r"<!--(.*?)-->", open(f).read(), flags=re.S):
        if "--" in c: sys.exit(1)
PY

[ "$fail" -eq 0 ] && echo "epmanifestcheck: PASS" || echo "epmanifestcheck: FAIL"
exit "$fail"
