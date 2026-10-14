#!/usr/bin/env bash
# forsoftbudgetcheck.sh — the --for lens stops when its answer is complete; an explicit --token-budget on the
# answer-shaping lenses is a SOFT TARGET (owner decisions 2026-10-14). Written RED-FIRST, before the code.
#
# WHAT CHANGES, and the contract every arm below pins:
#
#   DEFAULT --for (no budget). The ranked head the lens selected (<sigs total=>, i.e. the relevance floor
#   intersected with kForLensDefaultTopN or --pack-top-n) IS the answer. kForPayloadBudgetBytes stops being a
#   ceiling and becomes a stair-step TARGET: the trim ladder's shrink steps (A..E: shorter signatures, doc
#   excerpts) still run against it, but its row-drop step (F) runs only past a RUNAWAY GUARD
#   (kForRunawayGuardBytes, far above a typical answer). So:
#     (D1) a default answer serves every row of its head: shown == total;
#     (D2) those rows, in rank order, are the rows an uncapped call (--signatures-only at a huge target) serves;
#     (D3) rows served past the target are disclosed — <sigs past_target="K"> and a legend clause defining it —
#          and the legend no longer calls budget_bytes= a ceiling on that answer; no over_ceiling= (a target
#          cannot be "over");
#     (D4) hitting the runaway guard is the only default-regime cut: over_ceiling="1" on the root, the <sigs>
#          cut carries next=, the legend names the guard, and the <sigs> block stays within the guard;
#     (D5) the MCP `for` twin serves its whole head too.
#
#   EXPLICIT --token-budget=N / MCP budget_tokens on --for and --pack-task. Rows stop being added once the
#   target is reached; the rank 1..4 floor, the header and every disclosure always ship; there is no byte
#   promise, so nothing is labelled over a ceiling and nothing in the header is dropped to fit:
#     (E1) no over_ceiling= at a tight target; the floor rows ship; the <sigs> cut carries next=;
#     (E2) no legend clause is dropped "(ceiling)" at any target;
#     (E3) the served rows are a rank PREFIX that grows with N, and a prefix of the default answer's rows
#          (a target never selects different rows, only fewer);
#     (E4) the compact and full dialects serve the same rows at every target (rows are charged, not headers);
#     (E5) stop when complete: a target wider than the answer serves exactly the default answer's rows —
#          it neither pads nor caps the head below the default;
#     (E6) MCP `for` at a tight budget_tokens: no over_ceiling=, next_budget_tokens= still names the continuation;
#     (P1) --pack-task at a tight target: no over_ceiling=.
#
#   UNCHANGED (twin, must hold on base and head): the exit-3 CI-gate personality of --token-budget on the
#   default map (G1): over budget -> exit 3, the map withheld whole (a withheld="1" record, no rows).
#
# Usage:  bash test/forsoftbudgetcheck.sh [BIN]   |   RIPWIRE_BIN=asan/ripwire bash test/forsoftbudgetcheck.sh
# Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
cd "$ROOT"
echo "forsoftbudgetcheck: BIN=$BIN"

TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
CORPUS="src"                                    # short relative root (CHECKLIST 22): header bytes shift with the root path
Q="rank symbols by pagerank"                    # conceptual: a flat 40-row head whose default answer the 7.5 KB ladder cuts
CF="--cache=$TMP/c_for"; CP="--cache=$TMP/c_pack"; CM="--cache=$TMP/c_map"   # one cache per verb class

# the runaway guard, read from source (absent on a binary that predates it: D4's byte bound then FAILS, never skips)
GUARD="$( sed -n 's/^inline constexpr std::size_t kForRunawayGuardBytes *= *\([0-9]*\);.*/\1/p' src/serialize.h )"

# ── helpers ──────────────────────────────────────────────────────────────────────────────────────────────
cat > "$TMP/rows.py" <<'PY'
# the served <sigs> rows as "p:l:n", in RANK order (r=), one per line; exit 3 when there is no <sigs> block
import re, sys
s = sys.stdin.read()
m = re.search( r'<sigs[^>]*>(.*?)</sigs>', s, re.S )
if not m: sys.exit( 3 )
rows = []
for d in re.finditer( r'<d ([^>]*)>', m.group( 1 ) ):
    a = d.group( 1 )
    g = lambda k: ( re.search( r'\b' + k + r'="([^"]*)"', a ) or [ None, "?" ] )[ 1 ]
    r = g( "r" )
    rows.append( ( int( r ) if r.isdigit() else 10**9, g( "p" ) + ":" + g( "l" ) + ":" + g( "n" ) ) )
