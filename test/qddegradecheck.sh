#!/usr/bin/env bash
# qddegradecheck.sh — #387 follow-up: --quality-delta=A..B ANSWERS under memory pressure instead of refusing.
#
# THE DEFECT. A tripped memory guard in the range form gives exit 5 and nothing on stdout. On a large repository a
# 4-file change cannot get even the findings that need only those 4 files.
#
# THE CONTRACT these arms pin. Why a kind is LOCAL: every per-symbol baseline key is path-qualified (quality.h qualityKey)
# and so is the canonical id, so an UNCHANGED file yields the same keys and metrics in both trees and no row of such a kind:
#   * The 12 kinds split by input scope. LOCAL kinds (api-surface, complexity, error-masking, nesting, params,
#     placeholder, verbosity) need only the changed files' trees. defect-shape is LOCAL except its format-arity scan of a
#     C++ file, which reads the tree's C-family macro NAMES (the neighbourhood input). WHOLE-TREE kinds (dead-code,
#     duplication, new-clone-of-reused-helper) need every file. short-horizon-churn is unavailable in the range form.
#   * When the base commit fit but the target phase is projected past the limit (P1), or a commit's full ingest was
#     stopped by the guard (P0), the range form ingests only the changed files of both commits and runs the kinds that
#     can run there. Every kind it does not run is NAMED (kinds_skipped=, skipped_why="memory"), with the limit
#     (memory_limit=), the measured or lower-bound need (memory_need=), and a replay (kinds_next=) that runs exactly the
#     skipped kinds with enough memory.
#   * Exit: 2 when a row of a kind that ran gates; otherwise 5 (never 0 — a partial answer is not a pass).
#   * Exit 5 with nothing on stdout only when even the changed files do not fit.
#   * When memory suffices the degraded path is unreachable: output byte-identical to the build before it (L).
#   * --quality-kinds=K[,K] runs exactly those kinds (the replay's selector), header kinds_skipped= skipped_why="selector".
#
# Arms (RED on the build before this lane, except (A) the premise and (L) the guard):
#   (A) premise: the fixture's unpressured answer has a row of each of the 11 kinds the range form measures.
#   (B) P1 forced (RIPWIRE_TEST_MEMGUARD=project:1): the degraded answer's header, exit code, stderr line, ingests.
#   (C) P1: each LOCAL kind's rows equal the same kind's rows of (A); no row of a skipped kind.
#   (D) the kinds_next= replay yields exactly the skipped kinds' rows; union with (B) = (A); gating= adds up.
#   (E) P0 (crawl seam cuts the base's FULL ingest): answers from the changed files; defect-shape is skipped because a
#       changed file is C++; (E2) a Python-only change keeps defect-shape and its rows equal (A)'s.
#   (F) even the changed files do not fit (hard line after the base ingest, and every later reading): exit 5, nothing
#       on stdout, ONE line that names the phase AND that the changed files alone did not fit.
#   (G) near misses under project:1: an empty diff, a rename-only diff, a non-code-only diff, and A..A.
#   (H) --quality-kinds: unknown kind refused naming the 12; refused beside --quality-ack; each single kind = (A)'s rows
#       of that kind; all 12 named = byte-identical to no flag.
#   (I) --quality-ack under pressure refuses (exit 5) and leaves the ledger untouched.
#   (J) --scope naming an unchanged file under pressure is not refused as "names nothing indexed".
#   (K) legend + JSON: --legend=full defines every new attribute and parses as XML; --json carries the twins; an
#       unpressured --json carries none of them.
#   (L) guard: a projection seam index past the run's one projection reading changes nothing (byte-identical).
#
# Usage: bash test/qddegradecheck.sh [BIN]
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*"; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v git >/dev/null 2>&1 || { echo "  SKIP  qddegradecheck (git not available)"; exit 0; }
command -v python3 >/dev/null 2>&1 || { echo "  SKIP  qddegradecheck (python3 not available)"; exit 0; }
WORK="$( mktemp -d )"; trap 'rm -rf "$WORK"' EXIT
mkdir -p "$WORK/tmp"
echo "qddegradecheck: BIN=$BIN"

