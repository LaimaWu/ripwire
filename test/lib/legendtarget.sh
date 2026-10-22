# legendtarget.sh — the legend SIZE gates' shared verdict: STAIR-STEP TARGET + RUNAWAY GUARD (PROCESS rule 5), strict twin (rule 4).
# SOURCED (after the gate defines ok/no), never run. One copy for compactlegendcheck (U)/(L)/(M), graphlegendbudgetcheck,
# donelegendcheck, testgatelegendbudgetcheck and panellegendcheck (CHECKLIST 32); the verdict itself is test/lib/legendtarget.py.
#
# WHY. Every legend byte pin was a hard ceiling picked as "measured + a little". A ceiling that blocks a definition the answer
# needs is not helping (owner rule 5: complete answers first). So each pin is now:
#   TARGET   the old number, unchanged. Exceeding it is "explain or fail": the excess passes only when every byte of it is covered
#            by clauses registered in test/legend_over_target.tsv that RIDE on this answer AND are present-only here (the
#            attribute the clause defines is rendered in the payload). Each covering clause prints an OVER-TARGET line.
#            A smaller measure never fails.
#   GUARD    a hard RUNAWAY ceiling well above typical (compact: 8,000 B of comments per answer; the ten-verb loop: 16,000 B; a
#            full-dialect pin: 2 x its target rounded up to 500 B, lt_full_guard). A hit prints LEGEND-RUNAWAY with the clause
#            breakdown and fails: a present-only builder loop or an unbounded clause is the only thing that reaches it.
#   TWIN     RIPWIRE_LEGEND_PINS=strict (gate environment only, never a binary knob) restores today's hard-fail on today's
#            numbers, registry ignored. Every run also computes the strict verdict on the same measurements and lt_summary
#            reports it, so the old path is exercised on the current binary each time (both paths green at the base).
#
# API
#   lt_gate LABEL ID MEASURED TARGET GMEASURED GUARD DOC...   one size row; calls the gate's ok/no; returns 0 pass, 1 fail
#       An old-path twin arm (a legend minus one known reading, held at the pre-re-pin number) uses the SAME ID as its main
#       arm, so one registered clause explains both; otherwise a registered clause would still trip the twin's hard number.
#       ID is the registry surface ("ripwire.for/v1", "loop", "mcp:impact", "full:callers" …); MEASURED is what the target
#       bounds (a gate's own metric: prose subset, leading comments, all comments); GMEASURED is what the guard bounds; DOC… are
#       the answer file(s) the riding / present-only tests read.
#   lt_full_guard N    2 x N rounded up to a multiple of 500
#   lt_summary GATE    the strict-twin line for this run (call once, at the end)
#   lt_selftest        synthetic arms proving each verdict can go red (no binary; graphlegendbudgetcheck runs it)

LT_LIB="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
LT_REGISTRY="${LT_REGISTRY:-$( cd "$LT_LIB/../.." && pwd )/test/legend_over_target.tsv}"
LT_MODE="${RIPWIRE_LEGEND_PINS:-target}"
LT_N=0; LT_STRICT_OVER=""

lt_full_guard(){ echo $(( ( ( 2 * $1 ) + 499 ) / 500 * 500 )); }

lt_gate(){
    local label="$1" id="$2" meas="$3" tgt="$4" gmeas="$5" guard="$6"; shift 6
    local out first
    out="$( python3 "$LT_LIB/legendtarget.py" verdict "$id" "$meas" "$tgt" "$gmeas" "$guard" "$LT_MODE" "$LT_REGISTRY" "$@" 2>&1 )"
    first="${out%%$'\n'*}"
    LT_N=$(( LT_N + 1 ))
    case "$meas" in ''|*[!0-9]*) ;; *) case "$tgt" in ''|*[!0-9]*) ;; *) [ "$meas" -le "$tgt" ] || LT_STRICT_OVER="$LT_STRICT_OVER $id($meas>$tgt)" ;; esac ;; esac
    case "$first" in
        WITHIN)
            ok "$label: $meas B <= target $tgt B (runaway guard $guard B on $gmeas B)"; return 0 ;;
        EXPLAINED*)
            printf '%s\n' "$out" | sed -n '2,$p' | sed 's/^/  /'
            ok "$label: $meas B > target $tgt B — ${first#EXPLAINED } (runaway guard $guard B on $gmeas B)"; return 0 ;;
        RUNAWAY*)
            no "LEGEND-RUNAWAY $label: ${first#RUNAWAY }"; printf '%s\n' "$out" | sed -n '2,$p' | sed 's/^/  /'; return 1 ;;
        STRICT*|UNEXPLAINED*|NOTPRESENT*|BADINPUT*)
            no "$label: ${first#* }"; printf '%s\n' "$out" | sed -n '2,$p' | sed 's/^/  /'; return 1 ;;
        *)
            no "$label: the verdict helper answered nothing usable ('$( printf '%s' "$first" | head -c 160 )')"; return 1 ;;
    esac
}