rows.sort()
print( "\n".join( x[ 1 ] for x in rows ) )
PY
cat > "$TMP/attr.py" <<'PY'
# attr.py ELEMENT NAME: the value of NAME on the first <ELEMENT ...> open tag ("" when absent; exit 3 when no such tag)
import re, sys
el, name = sys.argv[ 1 ], sys.argv[ 2 ]
s = sys.stdin.read()
m = re.search( r'<' + el + r'\b([^>]*)>', s )
if not m: sys.exit( 3 )
v = re.search( r'\b' + name + r'="([^"]*)"', m.group( 1 ) )
print( v.group( 1 ) if v else "" )
PY
cat > "$TMP/prefix.py" <<'PY'
# prefix.py A B: exit 0 when the lines of file A are a prefix of the lines of file B
import sys
a = open( sys.argv[ 1 ] ).read().split( "\n" ); b = open( sys.argv[ 2 ] ).read().split( "\n" )
a = [ x for x in a if x ]; b = [ x for x in b if x ]
sys.exit( 0 if a == b[ :len( a ) ] else 1 )
PY
cat > "$TMP/mcptext.py" <<'PY'
import sys, json
for line in sys.stdin:
    line = line.strip()
    if not line: continue
    try: d = json.loads( line )
    except Exception: continue
    c = d.get( "result", {} ).get( "content" )
    if c: print( c[ 0 ].get( "text", "" ) )
PY
cli_for(){ "$BIN" "$CORPUS" --for="$Q" "$@" "$CF" 2>/dev/null; }
mcp_for(){ printf '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"for","arguments":{"path":"%s","task":"%s"%s}}}\n' \
                  "$CORPUS" "$Q" "${1:-}" | "$BIN" --mcp 2>/dev/null | python3 "$TMP/mcptext.py"; }
attr(){ python3 "$TMP/attr.py" "$1" "$2" < "$3"; }
rows(){ python3 "$TMP/rows.py" < "$1" > "$2"; }
isnum(){ case "${1:-}" in ''|*[!0-9]*) return 1;; esac; return 0; }

# ── the default answer (premise for D1..D3, E3, E5) ──────────────────────────────────────────────────────
cli_for > "$TMP/def.xml"; rc=$?
if [ "$rc" -ne 0 ] || ! rows "$TMP/def.xml" "$TMP/def.rows"; then
    no "premise: default --for on $CORPUS did not run (rc=$rc) or served no <sigs> — every arm below would measure nothing"
    echo "FAILURES ABOVE"; exit 1
fi
DS="$( attr sigs shown "$TMP/def.xml" )"; DT="$( attr sigs total "$TMP/def.xml" )"
DN="$( grep -c . "$TMP/def.rows" )"
if ! isnum "$DT" || [ "$DT" -lt 30 ]; then
    no "premise: the default head should be a wide one (total>=30) so the old 7.5 KB ladder cut it; total='$DT'"
fi

# (D1) the whole head ships
if isnum "$DT" && [ "$DN" -eq "$DT" ] && { [ -z "$DS" ] || [ "$DS" = "$DT" ]; }; then
    ok "(D1) default answer serves its whole head: $DN rows of total=$DT"
else
    no "(D1) default answer served $DN rows (shown='$DS') of total='$DT' — rows of the ranked head were dropped below the runaway guard"
fi

# (D2) the default rows ARE the uncapped rows, in rank order
cli_for --signatures-only --token-budget=1000000 > "$TMP/unc.xml"
if rows "$TMP/unc.xml" "$TMP/unc.rows" && cmp -s "$TMP/def.rows" "$TMP/unc.rows"; then
    ok "(D2) default rows == uncapped rows ($( grep -c . "$TMP/unc.rows" ) rows, rank order)"
else
    no "(D2) default rows ($DN) differ from the uncapped call's rows ($( grep -c . "$TMP/unc.rows" 2>/dev/null ))"
fi

# (D3) past-target disclosure on that answer
PT="$( attr sigs past_target "$TMP/def.xml" )"; OC="$( attr ctx over_ceiling "$TMP/def.xml" )"
if isnum "$PT" && [ "$PT" -gt 0 ] && [ "$PT" -le "$DN" ]; then
    ok "(D3) <sigs past_target=\"$PT\"> names the rows served past the default target"
else
    no "(D3) <sigs> carries no past_target=K (got '$PT') although the answer passes the 7.5 KB target"
fi
if grep -q 'past_target=' <( sed -n '1,/<sigs/p' "$TMP/def.xml" | sed 's/<sigs.*//' ); then
    ok "(D3) the header legend defines past_target="
else
    no "(D3) no legend clause defines past_target= ahead of <sigs>"
fi
if grep -q 'default BYTE ceiling this ranked payload' "$TMP/def.xml"; then
    no "(D3) the legend still calls budget_bytes= a ceiling on an answer that passed it by design"
else
    ok "(D3) the legend does not call budget_bytes= a ceiling on a past-target answer"
