#!/usr/bin/env bash
# situhunkcheck.sh — --situ's SYMBOL-GRANULAR change set: the blast radius seeds from the symbols the diff
# actually touched, not every symbol of every changed file — with the conservative file-granular fallback
# disclosed, never silent.
#
#   test/situhunkcheck.sh
#   RIPWIRE_BIN=asan/ripwire test/situhunkcheck.sh
#
# WHY THIS EXISTS. Field report (PR #302 thread, a 759-file Flask monolith): a two-function edit in a
# 37,000-line routes.py answered `--situ` with 117 suggested tests and a 1,414-symbol blast radius, with
# unrelated payment/encounter tests ranked highly "because routes.py is shared". The cause is the change
# set's granularity: --situ seeded the caller walk from EVERY symbol of every changed file, so one edited
# function in a shared monolith file dragged in every caller of every other function in it. Reproduced on
# django at depth-1: a 3-line edit inside QuerySet.exists() answered 4,748 symbols across 442 files and
# 323 tests — most of the suite.
#
# THE FIX THIS GATE PINS. When git hunk evidence exists (working tree vs HEAD, `diff --unified=0`), a symbol
# is IN the change set when a hunk's new-side range overlaps its span; the walk seeds from those symbols
# only. The fallback is per-file and conservative: a hunk that no BODY-BEARING symbol span contains — a
# module-level edit, a deletion (newCount=0), a rename with no content hunks, a decorator-only edit —
# returns its WHOLE FILE to file-granular treatment, because such an edit can affect every reader of the
# file and the name-based graph does not model reads. Every arm below is one of those two treatments, and
# the report NAMES which one applied (change_set: granularity=… files_narrowed=… files_file_granular=…
# hunks=… located=… unlocated=…) — the honesty contract: a narrower answer must say what narrowed it.
#
# THE FIXTURE is the field report in miniature: one shared routes file with a checkout function (the edit
# target) and an unrelated payment function, each with its own test file. File-granular treatment lists
# BOTH tests for an edit inside the checkout function alone; symbol-granular lists only the checkout test.
# That exclusion is the money assertion.
#
# Exit 0 = ALL PASS, non-zero = SOME FAILED.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first (cmake --build build -j)"; exit 2; }
command -v git >/dev/null 2>&1 || { echo "git required"; exit 2; }

# ── the fixture: one shared "monolith" routes file, two independent features, two test files ────────────
FX="$TMP/fx"
mkdir -p "$FX/app" "$FX/tests"
cat > "$FX/app/routes.py" <<'EOF'
CUTOFF_MINUTES = 15


def validate_booking(slot):
    return slot


def checkout_entry(slot):
    return validate_booking(slot)


def payment_charge(amount):
    return amount * 2


def encounter_close(enc):
    return enc
EOF
cat > "$FX/tests/test_checkout.py" <<'EOF'
from app.routes import checkout_entry


def test_checkout_slot():
    return checkout_entry("10:00")
EOF
cat > "$FX/tests/test_payment.py" <<'EOF'
from app.routes import payment_charge


def test_payment_charge():
    return payment_charge(5)
EOF
( cd "$FX" && git init -q -b main >/dev/null 2>&1
  git config user.email rw@example.invalid; git config user.name ripwire
  git add -A >/dev/null 2>&1
  GIT_AUTHOR_DATE='2026-01-01T00:00:00 +0000' GIT_COMMITTER_DATE='2026-01-01T00:00:00 +0000' \
    git commit -q -m seed >/dev/null 2>&1 ) || { echo "git fixture setup failed"; exit 2; }

cs_line(){ grep -m1 'change_set:' "$1"; }
has(){ case "$( cat "$1" )" in *"$2"*) return 0 ;; *) return 1 ;; esac; }
sec2(){ sed -n '/\[2\] tests to run/,/\[3\]/p' "$1"; }

# ── (a) THE NARROWED ARM — an edit inside ONE function seeds only that function ─────────────────────────
python3 - "$FX/app/routes.py" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
old = '''def checkout_entry(slot):
    return validate_booking(slot)'''
new = '''def checkout_entry(slot):
    if slot is None:
        return validate_booking("00:00")
    return validate_booking(slot)'''