lt_summary(){
    local gate="$1" v
    v="$( python3 "$LT_LIB/legendtarget.py" validate "$LT_REGISTRY" 2>&1 )"
    case "$v" in OK*) ;; *) no "($gate registry) test/legend_over_target.tsv is malformed: $v" ;; esac
    case "$LT_MODE" in target|strict) ;; *) no "($gate strict twin) RIPWIRE_LEGEND_PINS='$LT_MODE' is neither unset, target nor strict" ;; esac
    if [ "$LT_N" -eq 0 ]; then
        no "($gate strict twin) no size row was measured — the gate's size arms did not run"
    elif [ -z "$LT_STRICT_OVER" ]; then
        ok "($gate strict twin) all $LT_N size rows within today's pins (RIPWIRE_LEGEND_PINS=strict passes on this binary; mode=$LT_MODE)"
    elif [ "$LT_MODE" = strict ]; then
        no "($gate strict twin) over today's pins:$LT_STRICT_OVER"
    else
        printf '  INFO  (%s strict twin) over today'"'"'s pins, explained above as OVER-TARGET (RIPWIRE_LEGEND_PINS=strict fails these):%s\n' "$gate" "$LT_STRICT_OVER"
    fi
}

# Synthetic arms (CHECKLIST 1: each verdict can go red). A fake answer whose legend exceeds a target by one clause, then:
# unregistered -> UNEXPLAINED; registered + its attribute rendered -> EXPLAINED with an OVER-TARGET line; registered but the
# attribute absent -> NOTPRESENT; the excess larger than the registered bytes -> UNEXPLAINED; strict mode -> STRICT;
# past the guard -> RUNAWAY with a breakdown; a non-numeric measure -> BADINPUT; a malformed registry -> BAD.
lt_selftest(){
    local d bad="" r
    d="$( mktemp -d )" || { no "(lt-selftest) mktemp failed"; return 1; }
    printf '<!-- base legend [x= a thing] [extra_clause= rides only with extra_clause="1" on a row] --><r x="1"><s extra_clause="1"/></r>' >"$d/with"
    printf '<!-- base legend [x= a thing] [extra_clause= rides only with extra_clause="1" on a row] --><r x="1"><s/></r>' >"$d/without"
    printf '# surface\tclause\tbytes\tattr\tmarker\treason\n' >"$d/empty.tsv"
    printf 'syn:v\textra\t60\t extra_clause="\t[extra_clause= rides only\tsynthetic: the row carries extra_clause=\n' >"$d/reg.tsv"
    printf 'syn:v\textra\t10\t extra_clause="\t[extra_clause= rides only\tsynthetic: too few bytes registered\n' >"$d/small.tsv"
    printf 'syn:v\textra\tmany\t extra_clause="\t[extra_clause= rides only\tbad bytes\n' >"$d/bad.tsv"
    v(){ python3 "$LT_LIB/legendtarget.py" verdict "$@" | head -1 | cut -d' ' -f1; }
    [ "$( v syn:v 95 50 95 8000 target "$d/empty.tsv" "$d/with" )" = UNEXPLAINED ] || bad="$bad unregistered"
    [ "$( v syn:v 95 50 95 8000 target "$d/reg.tsv"   "$d/with" )" = EXPLAINED ]   || bad="$bad registered"
    python3 "$LT_LIB/legendtarget.py" verdict syn:v 95 50 95 8000 target "$d/reg.tsv" "$d/with" | grep -q '^OVER-TARGET syn:v measured=95 target=50: clause extra 60 B' || bad="$bad over-target-line"
    [ "$( v syn:v 95 50 95 8000 target "$d/reg.tsv"   "$d/without" )" = NOTPRESENT ] || bad="$bad not-present-only"
    [ "$( v syn:v 95 50 95 8000 target "$d/small.tsv" "$d/with" )" = UNEXPLAINED ] || bad="$bad under-registered"
    [ "$( v syn:v 95 50 95 8000 strict "$d/reg.tsv"   "$d/with" )" = STRICT ]      || bad="$bad strict"
    [ "$( v syn:v 45 50 45 8000 strict "$d/empty.tsv" "$d/with" )" = WITHIN ]      || bad="$bad smaller-never-fails"
    [ "$( v syn:v 95 50 9001 8000 target "$d/reg.tsv" "$d/with" )" = RUNAWAY ]     || bad="$bad runaway"
    python3 "$LT_LIB/legendtarget.py" verdict syn:v 95 50 9001 8000 target "$d/reg.tsv" "$d/with" | grep -q '^  clause ' || bad="$bad runaway-breakdown"
    [ "$( v syn:v 9x 50 95 8000 target "$d/reg.tsv" "$d/with" )" = BADINPUT ]      || bad="$bad non-numeric"
    [ "$( v syn:v 95 50 95 8000 target "$d/bad.tsv" "$d/with" )" = BADINPUT ]      || bad="$bad bad-registry"
    [ "$( v syn:v 95 50 95 8000 target "$d/reg.tsv" "$d/missing" )" = BADINPUT ]   || bad="$bad unreadable-doc"
    r="$( python3 "$LT_LIB/legendtarget.py" validate "$d/bad.tsv" )"; case "$r" in BAD*) ;; *) bad="$bad validate-bad" ;; esac
    [ "$( lt_full_guard 3769 )" = 8000 ] && [ "$( lt_full_guard 4000 )" = 8000 ] && [ "$( lt_full_guard 1590 )" = 3500 ] || bad="$bad full-guard"
    rm -rf "$d"; unset -f v
    [ -z "$bad" ] && ok "(lt-selftest) the shared size verdict goes red on each premise: unregistered, under-registered, not present-only, strict twin, runaway (with breakdown), non-numeric, bad registry, unreadable answer; a smaller measure never fails" \
                  || no "(lt-selftest) the shared size verdict did not go red on:$bad"
}