fi
if [ -z "$OC" ]; then ok "(D3) no over_ceiling= on a default answer under the guard"
else no "(D3) over_ceiling='$OC' on a default answer that is under the runaway guard (a target cannot be 'over')"; fi

# (D4) the runaway guard: a 400-row head runs into it
cli_for --pack-top-n=400 > "$TMP/guard.xml"
GOC="$( attr ctx over_ceiling "$TMP/guard.xml" )"; GNX="$( attr sigs next "$TMP/guard.xml" )"; GCAP="$( attr sigs capped "$TMP/guard.xml" )"
if [ "$GOC" = "1" ] && [ "$GCAP" = "1" ] && [ -n "$GNX" ]; then
    ok "(D4) --pack-top-n=400: the guard cut is disclosed (over_ceiling=1, <sigs capped=1 next=…>)"
else
    no "(D4) --pack-top-n=400: expected over_ceiling=1 + <sigs capped=1 next=…>; got over_ceiling='$GOC' capped='$GCAP' next='${GNX:0:40}'"
fi
if grep -qi 'runaway' <( sed 's/<sigs.*//' "$TMP/guard.xml" ); then ok "(D4) the legend names the runaway guard"
else no "(D4) no legend clause names the runaway guard on the answer that hit it"; fi
SIGB="$( python3 -c 'import re,sys; m=re.search(r"<sigs\b.*?</sigs>", sys.stdin.read(), re.S); print(len(m.group(0).encode()) if m else "")' < "$TMP/guard.xml" )"
if ! isnum "$GUARD"; then
    no "(D4) kForRunawayGuardBytes not found in src/serialize.h — the guard has no single owner"
elif ! isnum "$SIGB"; then
    no "(D4) no <sigs> block on the guard answer"
elif [ "$SIGB" -le "$GUARD" ]; then
    ok "(D4) <sigs> block $SIGB B <= guard $GUARD B"
else
    no "(D4) <sigs> block $SIGB B exceeds the runaway guard $GUARD B"
fi

# (D5) the MCP twin serves its whole head
mcp_for "" > "$TMP/mcp_def.xml"
MS="$( attr sigs shown "$TMP/mcp_def.xml" )"; MT="$( attr sigs total "$TMP/mcp_def.xml" )"
rows "$TMP/mcp_def.xml" "$TMP/mcp_def.rows"; MN="$( grep -c . "$TMP/mcp_def.rows" 2>/dev/null || echo 0 )"
if isnum "$MN" && [ "$MN" -gt 0 ] && { [ -z "$MT" ] || [ "$MN" = "$MT" ]; } && { [ -z "$MS" ] || [ "$MS" = "$MN" ]; }; then
    ok "(D5) MCP for serves its whole head: $MN rows"
else
    no "(D5) MCP for served $MN rows (shown='$MS') of total='$MT'"
fi

# (D6) negative: an answer that is complete under the target carries no past-target or guard disclosure
#      (a name-exact query: one anchor row set, well under 7.5 KB; on such answers the change is byte-identical)
"$BIN" "$CORPUS" --for="trimSigLadder" --signatures-only "$CF" > "$TMP/narrow.xml" 2>/dev/null; nrc=$?
NPT="$( attr sigs past_target "$TMP/narrow.xml" )"; NOC="$( attr ctx over_ceiling "$TMP/narrow.xml" )"
NB="$( wc -c < "$TMP/narrow.xml" | tr -d ' ' )"
if [ "$nrc" -ne 0 ] || ! isnum "$NB" || [ "$NB" -eq 0 ]; then no "(D6) the narrow name-exact answer did not run"
elif [ "$NB" -ge 7500 ]; then no "(D6) premise: the narrow answer ($NB B) is not under the 7.5 KB target — pick a narrower query"
elif [ -z "$NPT" ] && [ -z "$NOC" ] && ! grep -qi 'runaway\|past_target' "$TMP/narrow.xml"; then ok "(D6) a $NB B complete answer carries no past_target=/over_ceiling=/guard clause"
else no "(D6) a $NB B complete answer carries past_target='$NPT' over_ceiling='$NOC' or a guard/past-target clause"; fi