LOCAL_KINDS="api-surface complexity defect-shape error-masking nesting params placeholder verbosity"
WHOLE_KINDS="dead-code,duplication,new-clone-of-reused-helper"
ALL12="api-surface,complexity,dead-code,defect-shape,duplication,error-masking,nesting,new-clone-of-reused-helper,params,placeholder,short-horizon-churn,verbosity"

g(){ git -C "$1" -c user.name=t -c user.email=t@example.com "${@:2}"; }

# ── the fixture: a base commit and a target commit with one change per kind, beside UNCHANGED files ─────────────
# The unchanged files make the full tree larger than the changed set, so a crawl seam can cut the full ingest while the
# changed-files ingest stays under it (E).
mkfx(){
    local R="$1"; rm -rf "$R"; mkdir -p "$R/src" "$R/py"
    git -C "$R" init -q
    for k in 1 2 3 4 5 6; do
        printf 'int keep_%s( int v ) { return v + %s; }\n' "$k" "$k" >"$R/src/u$k.cpp"
        printf 'def keep_%s(v):\n    return v * %s\n' "$k" "$k" >"$R/py/u$k.py"
    done
    printf '#pragma once\ninline int api_scale( int a ) { return a * 2; }\n' >"$R/src/api.h"
    {
        printf '#include <cstdio>\n#include "api.h"\n'
        printf 'int helper( int a, int b ) { int s = 0; for( int i = 0; i < a; ++i ) { s += i * b; if( s > 100 ) { s -= b; } } return s + a - b; }\n'
        printf 'int use1( int x ) { return helper( x, 1 ); }\nint use2( int x ) { return helper( x, 2 ); }\nint use3( int x ) { return helper( x, 3 ); }\n'
        printf 'int legacy_used( int x ) { return x + 7; }\n'
        printf 'int wide( int a, int b, int c, int d, int e ) { return a + b + c + d + e; }\n'
        printf 'void risky() {}\nvoid handle() { try { risky(); } catch( ... ) { std::puts( "logged" ); } }\n'
        printf 'int grow( int x )\n{\n    if( x > 1 ) { return 1; }\n    return 0;\n}\n'
        printf 'int chatty( int x )\n{\n    int r = x;\n    r += 1;\n    r += 2;\n    return r;\n}\n'
        printf 'int drive( int x ) { return use1( x ) + use2( x ) + use3( x ) + legacy_used( x ) + wide( 1, 2, 3, 4, 5 ) + grow( x ) + api_scale( x ) + chatty( x ); }\n'
    } >"$R/src/core.cpp"
    printf 'def other():\n    return 1\n\ndef greet(name):\n    return "hello {}".format(name)\n' >"$R/py/mod.py"
    printf 'notes\n' >"$R/NOTES.txt"
    g "$R" add -A; g "$R" commit -qm base
}
# target: complexity+nesting (grow), verbosity (chatty), params (wide 5->6), api-surface (api_scale 1->2, a header),
# error-masking (an emptied catch), dead-code (legacy_used loses its caller), duplication + new-clone-of-reused-helper
# (helper, fan-in 3, copied), placeholder (a TODO, a NotImplementedError stub), defect-shape (a .format arity miss)
mktarget(){
    local R="$1"
    printf '#pragma once\ninline int api_scale( int a, int b ) { return a * b; }\n' >"$R/src/api.h"
    {
        printf '#include <cstdio>\n#include "api.h"\n'
        printf 'int helper( int a, int b ) { int s = 0; for( int i = 0; i < a; ++i ) { s += i * b; if( s > 100 ) { s -= b; } } return s + a - b; }\n'
        printf 'int use1( int x ) { return helper( x, 1 ); }\nint use2( int x ) { return helper( x, 2 ); }\nint use3( int x ) { return helper( x, 3 ); }\n'
        printf 'int legacy_used( int x ) { return x + 7; }\n'
        printf 'int wide( int a, int b, int c, int d, int e, int f ) { return a + b + c + d + e + f; }\n'
        printf 'void risky() {}\nvoid handle() { try { risky(); } catch( ... ) {} }\n'
        printf 'int grow( int x )\n{\n    int r = 0;\n    for( int i = 0; i < x; ++i ) {\n        if( i %% 2 ) {\n            while( r < i ) {\n'
        printf '                if( r %% 3 ) {\n                    for( int j = 0; j < 3; ++j ) {\n'
        printf '                        if( j && r ) { r += j; } else if( r > 9 ) { r -= 1; } else { r += 2; }\n                    }\n'
        printf '                } else if( r %% 5 ) { r += 3; } else { r += 4; }\n            }\n'
        printf '        } else if( i %% 7 ) { r -= 1; } else if( i %% 11 ) { r += 5; } else { r -= 2; }\n'
        printf '        if( r > 1000 && i > 3 ) { break; }\n        if( r < -1000 || i > 900 ) { continue; }\n    }\n    return r;\n}\n'
        printf 'int chatty( int x )\n{\n    int r = x;\n'
        for i in $( seq 1 69 ); do printf '    r += %s;\n' "$i"; done
        printf '    return r;\n}\n'
        printf 'int drive( int x ) { return use1( x ) + use2( x ) + use3( x ) + wide( 1, 2, 3, 4, 5, 6 ) + grow( x ) + api_scale( x, 2 ) + chatty( x ); }\n'
    } >"$R/src/core.cpp"
    printf 'int helper_copy( int a, int b ) { int s = 0; for( int i = 0; i < a; ++i ) { s += i * b; if( s > 100 ) { s -= b; } } return s + a - b; }\n' >"$R/src/copy.cpp"
    printf 'def other():\n    # TODO: wire this up\n    return 1\n\ndef greet(name):\n    return "hello {} {}".format(name)\n\ndef later():\n    raise NotImplementedError\n' >"$R/py/mod.py"
    g "$R" add -A; g "$R" commit -qm target
}

