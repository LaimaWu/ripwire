#!/usr/bin/env bash
# localdeclcheck.sh — a declaration inside a function body is not an indexed symbol (train 30, lane go-local-var).
#
# WHY. queries/go/tags.scm captured every `var_declaration` / `const_declaration`, wherever it sat, so a function-local
# `var c *model.Config = model.Make()` was indexed as a top-level `var c` symbol: --whereis labelled its line a def,
# the map listed it, and — being the nearest enclosing symbol of the initializer — it took the call, so
# `--callers=Make` named `var c` and `--callees=<the function>` lost Make. Rust's `const_item` / `static_item` and
# GDScript's `const_statement` patterns had the same unanchored shape (a fn / method / closure / func-local constant
# became a top-level var). The fix anchors each pattern to its package / item / script scope in the query itself.
# A package-level Go `var ( … )` group (var_declaration > var_spec_list in this grammar) was not captured at all; it
# is now, with each spec as its own role node so a call in one spec's initializer is charged to that var.
#
# OTHER LANGUAGES (checked 2026-10-17, no change needed): TS/JS const patterns are anchored to program /
# export_statement; Python's to module; Java/C# use field_declaration (class scope); C/C++ to translation_unit /
# declaration_list; PHP const_declaration is file/class scope; Kotlin has no val/var capture; Swift's local
# property_declaration is dropped by isSwiftLocalBinding (ingest_names.h); Rust `let` and Go `x :=` are never captured.
#
# ARMS (fixture test/localdeclfix/, copied to a temp dir and committed there so --whereis has a HEAD):
#   G1 map: function-local Go var / var group / := / const / const group / func-literal var / nested-block var absent
#   G2 map: package-level var, var group (both names), const, const group, a var after the functions, and the
#      package `sharedName` present — exactly one def of it (no overloads=)
#   G3 --callers / --callees: calls in local initializers are charged to the enclosing function, never to a var;
#      the grouped package var's call goes to ITS spec (pkgGroupA), not the group's last name
#   G4 --whereis: the local var line is not kind="def"; `sharedName` has exactly one def, the package line
#   G5 --dead-code: no local name listed (a guard: --dead-code lists functions, so this is green on base too)
#   R1/R2 Rust map: fn / method / closure const and static absent; crate, mod, impl, trait, trait-impl, extern present
#   R3 Rust map: a const in a `mod` declared inside a fn body is kept once (an item scope); a const beside it is not
#   D1/D2 GDScript map: func const absent; script and inner-class const / var present
#   Z  determinism: two map runs byte-identical
#
# Usage: bash test/localdeclcheck.sh [BIN]   (BIN defaults to $RIPWIRE_BIN, then build/ripwire)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"   # this gate builds a git repo: GIT_DIR/GIT_WORK_TREE unset (gitenvhermeticcheck D)
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$PWD/$BIN"
echo "localdeclcheck: BIN=$BIN"
FIX="$ROOT/test/localdeclfix"
[ -x "$BIN" ] || { echo "FAIL: no ripwire binary at $BIN — build first"; exit 2; }
[ -d "$FIX" ] || { echo "FAIL: no fixture at $FIX"; exit 2; }
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

cp -R "$FIX" "$TMP/fx" || { echo "FAIL: could not copy the fixture"; exit 2; }
for d in go rust gd; do
    ( cd "$TMP/fx/$d" && git init -q . && git config user.email g@e && git config user.name g \
        && git add -A && git commit -qm fixture ) >/dev/null 2>&1 || { echo "FAIL: could not commit fixture $d"; exit 2; }
done

# run DIR ARGS... → stdout in $OUT; a non-zero exit or an output without its root element is a FAIL (never an absence)
OUT=""
run(){
    local dir="$1"; shift
    OUT="$( cd "$TMP/fx/$dir" && "$BIN" . --no-cache "$@" 2>"$TMP/err" )"
    local rc=$?
    if [ "$rc" -ne 0 ] || [ -z "$OUT" ]; then
        no "$dir $* exited rc=$rc / empty output: $( head -c 300 "$TMP/err" )"
        return 1
    fi
    return 0
}
# a <s ...> row of kind T named N in $OUT
hasRow(){ printf '%s' "$OUT" | grep -q "<s t=\"$1\" n=\"$2\""; }
countOf(){ printf '%s' "$OUT" | grep -o "$1" | wc -l | tr -d ' '; }

echo "== Go"
if run go; then
    MAPGO="$OUT"
    for n in localCfg localGroupA localGroupB localShort localLimit localConstA closureVar nestedVar; do
        if printf '%s' "$OUT" | grep -q "n=\"$n\""; then no "G1 function-local '$n' is a map symbol"; else ok "G1 function-local '$n' absent from the map"; fi
    done
    for n in pkgCfg pkgGroupA pkgGroupB pkgLimit pkgConstA pkgConstB pkgAfter sharedName; do
        if hasRow var "$n"; then ok "G2 package-level '$n' is a var symbol"; else no "G2 package-level '$n' missing from the map"; fi
    done
    if printf '%s' "$OUT" | grep -q '<s t="var" n="sharedName" overloads='; then no "G2 'sharedName' has overloads= (the local one is still a def)"; else ok "G2 'sharedName' is one def"; fi
fi
if run go --callers=Make; then
    if hasRow fn UseWritten && hasRow var pkgCfg; then ok "G3 --callers=Make: UseWritten and pkgCfg"; else no "G3 --callers=Make lacks UseWritten or pkgCfg"; fi
    if printf '%s' "$OUT" | grep -q 'n="localCfg"'; then no "G3 --callers=Make names the local var localCfg"; else ok "G3 --callers=Make: no local var"; fi