# ── explicit targets ─────────────────────────────────────────────────────────────────────────────────────
SWEEP="300 1000 2000 3000 6000"
prev=""
for n in $SWEEP; do
    for lg in full compact; do
        cli_for --token-budget="$n" --legend="$lg" > "$TMP/t_${n}_$lg.xml"
        rows "$TMP/t_${n}_$lg.xml" "$TMP/t_${n}_$lg.rows" || no "(E) N=$n $lg: no <sigs> block"
    done
    f="$TMP/t_${n}_full.xml"
    TOC="$( attr ctx over_ceiling "$f" )"; TN="$( grep -c . "$TMP/t_${n}_full.rows" )"
    # (E1) at the tight end of the sweep
    if [ "$n" -le 1000 ]; then
        TNX="$( attr sigs next "$f" )"
        if [ -z "$TOC" ] && [ "$TN" -ge 4 ] && [ -n "$TNX" ]; then
            ok "(E1) N=$n: no over_ceiling=, $TN floor rows, the cut carries next="
        else
            no "(E1) N=$n: over_ceiling='$TOC', rows=$TN (floor 4), sigs next='${TNX:0:40}' — a soft target is never 'over' and always names its continuation"
        fi
    fi
    # (E2) the header is never shaped
    for lg in full compact; do
        if grep -q 'dropped (ceiling)' "$TMP/t_${n}_$lg.xml"; then
            no "(E2) N=$n $lg: legend clauses dropped '(ceiling)' — the header must not be shaped by a soft target"
        else
            ok "(E2) N=$n $lg: no legend clause dropped"
        fi
    done
    # (E3) rank prefix: of the default rows, and of the next wider target
    if python3 "$TMP/prefix.py" "$TMP/t_${n}_full.rows" "$TMP/def.rows"; then ok "(E3) N=$n: rows ($TN) are a rank prefix of the default rows"
    else no "(E3) N=$n: rows ($TN) are not a rank prefix of the default rows ($DN)"; fi
    if [ -n "$prev" ]; then
        if python3 "$TMP/prefix.py" "$TMP/t_${prev}_full.rows" "$TMP/t_${n}_full.rows"; then ok "(E3) rows(N=$prev) is a prefix of rows(N=$n)"
        else no "(E3) rows(N=$prev) is not a prefix of rows(N=$n) — not monotone"; fi
    fi
    # (E4) dialects agree on rows
    if cmp -s "$TMP/t_${n}_full.rows" "$TMP/t_${n}_compact.rows"; then ok "(E4) N=$n: compact rows == full rows ($TN)"
    else no "(E4) N=$n: compact rows ($( grep -c . "$TMP/t_${n}_compact.rows" )) != full rows ($TN)"; fi
    prev="$n"
done

# (E5) stop when complete: a wide target serves exactly the default answer's rows
cli_for --token-budget=20000 > "$TMP/wide.xml"
rows "$TMP/wide.xml" "$TMP/wide.rows"
if cmp -s "$TMP/wide.rows" "$TMP/def.rows" && [ "$DN" = "$DT" ]; then
    ok "(E5) --token-budget=20000 serves exactly the default (complete) rows: $DN"
else
    no "(E5) --token-budget=20000 served $( grep -c . "$TMP/wide.rows" ) rows; the default served $DN of total=$DT — a target wider than the answer must serve the complete head"
fi

# (E6) MCP for at a tight budget_tokens
mcp_for ',"budget_tokens":300' > "$TMP/mcp_300.xml"
M3OC="$( attr ctx over_ceiling "$TMP/mcp_300.xml" )"; M3NB="$( attr sigs next_budget_tokens "$TMP/mcp_300.xml" )"
if [ -z "$M3OC" ] && isnum "$M3NB"; then ok "(E6) MCP for budget_tokens=300: no over_ceiling=, next_budget_tokens=$M3NB"
else no "(E6) MCP for budget_tokens=300: over_ceiling='$M3OC' next_budget_tokens='$M3NB'"; fi

# (P1) --pack-task at a tight target
"$BIN" "$CORPUS" --pack-task="$Q" --token-budget=300 "$CP" > "$TMP/pt.xml" 2>/dev/null; prc=$?
PTOC="$( attr ctx over_ceiling "$TMP/pt.xml" 2>/dev/null )"; pa=$?
if [ "$prc" -ne 0 ] || [ "$pa" -ne 0 ]; then no "(P1) --pack-task --token-budget=300 did not run (rc=$prc) or has no <ctx> root"
elif [ -z "$PTOC" ]; then ok "(P1) --pack-task --token-budget=300: no over_ceiling= (soft target)"
else no "(P1) --pack-task --token-budget=300: over_ceiling='$PTOC' — a soft target is never 'over'"; fi

# ── (G1) twin: the default map's CI-gate personality is unchanged ────────────────────────────────────────
"$BIN" "$CORPUS" --token-budget=100 "$CM" > "$TMP/map.out" 2>/dev/null; grc=$?
GW="$( attr r withheld "$TMP/map.out" 2>/dev/null )"
if [ "$grc" -eq 3 ] && [ "$GW" = "1" ] && ! grep -q '<d ' "$TMP/map.out"; then ok "(G1) map --token-budget=100: exit 3, the map withheld whole (withheld=1, no rows)"
else no "(G1) map --token-budget=100: rc=$grc withheld='$GW', stdout $( wc -c < "$TMP/map.out" ) B — the gate personality must not move"; fi

if [ "$fail" -ne 0 ]; then echo "FAILURES ABOVE"; exit 1; fi
echo "forsoftbudgetcheck: all arms pass"