FX="$WORK/fx"; mkfx "$FX"; mktarget "$FX"
BASE="$( git -C "$FX" rev-parse HEAD~1 )"; TGT="$( git -C "$FX" rev-parse HEAD )"
# run [ENV=VAL ...] -- ARGS: stdout/stderr to $WORK/o, $WORK/e; rc in $RC
qd(){ ( cd "$FX" && env TMPDIR="$WORK/tmp" RIPWIRE_CACHE_STATS=1 "$@" >"$WORK/o" 2>"$WORK/e" ); RC=$?; }
# rows: every <r .../> and <sa .../> element on its own line, sorted (the union rule: rows compare as SORTED sets)
rows(){ python3 - "$1" "${2:-}" <<'PY'
import re, sys
doc = open(sys.argv[1], encoding="utf-8", errors="replace").read()
want = set(k for k in sys.argv[2].split(",") if k)
out = []
for m in re.finditer(r'<(r|sa) [^>]*/>', doc):
    k = re.search(r' kind="([^"]*)"', m.group(0))
    if k and (not want or k.group(1) in want):
        out.append(m.group(0))
print("\n".join(sorted(out)))
PY
}
attr(){ sed -n 's/.*<quality-delta [^>]*'"$2"'="\([^"]*\)".*/\1/p' "$1" | head -n 1; }
hasattr(){ grep -q "<quality-delta [^>]* $2=\"" "$1"; }
ingests(){ grep -c '^ripwire: cache-stats ' "$WORK/e"; }
memlines(){ grep -c '^ripwire: memory limit ' "$WORK/e"; }
isnum(){ case "$1" in ''|*[!0-9]*) return 1;; *) return 0;; esac; }

# ── (A) the premise ───────────────────────────────────────────────────────────────────────────────────────────
qd "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache; cp "$WORK/o" "$WORK/A.xml"; cp "$WORK/e" "$WORK/A.err"; ARC=$RC
missing=""
for k in api-surface complexity dead-code defect-shape duplication error-masking nesting new-clone-of-reused-helper params placeholder verbosity; do
    [ -n "$( rows "$WORK/A.xml" "$k" )" ] || missing="$missing $k"