assert old in s
open(p, "w").write(s.replace(old, new, 1))
EOF
OUT_A="$TMP/a.txt"
( cd "$FX" && "$BIN" . --situ --no-cache >"$OUT_A" 2>/dev/null )
[ -s "$OUT_A" ] || { no "(a) the fixture produced no --situ report at all — every arm below would be a false green"; echo "situhunkcheck: SOME FAILED"; exit 1; }

CS="$( cs_line "$OUT_A" )"
if [ -z "$CS" ]; then
  no "(a) the report carries no change_set: line — the granularity disclosure is missing entirely"
else
  for tok in 'granularity=symbol' 'changed_symbols=1' 'files_narrowed=1' 'files_file_granular=0' 'diff_hunks_unlocated=0'; do
    case "$CS" in *"$tok"*) ok "(a) change_set carries $tok" ;; *) no "(a) change_set lacks $tok: $CS" ;; esac
  done
fi
# the money assertion: the checkout test is recommended, the UNRELATED payment test is not.
if sec2 "$OUT_A" | grep -q 'test_checkout.py'; then
  ok "(a) [2] recommends the checkout test that reaches the edited function"
else
  no "(a) [2] does not recommend tests/test_checkout.py"
fi
if sec2 "$OUT_A" | grep -q 'test_payment.py'; then
  no "(a) [2] still recommends tests/test_payment.py — the file-granular blast radius the field report complained about"
else
  ok "(a) [2] excludes tests/test_payment.py — the unrelated feature's test no longer rides a shared file"
fi
# the blast radius itself shrank below the file's own symbol count.
FS="$( printf '%s' "$CS" | sed -n 's/.*file_symbols=\([0-9]*\).*/\1/p' )"
BR="$( grep -m1 '\[1\] blast radius' "$OUT_A" | sed -n 's/.*blast radius: \([0-9]*\) symbols.*/\1/p' )"
if [ -n "$FS" ] && [ -n "$BR" ] && [ "$BR" -lt "$FS" ]; then
  ok "(a) [1] blast radius ($BR symbols) is narrower than the changed file's own symbol count ($FS)"
else
  no "(a) [1] blast radius ($BR) is not narrower than file_symbols ($FS) — narrowing did not reach the walk"
fi

# ── (f) determinism on the narrowed arm ─────────────────────────────────────────────────────────────────
( cd "$FX" && "$BIN" . --situ --no-cache >"$TMP/a2.txt" 2>/dev/null )
if cmp -s "$OUT_A" "$TMP/a2.txt"; then ok "(f) the narrowed report is byte-identical across two runs"
else no "(f) the narrowed report is not deterministic"; fi

# ── (e) MCP parity — the situational_awareness verb narrows on the SAME evidence and says so ────────────
INNER="$( printf '%s\n' \
  '{"jsonrpc":"2.0","id":1,"method":"initialize"}' \
  '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"situational_awareness","arguments":{"path":"'"$FX"'"}}}' \
  | "$BIN" --mcp 2>/dev/null | tail -1 | python3 -c '
import sys, json
r = json.load(sys.stdin)
print(r["result"]["content"][0]["text"] if "result" in r else "__ERROR__")
' )"
case "$INNER" in
  __ERROR__*|"") no "(e) the MCP verb returned no payload" ;;
  *)
    case "$INNER" in *'"change_granularity":"symbol"'*) ok "(e) MCP JSON carries change_granularity=symbol" ;;
                        *) no "(e) MCP JSON lacks change_granularity: $( printf '%s' "$INNER" | head -c 300 )" ;; esac
    case "$INNER" in *test_checkout.py*) ok "(e) MCP tests_to_run holds the checkout test" ;;
                        *) no "(e) MCP tests_to_run lacks the checkout test" ;; esac
    case "$INNER" in *test_payment.py*) no "(e) MCP tests_to_run still holds the unrelated payment test" ;;
                        *) ok "(e) MCP tests_to_run excludes the unrelated payment test" ;; esac
    ;;
esac

# ── (b) THE FALLBACK ARM — a module-level edit no body-bearing symbol contains → file-granular ─────────
( cd "$FX" && git checkout -q -- app/routes.py )
python3 - "$FX/app/routes.py" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
old = "CUTOFF_MINUTES = 15"
new = "CUTOFF_MINUTES = 15\nGRACE_MINUTES = 5"
assert old in s
open(p, "w").write(s.replace(old, new, 1))
EOF
OUT_B="$TMP/b.txt"
( cd "$FX" && "$BIN" . --situ --no-cache >"$OUT_B" 2>/dev/null )
CS="$( cs_line "$OUT_B" )"
if [ -z "$CS" ]; then
  no "(b) the module-level edit's report carries no change_set: line"
