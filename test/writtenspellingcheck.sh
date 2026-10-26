#!/usr/bin/env bash
# writtenspellingcheck.sh — a seed spelled the way the SOURCE spells it binds the one definition it names, and
# nothing else: graph.h resolveAllByWrittenSpelling, the last tier of resolveAllByNameQualified (lane qn-input).
#
# Three shapes answered "symbol not found" before the tier (RED-FIRST, recorded 2026-10-25 against the base binary
# 681b49e5: every BIND arm below failed rc=1, every REFUSE arm already refused, every KEEP arm already passed):
#   (1) a TS method as `Class.method` (TS records no Symbol::scope; the owner is read by containment);
#   (2) a module / crate / package prefix: Python `asyncio.run`, `json.dump`, Go `batcheval.Put`, Rust `my_crate::Buffer::edit`;
#   (3) a C++ owner the index keeps only the innermost label of: an out-of-line nested-class definition and a
#       namespace-qualified one.
# The REFUSE arms are the false-claim controls: an owner nothing encloses, two same-named classes, an import alias,
# a type alias, a namesake owner in another package/namespace, a prefix two crates both satisfy, and a module no
# path names. Each target calls its own helper, which is the discriminator the arms grep for.
#
# Usage:  test/writtenspellingcheck.sh [BIN]   (BIN falls back to RIPWIRE_BIN, then build/ripwire). Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
FIX="$ROOT/test/writtenspellingfix"
fail=0
ok(){ printf '  PASS  %s\n' "$*"; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first (cmake --build build -j)"; exit 2; }
echo "writtenspellingcheck: BIN=$BIN  FIX=$FIX"

# bind SUB SPEC WANT [NOT...]: --callees=SPEC on FIX/SUB exits 0, names helper WANT, and names none of NOT
bind(){
    local sub="$1" spec="$2" want="$3"; shift 3
    local out rc bad=""
    out="$( cd "$FIX/$sub" && "$BIN" "$FIX/$sub" --no-cache --callees="$spec" 2>&1 )"; rc=$?
    for n in "$@"; do printf '%s' "$out" | grep -q "n=\"$n\"" && bad="$bad $n"; done
    if [ "$rc" -eq 0 ] && printf '%s' "$out" | grep -q "n=\"$want\"" && [ -z "$bad" ]; then
        ok "bind   $sub $spec -> $want"
    else
        no "bind   $sub $spec (rc=$rc, want $want, wrong:$bad)"; printf '%s\n' "$out" | tail -c 400; echo
    fi
}
# refuse SUB SPEC: --callees=SPEC on FIX/SUB exits 1 with found="0" (today's refusal, nothing bound)
refuse(){
    local sub="$1" spec="$2" out rc
    out="$( cd "$FIX/$sub" && "$BIN" "$FIX/$sub" --no-cache --callees="$spec" 2>&1 )"; rc=$?
    if [ "$rc" -eq 1 ] && printf '%s' "$out" | grep -q 'found="0"'; then
        ok "refuse $sub $spec"
    else
        no "refuse $sub $spec bound something (rc=$rc)"; printf '%s\n' "$out" | tail -c 400; echo
    fi
}

echo "-- (1) TS Class.method by containment"
bind   ts 'RuntimeDriver.beginTransaction' tsBeginHelper tsOtherHelper
bind   ts 'RuntimeDriver::commitTransaction' tsCommitHelper
bind   ts 'Other.beginTransaction' tsOtherHelper tsBeginHelper
bind   ts 'src/driver/runtime-driver.ts:RuntimeDriver.beginTransaction' tsBeginHelper
refuse ts 'src/other.ts:RuntimeDriver.beginTransaction'
refuse ts 'DupAction.run'                      # two same-named classes: not one entity
refuse ts 'AliasDriver.beginTransaction'       # a value alias is not an owner
refuse ts 'Missing.beginTransaction'
refuse ts 'driver.RuntimeDriver.inner'         # no such member

echo "-- (2) module / crate / package prefixes"
bind   py 'asyncio.run' py_asyncio_run_helper py_runner_method_helper py_app_run_helper
bind   py 'json.dump' py_json_dump_helper py_app_dump_helper
bind   py 'asyncio.Runner.run' py_runner_method_helper py_asyncio_run_helper
refuse py 'pickle.dump'                        # stdlib namesake: no path names the module, in-tree dump not bound
refuse py 'asyncio.dump'
refuse py 'self.run'                           # no owner, no module: never a bare-name pick
bind   go 'batcheval.Put' goPutHelper goOtherPutHelper
bind   go 'batcheval.Cmd.Eval' goEvalHelper
bind   go 'other.Put' goOtherPutHelper goPutHelper
refuse go 'be.Put'                             # an import alias names no directory
refuse go 'other.Cmd.Eval'                     # namesake owner absent from that package
bind   rs 'my_crate::Buffer::edit' rs_my_edit_helper rs_other_edit_helper
bind   rs 'othercrate::buf::Buffer::edit' rs_other_edit_helper rs_my_edit_helper
refuse rs 'crate::buf::Buffer::edit'           # both crates satisfy the prefix: not one entity
refuse rs 'language::Buffer::edit'
refuse rs 'my_crate::Alias::edit'              # a type alias is not peeled

echo "-- (3) C++ written qualifier + enclosing namespaces"
bind   cpp 'ConnectionManagerImpl::ActiveStream::decodeHeaders' cppStreamHelper cppRouterHelper cppUpstreamHelper
bind   cpp 'Envoy::Http::ConnectionManagerImpl::ActiveStream::decodeHeaders' cppStreamHelper cppRouterHelper
bind   cpp 'Router::Filter::decodeHeaders' cppRouterHelper cppUpstreamHelper cppStreamHelper
bind   cpp 'Upstream::Filter::decodeHeaders' cppUpstreamHelper cppRouterHelper
bind   cpp 'HttpConnectionManager::Utility::createSingletons' cppSingletonHelper
refuse cpp 'Http::Filter::decodeHeaders'       # namesake owner in another namespace
refuse cpp 'Router::ActiveStream::decodeHeaders'
refuse cpp 'Other::Utility::createSingletons'

echo "-- siblings: Ruby, and spellings earlier tiers own (unchanged)"
bind   rb 'Shop::Cart#total' rb_cart_total_helper
refuse rb 'Shopx::Cart#total'                 # a module no path names
bind   rb 'Cart#total' rb_cart_total_helper
bind   go 'Cmd.Eval' goEvalHelper
bind   cpp 'ActiveStream::decodeHeaders' cppStreamHelper cppRouterHelper

echo "-- round trip: every member row --callees prints binds back to the same definition"
for sub in ts py go rs cpp rb; do
    for seed in $( cd "$FIX/$sub" && "$BIN" "$FIX/$sub" --no-cache 2>/dev/null | grep -oE '<s t="(method|fn)" n="[^"]+"( sc="[^"]+")?' | sed -E 's/.* n="([^"]+)"( sc="([^"]+)")?/\3::\1/; s/^:://' | sort -u ); do
        got="$( cd "$FIX/$sub" && "$BIN" "$FIX/$sub" --no-cache --callers="$seed" 2>&1 )"
        if printf '%s' "$got" | grep -q 'found="0"'; then no "round trip $sub $seed refused"; fi
    done
done
[ "$fail" -eq 0 ] && ok "round trip: every printed sc::n / n resolves again (none refused)"

[ "$fail" -eq 0 ] && echo "writtenspellingcheck: PASS" || echo "writtenspellingcheck: FAIL"
exit "$fail"
