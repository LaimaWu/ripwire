#!/usr/bin/env bash
# danglingwarncheck.sh — the lifetime fences in CMakeLists.txt's compile-time fence block actually fence.
#
# WHY. A view, a reference or a [&] capture that outlives its object is a program that reads freed or reused bytes and,
# on macOS, usually prints a plausible wrong answer (tmp/lifetime-demo, 2026-10-18: shapes A/B silent garbage on
# AppleClang, C a SEGFAULT on gcc-14 only). The fence block (`RIPWIRE_COMPILE_TIME_FENCE_FLAGS`) promotes the
# compiler's lifetime warnings to errors; this gate compiles PLANTED instances with exactly those flags — read out of
# CMakeLists.txt, never restated here — and fails when a planted bug compiles or a near-miss safe shape does not.
#
# WHAT EACH TOOL HOLDS (measured 2026-10-18; AppleClang 21, Homebrew clang 22, g++ 13.4/14.4/16.2, clang-tidy 22):
#   A  `?:` mixing a string temporary with a view   clang -Wdangling-gsl (in -Werror=dangling).   GCC: not caught.
#   B  a view of a local string returned            clang -Wreturn-stack-address; tidy bugprone-dangling-handle.  GCC: not.
#   C  range-for over a temporary's member()        GCC < 15 -Wdangling-reference. Safe from GCC 15 / clang 19 on (C++23
#                                                   P2718 extends the temporary), so no diagnostic there, and none needed.
#   D  a [&] lambda escaping its frame              tidy clang-analyzer-core.StackAddressEscape (scripts/tidycheck.sh GATING).
#                                                   No compiler flag catches it on any of the five.
#   L  a temporary bound through an RW_LIFETIMEBOUND parameter (src/infra/platform.h)   clang -Wdangling.
# So each planted bug must be refused by the tool that holds it, on every one of those tools found on this host.
# A tool that is absent is SKIPPED by name; the gate passes only if at least one compiler ran.
#
# NEAR-MISS NEGATIVES (must compile clean under the same flags, on every compiler): a view used inside its
# full-expression; a const& that lifetime-extends a temporary; range-for over a by-value container (`for (x : f())`,
# extended in every standard); over a named local's member (`for (x : local.items())`); over a temporary's DATA
# member (`for (x : makeRow().cells)`, extended in every standard); a function returning a reference to an lvalue
# argument (the shape GCC 13/14's -Wdangling-reference is known to be noisy on).
#
# Usage: test/danglingwarncheck.sh        (no ripwire binary needed)

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
TMP="$( mktemp -d )"
trap 'rm -rf "$TMP"' EXIT
fail=0; ran=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
skip(){ printf '  SKIP  %s\n' "$*"; }

echo "danglingwarncheck: BIN=n/a (compile-only gate over planted sources)"

# ─── the fence flags, per compiler family, out of the ONE place they are set ────────────────────────────────────────
fence(){   # $1 = clang | gcc
    python3 - "$ROOT/CMakeLists.txt" "$1" <<'PY'
import re, sys
text = open(sys.argv[1]).read()
sets = re.findall(r'set\(RIPWIRE_COMPILE_TIME_FENCE_FLAGS\s+(.*?)\)', text, re.S)
# the block's three arms in order: MSVC cl.exe, Clang, everything else (GCC); a GCC version sub-arm appends with list()
if len(sets) < 3:
    sys.exit('fence block not found')
flags = sets[1].split() if sys.argv[2] == 'clang' else sets[2].split()
if sys.argv[2] == 'gcc':
    for extra in re.findall(r'list\(APPEND RIPWIRE_COMPILE_TIME_FENCE_FLAGS\s+(.*?)\)', text, re.S):
        flags += extra.split()
print(' '.join(f for f in flags if f.startswith('-W')))
PY
}
CLANG_FLAGS="$( fence clang )" || { no "could not read the Clang fence flags from CMakeLists.txt"; exit 1; }
GCC_FLAGS="$( fence gcc )"     || { no "could not read the GCC fence flags from CMakeLists.txt"; exit 1; }
case " $CLANG_FLAGS " in *" -Werror=dangling "*) ok "Clang fence carries -Werror=dangling" ;; *) no "Clang fence lost -Werror=dangling: $CLANG_FLAGS" ;; esac
case " $GCC_FLAGS " in *" -Werror=dangling-reference "*) ok "GCC fence carries -Werror=dangling-reference" ;; *) no "GCC fence lost -Werror=dangling-reference: $GCC_FLAGS" ;; esac

