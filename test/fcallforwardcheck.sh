#!/usr/bin/env bash
# fcallforwardcheck.sh — JS/TS call forwarders: `f.call( … )` / `f.apply( … )` on a PROVEN function binding is a call of f,
# and `f.bind( … )` is a value reference to f, never a call (lane fcall-forward, train 30).
#
#   test/fcallforwardcheck.sh                          # uses build/ripwire on test/fcallforwardfix
#   RIPWIRE_BIN=asan/ripwire test/fcallforwardcheck.sh
#   test/fcallforwardcheck.sh build_base/ripwire       # the RED run (a pre-change binary)
#
# THE DEFECT. `handleRequest.call( request.server, null, request, reply )` was recorded only as a member call named `call`
# on the receiver handleRequest: no edge to handleRequest, not even a gap. On a real JS framework the request pipeline
# (route handler → pre-parsing → request handler → … ) was unreachable through --path (reachable=0) and the handler was
# missing from --callees of the function that runs it.
#
# THE FIX (model.h kCallForwarders, read by ingest_valuerefs.h). A family-owned data row per language names the builtin
# forwarders: JS/TS call/apply (kind Call) and bind (kind Value). The value-reference walk, which already tracks
# parameters and locals lexically, queues `f.call( … )` / `f.apply( … )` when f is a bare identifier no enclosing
# parameter or local declares; at file end it keeps the site only when the FILE proves f names a function — a function
# declaration of the file (no file-scope variable rebinding the name) or a file-scope import from a RELATIVE module.
# The kept site is a call reference to f — the bare call `f( … )` — so the ordinary resolution tiers decide it, and a name
# with several definitions stays hedged exactly as the bare call is. `f.bind( … )` lands f, as a value, in the slot the
# whole call fills. The original member reference (`call`) is unchanged.
#
# ARMS (fixture test/fcallforwardfix/js and /ts):
#   (P1) `.call` on a relative require: --callees=runPreParsing has handleRequest, proven (no via=)       RED on base
#   (P2) `.apply` on a function declared BELOW the site (hoisting): viaApply → localDecl                  RED on base
#   (P3) `.call` on a destructured relative require: helperUser → helperA                                RED on base
#   (R)  --path=routeHandler,preHandlerInner reachable=1 hops=3 (the fastify-14 chain shape)            RED on base
#   (N1) a parameter shadows: nearParam's `cb.call` never reaches other.js:cb
#   (N2) a method-valued member `obj.method.call` never reaches other.js:method
#   (N3) a file-scope NON-function binding `notFn.call` never reaches other.js:notFn
#   (N4) a non-relative import `vendor.call` never reaches other.js:vendor
#   (N5) a loop variable `hook.call` never reaches other.js:hook
#   (N6) `Reflect.apply( target, … )` and `Object.prototype.toString.call` add no call
#   (H)  several candidate definitions: `dup.call` gets exactly the rows the bare `dup()` gets, all via="name" (hedged)
#   (K)  the member reference itself is unchanged: runPreParsing keeps its `call` row via="name" (the fixture's
#        object-literal method named call), as on the pre-change binary
#   (V1) `.bind` handed on as an argument is not a call site: handleRequest's caller sites are the `.call` line only
#   (V2) `onlyBound.bind( this )` as an argument: no caller row, a value row into="emitter.on#arg1"     RED on base
#   (T1) TS `.call` on an aliased named import, (T2) `.apply` on a default import: tsUser → target, defaultFn  RED on base
#   (T3) a namespace member `ns.target.call`, (T4) a typed parameter `handler.call`, (T5) `.bind` into a local,
#   (T6) a local arrow `run.call` (a disclosed floor): none adds a call
set -u
export PYTHONDONTWRITEBYTECODE=1
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
CORPUS="$ROOT/test/fcallforwardfix"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first (cmake --build build -j)"; exit 2; }
[ -d "$CORPUS/js" ] && [ -d "$CORPUS/ts" ] || { echo "fixture missing: $CORPUS"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
echo "fcallforwardcheck: BIN=$BIN  CORPUS=$CORPUS"

# run( dir, args… ): the answer, or the literal ERROR rc=N so an empty or failed run can never satisfy a check
run(){
    local d="$1"; shift
    local out rc
    out="$( cd "$CORPUS/$d" && "$BIN" . --no-cache "$@" 2>/dev/null )"; rc=$?
    if [ "$rc" -ne 0 ] || [ -z "$out" ]; then printf 'ERROR rc=%s' "$rc"; else printf '%s' "$out"; fi
}
root_tag(){ printf '%s' "$1" | grep -oE "<$2 [^>]*>" | head -1; }
attr(){ printf '%s' "$1" | grep -oE " $2=\"[^\"]*\"" | head -1 | sed -E 's/^[^"]*"([^"]*)"$/\1/'; }
# rows( doc ): the <s> rows as sorted "file:name[~name]" words (p= without its line; ~name marks via="name")
rows(){ printf '%s' "$1" | python3 -c '
import re, sys
t = re.sub( r"<!--.*?-->", "", sys.stdin.read(), flags = re.S )
out = []
for m in re.finditer( r"<s ([^>]*?)/?>", t ):
    a = dict( re.findall( r"(\w+)=\"([^\"]*)\"", m.group( 1 ) ) )
    if "n" in a and "p" in a:
        out.append( a[ "p" ].rsplit( ":", 1 )[ 0 ] + ":" + a[ "n" ] + ( "~name" if a.get( "via" ) == "name" else "" ) )
print( " ".join( sorted( out ) ) )'; }
# has_root( doc, tag ): the answer RAN and produced its root element (an absence is read only off such a run)
has_root(){ [ -n "$( root_tag "$1" "$2" )" ]; }

# ── JS ───────────────────────────────────────────────────────────────────────────────────────────────────────
D="$( run js --callees=runPreParsing )"; R="$( rows "$D" )"
if has_root "$D" callees; then
    case " $R " in *" handle-request.js:handleRequest "*) ok "(P1) runPreParsing → handleRequest, proven (rows: $R)";;
        *) no "(P1) runPreParsing should call handle-request.js:handleRequest without via=: [$R]";; esac
    case " $R " in *" route.js:call~name "*) ok "(K) the member reference is unchanged: the via=name \`call\` row stays";;
        *) no "(K) the original member reference changed: [$R]";; esac
