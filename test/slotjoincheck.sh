#!/usr/bin/env bash
# slotjoincheck.sh — IX-1 SLOT-TYPE JOIN: a function stored in a struct FIELD T.f, joined to the calls through that field.
#
# Before: `--callers=cmd_split_window_exec` (tmux) listed the two `.exec =` table entries as value rows with no dispatcher,
# and `--callees=cmdq_fire_command` had no row at all for `entry->exec( cmd, item )`. A human greps `.exec =` for the
# entries and `->exec(` for the dispatcher and joins them through the declared struct type; this gate pins that join.
#
# What the change does (ingest_valuerefs.h valueSlot / slotThroughAt / noteStructBody, valuerefindex.h slotCallsFor):
#   * a value row whose slot is a struct field keeps the object's declared struct type and the field (designated `.f = fn`,
#     a positional entry of a struct whose body the file holds, an anonymous struct, a field assignment);
#   * a call through a field (`x->f( … )`, `x->a->f( … )`, `t[ i ].f( … )`) keeps the root's declared type and the chain,
#     typed hop by hop through the field-type facts (a header's struct bodies) and typedef aliases;
#   * the join prints called_by= called_at= via="slot" (same declared type) or via="slot-name" (one side's type unknown) on
#     the callers <vr> row, and <vs> rows + slot_calls= on --callees. Never a proven call: count= never moves.
#
# Near misses (each must NOT join): the same field name on another struct type (both known), a cast receiver, a C++ member
# function named like a function-pointer field of another struct in the same file.
#
# Usage: bash test/slotjoincheck.sh [BIN]   (BIN defaults to RIPWIRE_BIN, then build/ripwire)

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "slotjoincheck: BIN=$BIN"
[ -x "$BIN" ] || { echo "  FAIL  no ripwire binary at $BIN"; exit 2; }
FIX="$ROOT/test/slotjoinfix"
[ -d "$FIX" ] || { echo "  FAIL  fixture dir missing: $FIX"; exit 2; }
fail=0
ok(){ printf '  PASS  %s\n' "$*"; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

# One run per query; an answer is read only off a command that succeeded and printed its root element.
run(){
    local out rc
    out="$( cd "$FIX" && "$BIN" "$FIX" "$@" --no-cache 2>/dev/null )"; rc=$?
    if [ "$rc" -ne 0 ] || ! printf '%s' "$out" | grep -q -e '<callers ' -e '<callees ' -e '^{"of":'; then
        printf 'RUNFAIL rc=%s' "$rc"
        return 0
    fi
    printf '%s' "$out"
}
has(){   # has LABEL OUTPUT NEEDLE
    case "$2" in RUNFAIL*) no "$1 (the run failed: $2)"; return;; esac
    if printf '%s' "$2" | grep -qF -- "$3"; then ok "$1"; else no "$1 — missing: $3"; fi
}
hasnt(){ # hasnt LABEL OUTPUT NEEDLE
    case "$2" in RUNFAIL*) no "$1 (the run failed: $2)"; return;; esac
    if printf '%s' "$2" | grep -qF -- "$3"; then no "$1 — unexpected: $3"; else ok "$1"; fi
}

echo "-- callers side: called_by= called_at= via= on the <vr> row"
o="$( run --callers=cmd_alpha_exec )"
has   "A1 designated .exec = f joins the dispatcher (same type)" "$o" 'into="cmd_alpha_entry.exec" called_by="fire_command" called_at="queue.c:4" via="slot"'
hasnt "A2 the same field name on another struct type never joins" "$o" 'fire_other'
has   "A3 never a proven call: count stays 0" "$o" 'count="0"'
has   "A4 the compact legend defines called_at=/via= when they ride" "$o" 'vr called_at= via=slot|slot-name'
o="$( run --callers=cmd_alpha_exec --legend=full )"
has   "A5 the full legend clause rides with via=" "$o" 'vr called_by= called_at= via=slot|slot-name'
o="$( run --callers=other_exec )"
has   "B1 the other struct's own dispatcher joins it" "$o" 'called_by="fire_other" called_at="queue.c:8" via="slot"'
hasnt "B2 and the first struct's dispatcher does not" "$o" 'fire_command'
o="$( run --callers=copy_command )"
has   "C1 a chained receiver wme->mode->command is typed through the header's field facts" "$o" 'called_by="send_keys" called_at="mode.c:5" via="slot"'
o="$( run --callers=copy_release )"
has   "C2 a typedef alias receiver resolves to the struct" "$o" 'called_by="drop_mode" called_at="mode.c:6" via="slot"'
o="$( run --callers=t_print )"
has   "D1 a positional table entry maps to its field by the struct's field order" "$o" 'into="ground_table[0][2]" called_by="parse" called_at="table.c:13" via="slot"'
o="$( run --callers=lay_even )"
has   "D2 an anonymous struct table joins its subscripted dispatcher" "$o" 'called_by="layout_select" called_at="table.c:22" via="slot"'
o="$( run --callers=on_cb )"
has   "E1 a value whose struct type is not read (nested literal) joins by name only" "$o" 'called_by="run_outer" called_at="mode.c:9" via="slot-name"'
o="$( run --callers=opa )"
has   "F1 C++: a function-pointer field its type declares in this file" "$o" 'called_by="use" called_at="vt.cpp:8" via="slot"'
o="$( run --callers=cmd_alpha_exec --json )"
has   "G1 the JSON twin carries the same keys" "$o" '"called_by":"fire_command","called_at":"queue.c:4","via":"slot"'

echo "-- callees side: slot_calls= and <vs> rows"
o="$( run --callees=fire_command )"
has   "H1 the dispatcher lists the slot and its targets" "$o" '<vs l="4" through="entry-&gt;exec" slot="cmd_ent.exec" via="slot" targets="2" shown="2"><t n="cmd_alpha_exec" p="cmds.c:2"/><t n="cmd_beta_exec" p="cmds.c:3"/></vs>'
has   "H2 slot_calls= counts it beside count=, never in it" "$o" 'count="0" '
has   "H3 slot_calls= on the root" "$o" 'slot_calls="1"'
o="$( run --callees=fire_cast )"
hasnt "H4 a cast receiver gets no slot row" "$o" 'slot_calls='
o="$( run --callees=run )"
hasnt "H5 C++: a member function named like another struct's pointer field is no slot call" "$o" 'slot_calls='
o="$( run --callees=fire_command --json )"
has   "H6 the JSON twin of <vs>" "$o" '"vss":[{"l":4,"through":"entry->exec","slot":"cmd_ent.exec","via":"slot","targets":2,"shown":2'

echo "-- well-formed XML where the clauses ride"
if command -v xmllint >/dev/null 2>&1; then
    for q in --callers=cmd_alpha_exec --callees=fire_command; do
        o="$( cd "$FIX" && "$BIN" "$FIX" "$q" --legend=full --no-cache 2>/dev/null )"
        if [ -n "$o" ] && printf '%s' "$o" | xmllint --noout - 2>/dev/null; then ok "X $q --legend=full is well-formed"; else no "X $q --legend=full is not well-formed XML"; fi
    done
else
    echo "  SKIP  xmllint not installed (the well-formedness arm needs it)"
fi

[ "$fail" -eq 0 ] && echo "ALL PASS" || echo "SOME FAILED"
exit "$fail"