fi
if run go --callers=Count; then
    if hasRow fn UseWritten && hasRow fn UseClosure && hasRow var pkgAfter; then ok "G3 --callers=Count: UseWritten, UseClosure (func literal), pkgAfter"; else no "G3 --callers=Count lacks UseWritten / UseClosure / pkgAfter"; fi
    if printf '%s' "$OUT" | grep -q 'n="closureVar"\|<s t="var" n="sharedName"'; then no "G3 --callers=Count names a local var"; else ok "G3 --callers=Count: no local var"; fi
fi
if run go --callers=Group; then
    if hasRow var pkgGroupA && hasRow fn UseWritten; then ok "G3 --callers=Group: pkgGroupA (its own spec) and UseWritten"; else no "G3 --callers=Group lacks pkgGroupA or UseWritten"; fi
    if hasRow var pkgGroupB; then no "G3 --callers=Group charges the group's call to pkgGroupB"; else ok "G3 --callers=Group: not pkgGroupB"; fi
fi
if run go --callers=Build; then
    if hasRow fn UseWritten && hasRow fn UseNested; then ok "G3 --callers=Build: UseWritten (:=) and UseNested (nested block)"; else no "G3 --callers=Build lacks UseWritten or UseNested"; fi
    if printf '%s' "$OUT" | grep -q 'n="nestedVar"'; then no "G3 --callers=Build names nestedVar"; else ok "G3 --callers=Build: no local var"; fi
fi
if run go --callees=UseWritten; then
    for n in Make Group Build Count; do
        if hasRow fn "$n"; then ok "G3 --callees=UseWritten has $n"; else no "G3 --callees=UseWritten lacks $n"; fi
    done
fi
if run go --whereis=localCfg; then
    h="$( countOf '<hit [^>]*kind="[a-z]*"' )"; d="$( countOf '<hit [^>]*kind="def"' )"
    if [ "$h" -ge 1 ] 2>/dev/null && [ "$d" = 0 ]; then ok "G4 --whereis=localCfg: $h hits, none kind=def"; else no "G4 --whereis=localCfg: hits=$h defs=$d (want ≥1 hit, 0 defs)"; fi
fi
if run go --whereis=sharedName; then
    pkgLine="$( grep -n '^var sharedName' "$TMP/fx/go/app/app.go" | cut -d: -f1 )"
    d="$( countOf '<hit [^>]*kind="def"' )"
    if [ "$d" = 1 ] && printf '%s' "$OUT" | grep -q "<hit [^>]*l=\"$pkgLine\" kind=\"def\""; then ok "G4 --whereis=sharedName: one def, the package line $pkgLine"; else no "G4 --whereis=sharedName: defs=$d (want 1 at line $pkgLine)"; fi
fi
if run go --dead-code; then
    if printf '%s' "$OUT" | grep -q '<dead-code [^>]*count="[0-9][0-9]*"'; then
        if printf '%s' "$OUT" | grep -q 'n="local\|n="closureVar"\|n="nestedVar"'; then no "G5 --dead-code lists a local"; else ok "G5 --dead-code lists no local (guard)"; fi
    else no "G5 --dead-code: no <dead-code count=> root"; fi
fi

echo "== Rust"
if run rust; then
    for n in METHOD_LOCAL FN_LOCAL_LIMIT FN_LOCAL_NAME CLOSURE_LOCAL; do
        if printf '%s' "$OUT" | grep -q "n=\"$n\""; then no "R1 block-local '$n' is a map symbol"; else ok "R1 block-local '$n' absent"; fi
    done
    for n in MODULE_LIMIT MODULE_NAME INNER_LIMIT INNER_NAME ASSOC_LIMIT EXTERN_ERRNO; do
        if hasRow var "$n"; then ok "R2 item-scope '$n' present"; else no "R2 item-scope '$n' missing"; fi
    done
    # R3 a `mod` declared INSIDE a fn body is still an item scope: its const is a real item with a path
    # (fn_mod::FN_MOD_LIMIT), so it stays indexed — exactly once — while a const beside it in the fn body does not.
    if printf '%s' "$OUT" | grep -q 'n="BESIDE_MOD_LOCAL"'; then no "R3 fn-body const beside a nested mod is a map symbol"; else ok "R3 fn-body BESIDE_MOD_LOCAL absent"; fi
    m="$( countOf '<s t="var" n="FN_MOD_LIMIT"' )"
    if [ "$m" = 1 ]; then ok "R3 const in a mod declared inside a fn: FN_MOD_LIMIT present once"; else no "R3 FN_MOD_LIMIT rows=$m (want 1)"; fi
    t="$( countOf '<s t="var" n="TRAIT_SIDES"' )"
    if [ "$t" = 2 ]; then ok "R2 TRAIT_SIDES: trait and trait-impl both present"; else no "R2 TRAIT_SIDES rows=$t (want 2)"; fi
fi

echo "== GDScript"
if run gd; then
    if printf '%s' "$OUT" | grep -q 'n="LOCAL_MAX"\|n="local_hp"'; then no "D1 func-local const/var is a map symbol"; else ok "D1 func-local LOCAL_MAX / local_hp absent"; fi
    for n in SCRIPT_MAX INNER_MAX script_health inner_hp; do
        if hasRow var "$n"; then ok "D2 script/class-scope '$n' present"; else no "D2 '$n' missing"; fi
    done
fi

echo "== determinism"
if run go; then
    if [ "$OUT" = "$MAPGO" ]; then ok "Z two Go map runs byte-identical"; else no "Z two Go map runs differ"; fi
fi

if [ "$fail" = 0 ]; then echo "ALL PASS"; exit 0; fi
echo "FAILED"; exit 1