# ─── planted sources ───────────────────────────────────────────────────────────────────────────────────────────────
cat > "$TMP/common.h" <<'CPP'
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include "infra/platform.h"
std::string makeKey( int n );
struct Row
{
    std::vector<std::string>        cells;
    const std::vector<std::string>& items() const RW_LIFETIMEBOUND { return cells; }
};
Row makeRow();
std::vector<std::string> makeList();
CPP
cat > "$TMP/A.cpp" <<'CPP'
#include "common.h"
std::size_t a( int argc ) { std::string_view key = argc > 0 ? makeKey( 42 ) : std::string_view{}; return key.size(); }
CPP
cat > "$TMP/B.cpp" <<'CPP'
#include "common.h"
std::string_view b( int n ) { std::string s = makeKey( n ); return s; }
CPP
cat > "$TMP/C.cpp" <<'CPP'
#include "common.h"
std::size_t c() { std::size_t n = 0; for( const std::string& s : makeRow().items() ) { n += s.size(); } return n; }
CPP
cat > "$TMP/D.cpp" <<'CPP'
#include "common.h"
std::function<std::string()> d( int n ) { std::string name = makeKey( n ); return [ & ] { return "hello " + name; }; }
CPP
cat > "$TMP/L.cpp" <<'CPP'
#include "common.h"
const std::string& pick( const std::string& s RW_LIFETIMEBOUND ) { return s; }
std::size_t l() { const std::string& r = pick( makeKey( 1 ) ); return r.size(); }
CPP
cat > "$TMP/neg.cpp" <<'CPP'
#include "common.h"
const std::string& pick( const std::string& s RW_LIFETIMEBOUND ) { return s; }
const std::string& longer( const std::string& a, const std::string& b ) { return a.size() >= b.size() ? a : b; }
std::size_t viewSize( std::string_view v ) { return v.size(); }
std::size_t neg()
{
    std::size_t n = viewSize( makeKey( 1 ) );                        // the view dies with the full-expression: safe
    n += std::string_view( makeKey( 2 ) ).size();                     // same
    const std::string& extended = makeKey( 3 );                      // const& extends the temporary
    n += extended.size();
    for( const std::string& s : makeList() ) { n += s.size(); }      // by-value container: extended in every standard
    const Row local = makeRow();
    for( const std::string& s : local.items() ) { n += s.size(); }   // a named local outlives the loop
    for( const std::string& s : makeRow().cells ) { n += s.size(); } // a temporary's DATA member: extended
    const std::string x = makeKey( 4 ), y = makeKey( 5 );
    n += longer( x, y ).size() + pick( x ).size();                   // reference to an lvalue argument
    return n;
}
CPP

# ─── compilers on this host ────────────────────────────────────────────────────────────────────────────────────────
BREW="${HOMEBREW_PREFIX:-/opt/homebrew}"
cands=( "${CXX:-}" c++ clang++ "$BREW/opt/llvm@22/bin/clang++" clang++-22 g++ g++-13 g++-14 g++-15 g++-16 )
seen=""
extra=()
[ "$( uname -s )" = "Darwin" ] && command -v xcrun >/dev/null 2>&1 && extra=( -isysroot "$( xcrun --show-sdk-path )" )
for cc in "${cands[@]}"; do
    [ -n "$cc" ] || continue
    command -v "$cc" >/dev/null 2>&1 || continue
    ver="$( "$cc" --version 2>/dev/null | head -1 )"
    case "$seen" in *"|$ver|"*) continue ;; esac
    seen="$seen|$ver|"
    if "$cc" --version 2>/dev/null | grep -qi clang; then
        fam=clang; flags="$CLANG_FLAGS"; sys=( "${extra[@]+"${extra[@]}"}" )
        expect_fail="A B L"; expect_quiet="C D"
    else
        fam=gcc; flags="$GCC_FLAGS"; sys=()
        major="$( "$cc" -dumpversion 2>/dev/null | cut -d. -f1 )"
        if [ "${major:-0}" -lt 13 ]; then skip "$cc ($ver): below the gcc 13 source-build floor"; continue; fi
        if [ "$major" -lt 15 ]; then expect_fail="C"; expect_quiet="A B D L"; else expect_fail=""; expect_quiet="A B C D L"; fi
    fi
    ran=1
    echo "-- $cc [$fam] $ver"
    # shellcheck disable=SC2086
    build(){ "$cc" -std=c++23 -O0 -c -o /dev/null -I "$ROOT/src" -I "$TMP" "${sys[@]+"${sys[@]}"}" $flags "$TMP/$1.cpp" >"$TMP/$1.$fam.log" 2>&1; }
    if build neg && ! grep -qiE 'dangling|lifetime|stack.address|local.addr' "$TMP/neg.$fam.log"; then
        ok "$cc: near-miss negatives compile with no lifetime diagnostic"
    else
        no "$cc: a near-miss negative was refused or warned:"; sed 's/^/        /' "$TMP/neg.$fam.log" | head -12
    fi
    for p in $expect_fail; do
        if build "$p"; then
            no "$cc: planted $p COMPILED under the fence flags — the fence no longer holds it"
        elif grep -qE 'error:.*\[-Werror[=,]' "$TMP/$p.$fam.log"; then
            ok "$cc: planted $p refused ($( grep -m1 -oE '\[-Werror[=,][A-Za-z-]+\]' "$TMP/$p.$fam.log" ))"
        else
            no "$cc: planted $p failed to compile for a reason that is not a fence error:"; head -5 "$TMP/$p.$fam.log" | sed 's/^/        /'
        fi
    done
    for p in $expect_quiet; do
        if build "$p"; then
            printf '  INFO  %s: planted %s compiles (not held by this compiler; see the table above)\n' "$cc" "$p"
        elif grep -qE 'error:.*\[-Werror[=,]' "$TMP/$p.$fam.log"; then
            # e.g. GCC's -O2 -Werror=uninitialized also reads C and L as a load from a dead temporary: a bonus, not a promise
            printf '  INFO  %s: planted %s also refused, by %s (not promised)\n' "$cc" "$p" "$( grep -m1 -oE '\[-Werror[=,][A-Za-z-]+\]' "$TMP/$p.$fam.log" )"
        else
            no "$cc: planted $p failed to build where this compiler holds no fence for it — the plant itself is broken:"
            head -5 "$TMP/$p.$fam.log" | sed 's/^/        /'
        fi
    done
