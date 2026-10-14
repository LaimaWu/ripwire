#!/usr/bin/env bash
# selftest.sh [HARNESS] -- the harness's own arms, run by test/langidentitycheck.sh (and runnable alone, against an
# older harness to show each arm red there). Drives the harness with stub.sh binaries over the gd fixture, so it needs
# no ripwire build and takes seconds. Prints one PASS/FAIL line per arm; exit 1 when any arm fails.
set -u
HERE="$( cd "$( dirname "$0" )" && pwd )"
H="${1:-$HERE/../langidentitycheck.sh}"
S="$( mktemp -d )"
trap 'rm -rf "$S"' EXIT
fail=0
for v in base moved stderr msg stdout rc; do  # one bash per call: the variant is baked in, not exec'd through a wrapper
    { printf '#!/usr/bin/env bash\nset -- %s "$@"\n' "$v"; tail -n +2 "$HERE/stub.sh"; } > "$S/$v"
    chmod +x "$S/$v"
done
C=gd/gdscriptfix
T=$'\t'
NARM=0
# arm NAME WANT_RC WANT_REGEX [ENV=..] -- HARNESS-ARGS...: start the harness in the background; check() judges it.
# Arms run concurrently (each is mostly process start), so the whole self-test costs about one harness run.
arm(){
    local name="$1" want="$2" re="$3" envs=()
    shift 3
    while [ "$1" != "--" ]; do envs+=( "$1" ); shift; done
    shift
    NARM=$(( NARM + 1 ))
    printf '%s\t%s\t%s\n' "$name" "$want" "$re" > "$S/arm$NARM.spec"
    ( env LANGIDENTITY_SELFTEST_CHILD=1 "${envs[@]+"${envs[@]}"}" bash "$H" "$@" --langs=gd --jobs=4 > "$S/arm$NARM.out" 2>&1
      echo $? > "$S/arm$NARM.rc" ) &
}
judge(){  # judge N: the rc, and every pattern of the ' ;; '-separated list present (re) or absent (!re)
    local name want res re rc out="$S/arm$1.out" hit=1
    IFS=$'\t' read -r name want res < "$S/arm$1.spec"
    rc="$( cat "$S/arm$1.rc" )"
    while [ -n "$res" ]; do
        re="${res%% ;; *}"
        case "$res" in *' ;; '*) res="${res#* ;; }" ;; *) res="" ;; esac
        case "$re" in
            '!'*) ! grep -Eq -- "${re#!}" "$out" || hit=0 ;;
            *) grep -Eq -- "$re" "$out" || hit=0 ;;
        esac
    done
    re="$( sed -n 's/^[^	]*	[^	]*	//p' "$S/arm$1.spec" )"
    if [ "$rc" = "$want" ] && [ "$hit" = 1 ]; then
        printf '  PASS  harness self-test %s: rc=%s\n' "$name" "$rc"
    else
        printf '  FAIL  harness self-test %s: rc=%s (want %s), output check /%s/ failed=%s:\n' "$name" "$rc" "$want" "$re" "$(( 1 - hit ))"
        sed 's/^/          | /' "$out" | tail -8
        fail=1
    fi
}
int(){ local f="$S/int$(( NARM + 1 )).tsv"; printf '%s\n' "$@" > "$f"; printf '%s' "$f"; }

# 1. stderr is compared, after the provenance tag and TMPDIR are normalised away -- and nothing more.
#    (Same run: <file-scope> is not picked as a top symbol, and a SAME with zero rows on both sides is counted apart.)
arm stderr-tag-moved 0 ' 0 DIFF ;; !--callers=<file-scope> ;; SAME \([1-9][0-9]* with zero rows' -- "$S/base" "$S/moved"
arm stderr-extra-line 1 'DIFF .*stderr only' -- "$S/base" "$S/stderr"
arm stderr-message-changed 1 'DIFF .*stderr only' -- "$S/base" "$S/msg"
arm exit-status-changed 1 'ERROR .*rc base=0 head=1' -- "$S/base" "$S/rc"
# 2. SKIP only for an empty BASE; a BASE that is named but missing is a usage error
arm base-missing-positional 2 'no BASE binary at' -- "$S/nosuchbase" "$S/base"
arm base-missing-env 2 'no BASE binary at' RIPWIRE_BASE="$S/nosuchbase" -- "$S/base"
arm base-empty-skips 0 'SKIP' RIPWIRE_BASE= -- "$S/base"
# 3. --intended: exact rows, an existing test/*check.sh arm, an argv the plan has
arm intended-wildcard-refused 2 'no wildcards' -- "$S/base" "$S/stdout" \
    --intended="$( int "$C$T*${T}test/langidentitycheck.sh" )"
arm intended-free-text-arm-refused 2 'ARM must start' -- "$S/base" "$S/stdout" \
    --intended="$( int "$C$T--callers=alpha${T}because I said so" )"
arm intended-missing-arm-refused 2 'ARM must start' -- "$S/base" "$S/stdout" \
    --intended="$( int "$C$T--callers=alpha${T}test/nosuchgatecheck.sh" )"
arm intended-argv-not-planned 2 'not in the plan' -- "$S/base" "$S/stdout" \
    --intended="$( int "$C$T--callers=gamma${T}test/langidentitycheck.sh" )"
arm stdout-diff-unexplained 1 '2 DIFF \(0 intended\)' -- "$S/base" "$S/stdout"
arm intended-exact-rows 0 'waived 1 DIFF' -- "$S/base" "$S/stdout" \
    --intended="$( int "$C$T--callers=alpha${T}test/langidentitycheck.sh" "$C$T--callers=beta${T}test/langidentitycheck.sh arm 3" )"
# 4. corpora: an explicit --repo runs whatever --langs says; a corpus id that repeats is refused, not overwritten
mkdir -p "$S/pyrepo" "$S/dup/gdscriptfix"
arm repo-outside-langs-kept 0 'py/pyrepo.repo' -- "$S/base" "$S/base" --repo=py:"$S/pyrepo" --list
arm duplicate-corpus-id-refused 2 'duplicate corpus id\(s\): gd/gdscriptfix' -- "$S/base" "$S/base" \
    --repo=gd:"$S/dup/gdscriptfix" --list
wait
i=1
while [ "$i" -le "$NARM" ]; do judge "$i"; i=$(( i + 1 )); done
exit "$fail"