done
if [ "$ARC" = 2 ] && [ -z "$missing" ]; then ok "(A) premise: the unpressured answer has a row of each of the 11 measured kinds (rc=2)"
else no "(A) premise: rc=$ARC, kinds with no row:$missing — every arm below compares against this answer, so it proves nothing"; fi
AGATE="$( attr "$WORK/A.xml" gating )"

# ── (B) P1 forced: the base fit, the target phase is projected past the limit ───────────────────────────────────
qd RIPWIRE_TEST_MEMGUARD=project:1 "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache; cp "$WORK/o" "$WORK/B.xml"; cp "$WORK/e" "$WORK/B.err"; BRC=$RC; BING="$( ingests )"
bgate="$( attr "$WORK/B.xml" gating )"; need="$( attr "$WORK/B.xml" memory_need )"; needn="${need%M}"
wantrc=5; isnum "$bgate" && [ "$bgate" -gt 0 ] && wantrc=2
bnext="$( attr "$WORK/B.xml" kinds_next )"
if [ -s "$WORK/B.xml" ] && python3 -c 'import sys,xml.dom.minidom as m; m.parse(sys.argv[1])' "$WORK/B.xml" 2>/dev/null \
   && [ "$( attr "$WORK/B.xml" kinds_skipped )" = "$WHOLE_KINDS" ] && [ "$( attr "$WORK/B.xml" skipped_why )" = memory ] \
   && [ "$( attr "$WORK/B.xml" ingest_scope )" = changed-files ] && hasattr "$WORK/B.xml" memory_limit \
   && isnum "$needn" && [ "$( attr "$WORK/B.xml" also_skipped )" = "config-warnings,stale-acks" ] && ! hasattr "$WORK/B.xml" stale \
   && ! hasattr "$WORK/B.xml" register-macro-excluded && isnum "$bgate" && [ "$BRC" = "$wantrc" ]; then
    ok "(B) P1: a degraded answer (rc=$BRC, gating=$bgate) names kinds_skipped=$WHOLE_KINDS, the limit, need=${need}, also_skipped, no stale=/dead-code counts"
else
    no "(B) P1: rc=$BRC (want $wantrc) gating=$bgate kinds_skipped=$( attr "$WORK/B.xml" kinds_skipped ) why=$( attr "$WORK/B.xml" skipped_why ) scope=$( attr "$WORK/B.xml" ingest_scope ) need=$need stdout=$( wc -c <"$WORK/B.xml" | tr -d ' ' )B"
fi
if [ "$bnext" = "--quality-delta=$BASE..$TGT --quality-kinds=$WHOLE_KINDS --max-memory=$need" ]; then ok "(B) kinds_next= replays exactly the skipped kinds at the measured need"
else no "(B) kinds_next='$bnext'"; fi
if [ "$( memlines )" = 1 ] && grep -q "^ripwire: memory limit .*dead-code, duplication, new-clone-of-reused-helper" "$WORK/B.err" && [ "$BING" = 2 ]; then
    ok "(B) one stderr line names the skipped kinds; two ingests (the base in full, then the target's changed files)"
else no "(B) memory lines=$( memlines ) ingests=$BING stderr: $( grep '^ripwire: [^c]' "$WORK/B.err" | head -c 300 )"; fi

# ── (C) P1: LOCAL kinds equal the unpressured answer, kind by kind ─────────────────────────────────────────────
if [ -n "$( attr "$WORK/B.xml" kinds_skipped )" ]; then
    for k in $LOCAL_KINDS; do
        if [ "$( rows "$WORK/B.xml" "$k" )" = "$( rows "$WORK/A.xml" "$k" )" ] && [ -n "$( rows "$WORK/A.xml" "$k" )" ]; then ok "(C) P1 $k: rows identical to the unpressured answer"
        else no "(C) P1 $k: rows differ: $( diff <( rows "$WORK/A.xml" "$k" ) <( rows "$WORK/B.xml" "$k" ) | head -c 300 )"; fi
    done
    [ -z "$( rows "$WORK/B.xml" "$WHOLE_KINDS" )" ] && ok "(C) P1: no row of a skipped kind" || no "(C) P1: a skipped kind has rows"
    [ "$( attr "$WORK/B.xml" api-new-surface )" = "$( attr "$WORK/A.xml" api-new-surface )" ] && ok "(C) P1: api-new-surface= equal" || no "(C) api-new-surface differs"