else no "(P1) --callees=runPreParsing did not run: $D"; fi

D="$( run js --callees=viaApply )"; R="$( rows "$D" )"
if has_root "$D" callees && [ "$R" = "route.js:localDecl" ]; then ok "(P2) viaApply → localDecl (a declaration below the site)"; else no "(P2) viaApply should call exactly route.js:localDecl: [$R] ${D:0:120}"; fi

D="$( run js --callees=helperUser )"; R="$( rows "$D" )"
case " $R " in *" helpers.js:helperA "*) ok "(P3) helperUser → helperA (destructured relative require)";;
    *) no "(P3) helperUser should call helpers.js:helperA: [$R] ${D:0:120}";; esac

D="$( run js --path=routeHandler,preHandlerInner )"; P="$( root_tag "$D" path )"
if [ "$( attr "$P" reachable )" = 1 ] && [ "$( attr "$P" hops )" = 3 ]; then ok "(R) --path=routeHandler,preHandlerInner reachable=1 hops=3"; else no "(R) --path should be reachable=1 hops=3: ${P:-$D}"; fi

# the near-misses: no answer may reach a namesake in other.js (each would be a FALSE edge)
for pair in N1:nearParam N2:nearMember N3:nearNonFunc N4:nearExternal N5:nearLoop N6:nearGlobal; do
    arm="${pair%%:*}"; fn="${pair#*:}"
    D="$( run js --callees="$fn" )"; R="$( rows "$D" )"
    if ! has_root "$D" callees; then no "($arm) --callees=$fn did not run: $D"; continue; fi
    case " $R " in *" other.js:"*|*":target"*) no "($arm) $fn reaches a namesake it cannot call: [$R]";;
        *) ok "($arm) $fn adds no forwarded call (rows: ${R:-none})";; esac