done

# ─── D and B under the GATING clang-tidy subset ───────────────────────────────────────────────────────────────────
GATING="$( "$ROOT/scripts/tidycheck.sh" --list | sed -n 's/^checks: //p' )"
CT=""
for c in "${CLANG_TIDY:-}" "$( command -v clang-tidy-22 2>/dev/null )" "$BREW/opt/llvm@22/bin/clang-tidy"; do
    [ -n "$c" ] && [ -x "$c" ] && "$c" --version 2>/dev/null | grep -q 'version 22\.' && { CT="$c"; break; }
done
if [ -z "$CT" ] || [ -z "$GATING" ]; then
    skip "clang-tidy 22 not found: the D (escaping [&]) and B arms of the tidy gate are not proved here"
else
    # Each plant must be refused BY NAME by the check that holds it (the table above): a non-zero tidy exit for any
    # other reason — a compile error in the plant, a missing header, a renamed check — proves nothing and is a FAIL.
    for pc in D:clang-analyzer-core.StackAddressEscape B:bugprone-dangling-handle; do
        p="${pc%%:*}"; chk="${pc#*:}"
        if "$CT" --quiet --checks="-*,$GATING" --warnings-as-errors='*' "$TMP/$p.cpp" -- -std=c++23 -I "$ROOT/src" -I "$TMP" \
               "${extra[@]+"${extra[@]}"}" >"$TMP/$p.tidy.log" 2>&1; then
            no "clang-tidy gating subset passed planted $p"
        elif grep -qF "[$chk" "$TMP/$p.tidy.log"; then
            ok "clang-tidy gating subset refuses planted $p ([$chk])"
        else
            no "clang-tidy refused planted $p but not by $chk — not proof the gate holds it:"
            grep -m5 'warning:\|error:' "$TMP/$p.tidy.log" | sed 's/^/        /'
        fi
    done
    if "$CT" --quiet --checks="-*,$GATING" --warnings-as-errors='*' "$TMP/neg.cpp" -- -std=c++23 -I "$ROOT/src" -I "$TMP" \
           "${extra[@]+"${extra[@]}"}" >"$TMP/neg.tidy.log" 2>&1; then
        ok "clang-tidy gating subset passes the near-miss negatives"
    else
        no "clang-tidy gating subset refused a near-miss negative:"; grep -m5 'warning:\|error:' "$TMP/neg.tidy.log" | sed 's/^/        /'
    fi
fi

[ "$ran" = 1 ] || { skip "no C++ compiler found on PATH: the planted arms were not proved"; exit "$fail"; }   # a lost fence flag still fails
[ "$fail" = 0 ] && printf 'ALL PASS\n' || printf 'FAILURES ABOVE\n'
exit "$fail"
