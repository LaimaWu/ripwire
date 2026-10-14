#!/usr/bin/env bash
# langidentitycheck.sh -- the byte-identity harness every language-layer ("common code") slice runs.
#
# A slice that folds per-language code into a shared table or hook must change NOTHING observable. This runs a BASE and
# a HEAD ripwire binary over the same corpora with a fixed argv set and compares stdout bytes and the exit code.
#
# USAGE
#     bash test/langidentitycheck.sh BASE HEAD [--langs=cpp,py] [--syms=FILE] [--intended=FILE]
#                                    [--repo=LANG:DIR ...] [--jobs=N] [--timeout=S] [--keep=DIR] [--list]
#   BASE   the binary built from the commit your change started from (build it in a second worktree or build dir)
#   HEAD   the binary built from your change
#   --langs=a,b     only these language tags (cpp py ts go rs swift objc js sh java rb cs c php lua ex dart kt gd clj);
#                   default: every manifest row. A slice names the languages it touches.
#   --syms=FILE     the symbols the change touched, one NAME (or LANG:NAME) per line; run in every matching corpus that
#                   has the name in BASE's or HEAD's map, in addition to the top 5 symbols per corpus from BASE's map.
#   --intended=FILE a DIFF is a failure unless a row explains it: CORPUS-GLOB <TAB> ARGV-GLOB <TAB> ARM, where CORPUS is
#                   the id printed on the "corpus" line (cpp/massfix), ARGV the text printed on the argv line, and ARM names
#                   the pinned gate arm (or finding) that makes the change intended. All three columns are required.
#   --repo=LANG:DIR a real repository to add as a corpus under that language tag. Repeatable. Callers pass what they have
#                   (an in-sample clone of their language); the harness names none.
#   --jobs=N        argv run in parallel (default 1: the machine is usually shared)
#   --keep=DIR      write BASE/HEAD stdout and stderr of every non-SAME argv under DIR
#   --list          print the corpora that would run, and stop
#
# CORPORA   test/langidentity/manifest.tsv maps each language tag to its fixture directories (test/*fix); --repo adds more.
# ARGV      per corpus: the map, --deps --limit=100000, --skipped, --clones --limit=100000, --dead-code, --hotspots, --tree,
#           --nonlocal-state; then for the top 5 symbols per corpus from BASE's map plus the --syms names: --callers,
#           --callees, --impact, --uses, --whereis, --expand. Every run is --no-cache with a per-binary TMPDIR.
# REPORT    one line per argv: SAME | DIFF | EMPTY | ERROR with base/head wall seconds. A DIFF with no --intended row, any
#           EMPTY (a vacuous comparison) and any ERROR (non-zero exit, timeout) fails the gate. Then the self-test (HEAD
#           run with --top-k=199, or one less than the corpus has, must read as DIFF) and last the ratchet line
#           (test/langidentity/ratchet.tsv: the "== Lang::" predicates outside per-language files may not rise).
#
# With no BASE this SKIPS and exits 0, like argvdiffcheck.sh: CI has no "previous" binary. regression.sh passes one argument
# (the binary under test); RIPWIRE_BASE names the reference binary. The static checks (manifest rows, ratchet) still run.
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
DIR="$ROOT/test/langidentity"
MANIFEST="$DIR/manifest.tsv"
CODE_LANGS=( cpp py ts go rs swift objc js sh java rb cs c php lua ex dart kt gd clj )
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
abspath(){ case "$1" in /*) printf '%s' "$1" ;; *) printf '%s/%s' "$PWD" "$1" ;; esac; }

# -- arguments: arrays throughout, nothing is word-split --
POS=()
LANGS="" SYMS="" INTENDED="" JOBS=1 TIMEOUT=600 KEEP="" LIST=0
REPOS=()
for arg in "$@"; do
    case "$arg" in
        --langs=*) LANGS="${arg#--langs=}" ;;
        --syms=*) SYMS="${arg#--syms=}" ;;
        --intended=*) INTENDED="${arg#--intended=}" ;;
        --repo=*) spec="${arg#--repo=}"; REPOS+=( "${spec%%:*}:$( abspath "${spec#*:}" )" ) ;;
        --jobs=*) JOBS="${arg#--jobs=}" ;;
        --timeout=*) TIMEOUT="${arg#--timeout=}" ;;
        --keep=*) KEEP="${arg#--keep=}" ;;
        --list) LIST=1 ;;
        --*) echo "langidentitycheck: unknown option $arg"; exit 2 ;;
        *) POS+=( "$arg" ) ;;
    esac
done
case "$JOBS$TIMEOUT" in *[!0-9]*|"") echo "langidentitycheck: --jobs and --timeout must be whole numbers"; exit 2 ;; esac
BASE="${RIPWIRE_BASE:-}"
HEAD="${RIPWIRE_BIN:-$ROOT/build/ripwire}"
case ${#POS[@]} in
    0) ;;
    1) HEAD="${POS[0]}" ;;
    2) BASE="${POS[0]}"; HEAD="${POS[1]}" ;;
    *) echo "langidentitycheck: usage: langidentitycheck.sh BASE HEAD [options]"; exit 2 ;;
esac
[ -n "$BASE" ] && BASE="$( abspath "$BASE" )"
HEAD="$( abspath "$HEAD" )"

# -- static arm 1: the manifest. Runs with or without a BASE. --
manifest_check(){
    local bad=0 lang dir row n=0
    [ -f "$MANIFEST" ] || { no "manifest: $MANIFEST is missing"; return; }
    for lang in "${CODE_LANGS[@]}"; do
        row="$( awk -F'\t' -v l="$lang" '!/^#/ && $1==l {print $2}' "$MANIFEST" )"
        [ -n "$row" ] || { no "manifest: language '$lang' has no row (add a fixture dir, or '$lang<TAB>-' to say it has none)"; bad=1; }
    done
    while IFS=$'\t' read -r lang dir; do
        case "$lang" in ''|'#'*) continue ;; esac
        n=$(( n + 1 ))
        case " ${CODE_LANGS[*]} " in *" $lang "*) ;; *) no "manifest: '$lang' is not a code-language tag"; bad=1 ;; esac
        [ "$dir" = "-" ] && continue
        [ -d "$ROOT/$dir" ] || { no "manifest: $lang -> $dir is not a directory"; bad=1; }
    done < "$MANIFEST"
    # every code language the enum labels must be known here: a language appended to src/model.h needs a row
    local tags tag
    tags="$( grep -oE 'lang="[a-z|]+"' "$ROOT/src/model.h" | head -1 | sed 's/lang="//;s/"//' | tr '|' ' ' )"
    for tag in $tags; do
        case "$tag" in md|json|toml|yaml) continue ;; esac
        case " ${CODE_LANGS[*]} " in *" $tag "*) ;; *) no "manifest: src/model.h labels a language '$tag' this gate does not know (add it to CODE_LANGS and the manifest)"; bad=1 ;; esac
    done
    [ "$bad" = 0 ] && ok "manifest: $n row(s), all ${#CODE_LANGS[@]} code languages covered, every directory exists"
}

SCRATCH="$( mktemp -d )"
trap 'rm -rf "$SCRATCH"' EXIT
ratchet_only(){
    ( cd "$ROOT" && python3 -I "$DIR/runner.py" --ratchet-only --head "$HEAD" --root "$ROOT" --scratch "$SCRATCH" \
        --manifest "$MANIFEST" --ratchet "$DIR/ratchet.tsv" ) || fail=1
}

if [ -z "$BASE" ] || [ ! -x "$BASE" ]; then
    echo "langidentitycheck: SKIP — no BASE reference binary (BIN=$HEAD)"
    echo "  (run: bash test/langidentitycheck.sh BASE HEAD --langs=<touched languages>, or set RIPWIRE_BASE; the manifest check below still runs)"
    manifest_check
    if [ -x "$HEAD" ]; then
        ratchet_only
    else
        echo "  SKIP  ratchet: no binary at $HEAD to count with"
    fi
    [ "$fail" = 0 ] || echo "FAILURES ABOVE"
    exit "$fail"
fi
[ -x "$HEAD" ] || { echo "langidentitycheck: no HEAD binary at $HEAD"; exit 2; }
echo "langidentitycheck: BASE=$BASE"
echo "langidentitycheck: BIN =$HEAD"
manifest_check

ARGS=( --base "$BASE" --head "$HEAD" --root "$ROOT" --scratch "$SCRATCH" --manifest "$MANIFEST" --ratchet "$DIR/ratchet.tsv"
       --jobs "$JOBS" --timeout "$TIMEOUT" )
[ -n "$LANGS" ] && ARGS+=( --langs "$LANGS" )
[ -n "$SYMS" ] && ARGS+=( --syms "$( abspath "$SYMS" )" )
[ -n "$INTENDED" ] && ARGS+=( --intended "$( abspath "$INTENDED" )" )
[ -n "$KEEP" ] && ARGS+=( --keep "$( abspath "$KEEP" )" )
[ "$LIST" = 1 ] && ARGS+=( --list )
for r in "${REPOS[@]+"${REPOS[@]}"}"; do ARGS+=( --repo "$r" ); done
cd "$ROOT" || exit 2
python3 -I "$DIR/runner.py" "${ARGS[@]}" || fail=1
[ "$fail" = 0 ] && echo "ALL PASS" || echo "FAILURES ABOVE"
exit $fail