done
for nm in cb hook notFn method vendor target; do
    D="$( run js --callers=other.js:"$nm" )"; C="$( root_tag "$D" callers )"
    if [ -n "$C" ] && [ "$( attr "$C" count )" = 0 ]; then ok "(N) other.js:$nm has no caller"; else no "(N) other.js:$nm should have count=0: ${C:-$D}"; fi
done

DF="$( run js --callees=dupForwarded )"; DB="$( run js --callees=dupBare )"
RF="$( rows "$DF" | tr ' ' '\n' | grep -v ':call~name$' | tr '\n' ' ' | sed 's/ $//' )"; RB="$( rows "$DB" )"
if has_root "$DF" callees && has_root "$DB" callees && [ -n "$RB" ] && [ "$RF" = "$RB" ]; then
    case " $RB " in *" route.js:dup "*) no "(H) a dup row is proven (no via=): [$RB]";;
        *) ok "(H) dup.call gets exactly the bare call's rows, all hedged: [$RF]";; esac
else no "(H) dup.call rows [$RF] differ from the bare dup() rows [$RB]"; fi

D="$( run js --callers=handleRequest )"; C="$( root_tag "$D" callers )"
SITES="$( printf '%s' "$D" | grep -oE 'sites_at="[^"]*"' | head -1 )"
if [ -n "$C" ] && [ "$( attr "$C" count )" = 1 ] && [ "$SITES" = 'sites_at="route.js:16"' ]; then ok "(V1) handleRequest's one caller site is the .call line, not the .bind line"; else no "(V1) handleRequest callers should be count=1 sites_at=route.js:16: ${C:-$D} $SITES"; fi

D="$( run js --callers=onlyBound )"; C="$( root_tag "$D" callers )"
if [ -n "$C" ] && [ "$( attr "$C" count )" = 0 ] && printf '%s' "$D" | grep -q '<vr in_id="binder" bind="route.js:69" into="emitter.on#arg1"/>'; then ok "(V2) onlyBound.bind( this ) is a value row into emitter.on#arg1, not a caller"; else no "(V2) onlyBound should have count=0 and a value row into emitter.on#arg1: ${D:0:400}"; fi
D="$( run js --callees=binder )"; R="$( rows "$D" )"
if has_root "$D" callees && [ -z "$R" ]; then ok "(V2) binder calls nothing (the bind is no call)"; else no "(V2) binder should have no callee row: [$R]"; fi

# ── TS ───────────────────────────────────────────────────────────────────────────────────────────────────────
D="$( run ts --callees=tsUser )"; R="$( rows "$D" )"
if has_root "$D" callees && [ "$R" = "lib.ts:target lib2.ts:defaultFn" ]; then ok "(T1/T2) tsUser → target (aliased named import) and defaultFn (default import), proven"; else no "(T1/T2) tsUser should call exactly lib.ts:target lib2.ts:defaultFn: [$R] ${D:0:120}"; fi
for pair in T3:tsNs T4:tsParam T6:tsLocalArrow; do
    arm="${pair%%:*}"; fn="${pair#*:}"
    D="$( run ts --callees="$fn" )"; R="$( rows "$D" )"
    if has_root "$D" callees && [ -z "$R" ]; then ok "($arm) $fn adds no call"; else no "($arm) $fn should have no callee row: [$R] ${D:0:120}"; fi
done
D="$( run ts --callers=defaultFn )"; R="$( rows "$D" )"
if [ "$R" = "user.ts:tsUser" ]; then ok "(T5) defaultFn.bind into a local is no call: its only caller is tsUser"; else no "(T5) defaultFn's callers should be exactly user.ts:tsUser: [$R]"; fi
for nm in handler run; do
    D="$( run ts --callers=other.ts:"$nm" )"; C="$( root_tag "$D" callers )"
    if [ -n "$C" ] && [ "$( attr "$C" count )" = 0 ]; then ok "(T4/T6) other.ts:$nm has no caller"; else no "(T4/T6) other.ts:$nm should have count=0: ${C:-$D}"; fi
done

[ "$fail" -eq 0 ] && echo "ALL PASS" || echo "SOME FAILED"
exit "$fail"