else
  for tok in 'files_file_granular=1' 'files_narrowed=0'; do
    case "$CS" in *"$tok"*) ok "(b) change_set carries $tok — the unlocatable hunk fell back" ;;
                        *) no "(b) change_set lacks $tok: $CS" ;; esac
  done
  case "$CS" in *granularity=file*) ok "(b) change_set names granularity=file" ;;
                *) no "(b) change_set does not name granularity=file: $CS" ;; esac
  CS_SYM="$( printf '%s' "$CS" | sed -n 's/.*changed_symbols=\([0-9]*\).*/\1/p' )"
  FS_SYM="$( printf '%s' "$CS" | sed -n 's/.*file_symbols=\([0-9]*\).*/\1/p' )"
  if [ -n "$CS_SYM" ] && [ "$CS_SYM" = "$FS_SYM" ]; then
    ok "(b) the fallback keeps the whole file's symbols ($CS_SYM = file_symbols)"
  else
    no "(b) the fallback did NOT keep the whole file: changed_symbols=$CS_SYM file_symbols=$FS_SYM"
  fi
fi
# conservative superset: BOTH tests are recommended again, exactly as before the fix.
if sec2 "$OUT_B" | grep -q 'test_payment.py' && sec2 "$OUT_B" | grep -q 'test_checkout.py'; then
  ok "(b) [2] lists both tests — the fallback is the conservative superset"
else
  no "(b) [2] lost a test under the fallback — narrowing leaked into a file-granular case"
fi

# ── (c) THE DELETION ARM — a removed function is a hunk with newCount=0 → file-granular, disclosed ─────
( cd "$FX" && git checkout -q -- app/routes.py )
python3 - "$FX/app/routes.py" <<'EOF'
import sys
p = sys.argv[1]
s = open(p).read()
old = '''

def encounter_close(enc):
    return enc
'''
assert old in s
open(p, "w").write(s.replace(old, "", 1))
EOF
OUT_C="$TMP/c.txt"
( cd "$FX" && "$BIN" . --situ --no-cache >"$OUT_C" 2>/dev/null )
CS="$( cs_line "$OUT_C" )"
if [ -z "$CS" ]; then
  no "(c) the deletion's report carries no change_set: line"
else
  case "$CS" in *files_file_granular=1*) ok "(c) change_set falls back to file-granular on a deletion" ;;
                *) no "(c) change_set did NOT fall back on a deletion: $CS" ;; esac
  UN="$( printf '%s' "$CS" | sed -n 's/.*diff_hunks_unlocated=\([0-9]*\).*/\1/p' )"
  if [ -n "$UN" ] && [ "$UN" -ge 1 ]; then ok "(c) the unlocatable deletion hunk is disclosed (unlocated=$UN)"
  else no "(c) the deletion hunk was not counted as unlocated: $CS"; fi
fi

# ── (d) THE CLEAN-SELECTOR ARM — no hunk evidence, today's behavior byte-for-byte in substance ──────────
( cd "$FX" && git checkout -q -- app/routes.py )
OUT_D="$TMP/d.txt"
( cd "$FX" && "$BIN" . --situ=app/routes.py --no-cache >"$OUT_D" 2>/dev/null )
CS="$( cs_line "$OUT_D" )"
if [ -z "$CS" ]; then
  no "(d) the clean selector's report carries no change_set: line"
else
  case "$CS" in *granularity=file*) ok "(d) change_set names granularity=file with no hunk evidence" ;;
                *) no "(d) change_set claims narrowing with no hunk evidence: $CS" ;; esac
  case "$CS" in *diff_hunks=0*) ok "(d) change_set reports hunks=0 on a clean tree" ;;
                *) no "(d) change_set reports hunks on a clean tree: $CS" ;; esac
fi
if sec2 "$OUT_D" | grep -q 'test_payment.py'; then
  ok "(d) [2] keeps the file-granular answer for the explicit selector (unchanged behavior)"
else
  no "(d) [2] lost the payment test under the explicit selector — a clean-tree regression"
fi

if [ "$fail" -eq 0 ]; then echo "situhunkcheck: ALL PASS"; else echo "situhunkcheck: SOME FAILED"; fi
exit "$fail"