else
    no "(C) P1: premise — (B) is not a degraded answer (no kinds_skipped=), so per-kind identity proves nothing"
fi

# ── (D) the replay ─────────────────────────────────────────────────────────────────────────────────────────────
if [ -n "$bnext" ]; then
    # shellcheck disable=SC2086
    qd "$BIN" . $bnext --no-cache; cp "$WORK/o" "$WORK/D.xml"; DRC=$RC
    dgate="$( attr "$WORK/D.xml" gating )"
    if [ "$( rows "$WORK/D.xml" )" = "$( rows "$WORK/A.xml" "$WHOLE_KINDS" )" ] && [ -n "$( rows "$WORK/D.xml" )" ]; then ok "(D) the replay's rows are exactly (A)'s skipped-kind rows"
    else no "(D) replay rc=$DRC rows differ: $( diff <( rows "$WORK/A.xml" "$WHOLE_KINDS" ) <( rows "$WORK/D.xml" ) | head -c 300 )"; fi
    if [ "$( { rows "$WORK/B.xml"; rows "$WORK/D.xml"; } | sort )" = "$( rows "$WORK/A.xml" )" ]; then ok "(D) union of the degraded answer and the replay = the unpressured answer (as sorted rows)"
    else no "(D) union differs from (A)"; fi
    if isnum "$bgate" && isnum "$dgate" && isnum "$AGATE" && [ $(( bgate + dgate )) = "$AGATE" ]; then ok "(D) gating= adds up: $bgate + $dgate = $AGATE"
    else no "(D) gating: B=$bgate D=$dgate A=$AGATE"; fi
    [ "$( attr "$WORK/D.xml" skipped_why )" = selector ] && ok "(D) the replay says its kinds were selected (skipped_why=selector)" || no "(D) replay skipped_why=$( attr "$WORK/D.xml" skipped_why )"
else
    no "(D) premise: no kinds_next= to replay"
fi

# ── (E) P0: the guard cut the base commit's FULL ingest (crawl seam: the full tree has more entries than the changed set) ─
qd RIPWIRE_TEST_MEMGUARD=crawl:12 "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache; cp "$WORK/o" "$WORK/E.xml"; cp "$WORK/e" "$WORK/E.err"; ERC=$RC
if [ -s "$WORK/E.xml" ] && [ "$( attr "$WORK/E.xml" kinds_skipped )" = "dead-code,defect-shape,duplication,new-clone-of-reused-helper" ] \
   && [ "$( attr "$WORK/E.xml" ingest_scope )" = changed-files ] && [ "$( ingests )" = 3 ] && { [ "$ERC" = 2 ] || [ "$ERC" = 5 ]; }; then
    ok "(E) P0: the cut full ingest is never scored; the changed files answer (rc=$ERC); defect-shape skipped (a changed C++ file)"
else
    no "(E) P0: rc=$ERC kinds_skipped=$( attr "$WORK/E.xml" kinds_skipped ) ingests=$( ingests ) stderr: $( grep '^ripwire: [^c]' "$WORK/E.err" | head -c 300 )"
fi
if [ -n "$( attr "$WORK/E.xml" kinds_skipped )" ]; then
    for k in api-surface complexity error-masking nesting params placeholder verbosity; do
        [ "$( rows "$WORK/E.xml" "$k" )" = "$( rows "$WORK/A.xml" "$k" )" ] && ok "(E) P0 $k: rows identical" || no "(E) P0 $k: rows differ"
    done
    case "$( attr "$WORK/E.xml" memory_need )" in '>'*M) ok "(E) P0: memory_need= is a lower bound ('>' the limit) — the full need was never measured";; *) no "(E) P0 memory_need=$( attr "$WORK/E.xml" memory_need )";; esac
else
    no "(E) P0: premise — not a degraded answer"
fi
# (E2) a Python-only change: no changed C++ file, so defect-shape runs on the changed files and equals the unpressured rows
FX2="$WORK/fx2"; mkfx "$FX2"
printf 'def other():\n    return 1\n\ndef greet(name):\n    return "hello {} {}".format(name)\n' >"$FX2/py/mod.py"; g "$FX2" add -A; g "$FX2" commit -qm py
( cd "$FX2" && TMPDIR="$WORK/tmp" "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache >"$WORK/E2a.xml" 2>/dev/null )
( cd "$FX2" && TMPDIR="$WORK/tmp" RIPWIRE_TEST_MEMGUARD=crawl:12 "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache >"$WORK/E2.xml" 2>/dev/null )
if [ "$( attr "$WORK/E2.xml" kinds_skipped )" = "$WHOLE_KINDS" ] && [ -n "$( rows "$WORK/E2a.xml" defect-shape )" ] \
   && [ "$( rows "$WORK/E2.xml" defect-shape )" = "$( rows "$WORK/E2a.xml" defect-shape )" ]; then
    ok "(E2) P0, Python-only change: defect-shape runs on the changed files and its rows equal the unpressured ones"
else
    no "(E2) kinds_skipped=$( attr "$WORK/E2.xml" kinds_skipped ) unpressured defect rows=$( rows "$WORK/E2a.xml" defect-shape | grep -c . )"
fi

# ── (F) even the changed files do not fit ──────────────────────────────────────────────────────────────────────
qd RIPWIRE_TEST_MEMGUARD=hard:1 "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache; FRC=$RC
if [ "$FRC" = 5 ] && [ ! -s "$WORK/o" ] && [ "$( grep -c '^ripwire: [^c]' "$WORK/e" )" = 1 ] \
   && grep -q '^ripwire: memory limit reached during the ingest of the base commit (limit .*changed files' "$WORK/e"; then
    ok "(F) the changed files do not fit either: exit 5, nothing on stdout, one line naming the phase and the changed-files attempt"
else
    no "(F) rc=$FRC stdout=$( wc -c <"$WORK/o" | tr -d ' ' )B stderr: $( grep '^ripwire: [^c]' "$WORK/e" | head -c 300 )"
fi

# ── (G) near misses under project:1 ────────────────────────────────────────────────────────────────────────────
nm(){ # $1 label, $2 repo; compares LOCAL rows with the unpressured run and requires the degraded header
    local L="$1" R="$2"
    ( cd "$R" && TMPDIR="$WORK/tmp" "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache >"$WORK/nmA.xml" 2>/dev/null )
    ( cd "$R" && TMPDIR="$WORK/tmp" RIPWIRE_TEST_MEMGUARD=project:1 "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache >"$WORK/nmB.xml" 2>"$WORK/nmB.err" ); local rc=$?
    local loc; loc="$( echo "$LOCAL_KINDS" | tr ' ' ',' )"
    if [ "$( attr "$WORK/nmB.xml" kinds_skipped )" = "$WHOLE_KINDS" ] && [ "$( rows "$WORK/nmB.xml" "$loc" )" = "$( rows "$WORK/nmA.xml" "$loc" )" ] \
       && ! grep -q 'could not materialize' "$WORK/nmB.err" && { [ "$rc" = 5 ] || [ "$rc" = 2 ]; }; then
        ok "(G) $L: degraded answer (rc=$rc), LOCAL rows equal the unpressured run"
    else
        no "(G) $L: rc=$rc kinds_skipped=$( attr "$WORK/nmB.xml" kinds_skipped ) stderr: $( head -c 300 "$WORK/nmB.err" )"
    fi
}
FE="$WORK/fe"; mkfx "$FE"; g "$FE" commit -q --allow-empty -m empty; nm "empty diff" "$FE"
FR="$WORK/fr"; mkfx "$FR"; g "$FR" mv py/u1.py py/u1_moved.py; g "$FR" commit -qm rename; nm "rename-only diff" "$FR"
FN="$WORK/fn"; mkfx "$FN"; printf 'more notes\n' >>"$FN/NOTES.txt"; g "$FN" add -A; g "$FN" commit -qm notes; nm "non-code-only diff" "$FN"
( cd "$FX" && TMPDIR="$WORK/tmp" "$BIN" . --quality-delta=HEAD..HEAD --no-cache >"$WORK/AA.xml" 2>"$WORK/AA.err" ); r1=$?
( cd "$FX" && TMPDIR="$WORK/tmp" RIPWIRE_TEST_MEMGUARD=project:1 "$BIN" . --quality-delta=HEAD..HEAD --no-cache >"$WORK/AAp.xml" 2>"$WORK/AAp.err" ); r2=$?
if [ "$r1" = "$r2" ] && cmp -s "$WORK/AA.xml" "$WORK/AAp.xml" && [ -s "$WORK/AA.xml" ]; then ok "(G) A..A: one tree, nothing to project — answered in full, byte-identical (rc=$r1)"
else no "(G) A..A: rc $r1 vs $r2 or the answers differ"; fi

# ── (H) --quality-kinds ───────────────────────────────────────────────────────────────────────────────────────
qd "$BIN" . --quality-delta=HEAD~1..HEAD --quality-kinds=complexity,dupe --no-cache
if [ "$RC" = 1 ] && [ ! -s "$WORK/o" ] && grep -q "dupe" "$WORK/e" && grep -q "new-clone-of-reused-helper" "$WORK/e" && grep -q "short-horizon-churn" "$WORK/e"; then
    ok "(H) an unknown kind is refused (exit 1) naming it and the 12 kinds"
else no "(H) unknown kind: rc=$RC stderr: $( head -c 300 "$WORK/e" )"; fi
qd "$BIN" . --quality-delta=HEAD~1..HEAD --quality-kinds=complexity --quality-ack --no-cache
if [ "$RC" = 1 ] && [ ! -e "$FX/.ripwire_quality_acks" ] && grep -q -- '--quality-kinds.*--quality-ack\|--quality-ack.*--quality-kinds' "$WORK/e" && ! grep -q 'unknown flag' "$WORK/e"; then
    ok "(H) --quality-kinds beside --quality-ack is refused (exit 1) naming both flags, no ledger written"
else no "(H) with --quality-ack rc=$RC stderr: $( head -c 300 "$WORK/e" )"; fi
for k in api-surface complexity dead-code defect-shape duplication error-masking nesting new-clone-of-reused-helper params placeholder verbosity; do
    qd "$BIN" . --quality-delta=HEAD~1..HEAD "--quality-kinds=$k" --no-cache
    if [ "$( rows "$WORK/o" )" = "$( rows "$WORK/A.xml" "$k" )" ] && [ -n "$( rows "$WORK/o" )" ] && [ "$( attr "$WORK/o" skipped_why )" = selector ]; then ok "(H) --quality-kinds=$k: exactly (A)'s $k rows"
    else no "(H) --quality-kinds=$k: rc=$RC rows=$( rows "$WORK/o" | grep -c . ) want $( rows "$WORK/A.xml" "$k" | grep -c . ) why=$( attr "$WORK/o" skipped_why )"; fi
done
qd "$BIN" . --quality-delta=HEAD~1..HEAD "--quality-kinds=$ALL12" --no-cache
[ "$RC" = "$ARC" ] && cmp -s "$WORK/o" "$WORK/A.xml" && ok "(H) all 12 named: byte-identical to no selector" || no "(H) all 12 named: rc=$RC or output differs from (A)"

# ── (I) --quality-ack under pressure ──────────────────────────────────────────────────────────────────────────
qd RIPWIRE_TEST_MEMGUARD=project:1 "$BIN" . --quality-delta=HEAD~1..HEAD --quality-ack --no-cache
if [ "$RC" = 5 ] && [ ! -e "$FX/.ripwire_quality_acks" ] && [ "$( memlines )" = 1 ]; then ok "(I) --quality-ack under pressure refuses (exit 5), the ledger untouched"
else no "(I) rc=$RC ledger=$( [ -e "$FX/.ripwire_quality_acks" ] && echo written || echo absent ) stderr: $( grep '^ripwire: [^c]' "$WORK/e" | head -c 300 )"; fi
rm -f "$FX/.ripwire_quality_acks"

# ── (J) --scope naming an unchanged file under pressure ───────────────────────────────────────────────────────
qd "$BIN" . --quality-delta=HEAD~1..HEAD --scope=py/u2.py --no-cache; jA=$RC; cp "$WORK/o" "$WORK/JA.xml"
qd RIPWIRE_TEST_MEMGUARD=project:1 "$BIN" . --quality-delta=HEAD~1..HEAD --scope=py/u2.py --no-cache
if [ "$RC" != 1 ] && ! grep -q 'names nothing' "$WORK/e" && [ -n "$( attr "$WORK/o" kinds_skipped )" ] \
   && [ "$( rows "$WORK/o" )" = "$( rows "$WORK/JA.xml" "$( echo "$LOCAL_KINDS" | tr ' ' ',' )" )" ]; then
    ok "(J) --scope on an unchanged file under pressure: answered (rc=$RC, unpressured rc=$jA), not refused as naming nothing"
else no "(J) rc=$RC (unpressured $jA) stderr: $( grep '^ripwire: [^c]' "$WORK/e" | head -c 300 )"; fi

# ── (K) legend and JSON ───────────────────────────────────────────────────────────────────────────────────────
qd RIPWIRE_TEST_MEMGUARD=project:1 "$BIN" . --quality-delta=HEAD~1..HEAD --legend=full --no-cache
missing=""
for a in kinds_skipped= skipped_why= ingest_scope= memory_need= also_skipped= kinds_next=; do
    sed -n 's/^\(<!--.*-->\).*/\1/p' "$WORK/o" | grep -q -- "$a" || missing="$missing $a"
done
if [ -z "$missing" ] && python3 -c 'import sys,xml.dom.minidom as m; m.parse(sys.argv[1])' "$WORK/o" 2>/dev/null; then ok "(K) --legend=full defines every new attribute and parses as XML"
else no "(K) legend lacks:$missing (or invalid XML)"; fi
qd RIPWIRE_TEST_MEMGUARD=project:1 "$BIN" . --quality-delta=HEAD~1..HEAD --json --no-cache
if python3 - "$WORK/o" <<'PY'
import json, sys
d = json.load(open(sys.argv[1]))
assert d["kinds_skipped"] == ["dead-code", "duplication", "new-clone-of-reused-helper"], d.get("kinds_skipped")
assert d["skipped_why"] == "memory" and d["ingest_scope"] == "changed-files"
assert d["also_skipped"] == ["config-warnings", "stale-acks"] and "stale" not in d
assert d["kinds_next"].startswith("--quality-delta=") and "memory_need" in d and "memory_limit" in d
PY
then ok "(K) --json carries the twins (kinds_skipped list, skipped_why, ingest_scope, also_skipped, kinds_next, memory_*)"
else no "(K) --json twins missing or wrong: $( head -c 300 "$WORK/o" )"; fi
qd "$BIN" . --quality-delta=HEAD~1..HEAD --json --no-cache
grep -q '"kinds_skipped"\|"ingest_scope"\|"kinds_next"\|"also_skipped"' "$WORK/o" && no "(K) an unpressured --json carries a degraded-only key" || ok "(K) an unpressured --json carries none of the degraded keys"

# ── (L) guard: the projection seam past the run's one projection reading changes nothing ───────────────────────
qd RIPWIRE_TEST_MEMGUARD=project:2 "$BIN" . --quality-delta=HEAD~1..HEAD --no-cache
if [ "$RC" = "$ARC" ] && cmp -s "$WORK/o" "$WORK/A.xml" && [ "$( grep -v '^ripwire: cache-stats ' "$WORK/e" )" = "$( grep -v '^ripwire: cache-stats ' "$WORK/A.err" )" ]; then
    ok "(L) project:2 (past the one projection reading) is byte-identical to no seam (rc=$RC)"
else no "(L) project:2 rc=$RC (want $ARC) or the answer differs"; fi

[ "$fail" = 0 ] && echo "qddegradecheck: ALL PASS" || echo "qddegradecheck: FAIL"
exit "$fail"
