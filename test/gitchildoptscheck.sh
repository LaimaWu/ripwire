#!/usr/bin/env bash
# gitchildoptscheck.sh — every git child ripwire starts carries the options that neutralise repository-configured
# filter and diff programs (src/gitcmd.h, src/githarden.h).
#
# githardencheck.sh covers core.fsmonitor. This gate covers the filter drivers a repository's own config defines
# (`filter.<name>.clean/smudge/process`) and the diff programs (diff.external, diff.<driver>.command/textconv).
#
# The fixture: one commit, `*.c filter=ev`, and a local filter.ev.clean whose program only appends a line to MARK and
# passes its input through, so every time git runs it is counted. A ripwire run can leave the index refreshed,
# which changes what the next run does, so EVERY behavioural row rebuilds the fixture first.
#
# Arms:
#   (A) presence: on a fresh fixture, plain `git --no-optional-locks status` runs the filter — the fixture is live.
#       If this git does not, the gate cannot conclude and exits 2 rather than pass vacuously.
#   (B) red, only when RIPWIRE_BASE_BIN names a pre-fix binary: `<base> <fx> --situ` runs the filter.
#   (C) green: map, --situ, --quality-delta, --hotspots, --rank-by=churn and --doctor each run the filter 0 times,
#       each on a fresh fixture, and each must have answered (exit status + a stdout marker) so a run that stops
#       before git cannot pass as "0 firings".
#   (D) disclosure: stderr carries git_harden=filter-driver; stdout does not; --doctor's git-config-trust row
#       reports filter_drivers="1" filter_scope="local" filter_pinned="1", and filter_drivers="0" on a plain repo.
#   (E) MCP roots named only in a request: a server started with NO root drives situational_awareness,
#       quality_delta, stray_content, edit_check and quality_baseline at the fixture, an `analyze` whose `paths`
#       names the fixture and a second repository (a multi-root workspace), and a `batch`; a server started on the
#       second repository is asked quality_delta about the fixture. Each row: 0 firings, and the request answered.
#   (S) the fixture with its .git/config moved elsewhere and replaced by a symlink to it (git follows the link; the
#       cheap pre-scan does not read through it): --situ and an MCP quality_delta run the filter 0 times.
#   (F) a PATH shim that records each git child's argv and GIT_* environment and then execs the real git. Over the
#       same verbs, on the fixture and on a plain multi-commit repository with a working-tree edit:
#         F1 every child starts with --no-optional-locks -c core.fsmonitor=false -c log.showSignature=false
#            (the `config --get core.fsmonitor` probe excepted, as in githardencheck K);
#         F2 on the fixture, every child after the filter-driver probe carries the four filter.ev pins;
#         F3 every diff/log/show/blame child carries --no-ext-diff and --no-textconv, and every status/diff child
#            carries --ignore-submodules=dirty;
#         F4 no child saw a GIT_EXTERNAL_DIFF, GIT_ATTR_SOURCE or GIT_CONFIG_PARAMETERS ripwire did not inherit.
#       A per-verb table (children, subcommands, F1-F3 violations) is printed. Controls: a synthetic log with one
#       child lacking each option must be flagged by the same checker, and with RIPWIRE_BASE_BIN the base binary's
#       logs must fail F2 and F3.
#   (G) structural: every source line that spells a git log/diff/show/blame invocation carries --no-textconv, and
#       every `status --porcelain` carries --ignore-submodules=dirty; a mutation control on a COPY of src/ must be
#       found by the same scan.
#
# Not armed: two callers racing to probe the same root (githarden.h hardenLateRoot holds its lock across the probe).
# Both servers dispatch requests one at a time, so no request sequence can make two probes of one root overlap.
#
# Usage: RIPWIRE_BIN=build/ripwire [RIPWIRE_BASE_BIN=/path/to/pre-fix/ripwire] bash test/gitchildoptscheck.sh

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
# shellcheck source=lib/clean-env.sh
. "$ROOT/test/lib/clean-env.sh"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
BASE="${RIPWIRE_BASE_BIN:-}"
fail=0
ok(){ printf '  PASS  %s\n' "$*"; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; return 0; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first (cmake --build build -j)"; exit 2; }
REALGIT="$( command -v git )"
[ -n "$REALGIT" ] || { echo "git not on PATH — this gate needs it"; exit 2; }
echo "gitchildoptscheck: BIN=$BIN git=$( "$REALGIT" --version )${BASE:+ BASE=$BASE}"

TMP="$( mktemp -d )"
trap 'rm -rf "$TMP"' EXIT
TMP="$( cd "$TMP" && pwd -P )"
export XDG_CACHE_HOME="$TMP/cache"; mkdir -p "$XDG_CACHE_HOME"
MARK="$TMP/MARK"
CMD="$TMP/cmd.sh"
printf '#!/bin/sh\necho "cmd $* in $PWD" >> %s\ncat\n' "$MARK" > "$CMD"; chmod +x "$CMD"
FX="$TMP/f12b"
fired(){ [ -f "$MARK" ] && wc -l < "$MARK" | tr -d ' ' || echo 0; }

# The fixture: clean filter, content unchanged, only the stat data stale.
mkfx(){
    rm -rf "$FX" "$MARK" "$TMP/cfgstore"
    mkdir -p "$FX"
    ( cd "$FX" && "$REALGIT" init -q && printf 'int f(void){return 1;}\n' > f.c && printf '*.c filter=ev\n' > .gitattributes \
        && "$REALGIT" add -A && "$REALGIT" -c user.name=g -c user.email=g@g commit -qm i \
        && "$REALGIT" config filter.ev.clean "$CMD clean" && touch -t 202001010000 f.c ) >/dev/null 2>&1 \
        || { echo "fixture build failed"; exit 2; }
}

# ── (A) presence ────────────────────────────────────────────────────────────────────────────────────────
mkfx
"$REALGIT" --no-optional-locks -C "$FX" status --porcelain >/dev/null 2>&1
if [ "$( fired )" -ge 1 ]; then
    ok "(A) presence: plain git status runs the fixture's clean filter ($( fired ) call) — the fixture is live"
else
    echo "  SKIP  (A) this git does not run the clean filter on a stale-stat file here — the gate cannot conclude (exit 2)"; exit 2
fi

# run_row TAG RCS MARKER CMD…: fresh fixture, run, count. Sets RVERDICT (ok|rc|marker|fired) and RFIRED.
run_row(){
    rtag="$1"; rrcs="$2"; rmarker="$3"; shift 3
    mkfx
    "$@" > "$TMP/r-$rtag.out" 2> "$TMP/r-$rtag.err"
    RRC=$?
    RFIRED="$( fired )"
    case " $rrcs " in *" $RRC "*) ;; *) RVERDICT=rc; return 0 ;; esac
    if [ -n "$rmarker" ] && ! grep -qF -- "$rmarker" "$TMP/r-$rtag.out"; then RVERDICT=marker; return 0; fi
    if [ "$RFIRED" -eq 0 ]; then RVERDICT=ok; else RVERDICT=fired; fi
    return 0
}

# ── (B) red on the pre-fix binary, when one is named ────────────────────────────────────────────────────
if [ -n "$BASE" ]; then
    run_row "base-situ" "0" "" "$BASE" "$FX" --situ
    if [ "$RVERDICT" = fired ]; then
        ok "(B) red: the pre-fix binary's --situ ran the clean filter $RFIRED time(s)"
    else
        no "(B) the pre-fix binary did not run the filter (verdict $RVERDICT) — the red cannot be shown"
    fi
else
    echo "  NOTE  (B) RIPWIRE_BASE_BIN not set — the red arm is skipped"
fi

# ── (C) green across the git-backed verbs ───────────────────────────────────────────────────────────────
VERBS='map||0|<
situ|--situ|0|situational-awareness
quality-delta|--quality-delta|0 2|<
hotspots|--hotspots|0|<
churn|--rank-by=churn|0|<
doctor|--doctor|0 1|<doctor '
while IFS='|' read -r vtag vflag vrcs vmark; do
    # shellcheck disable=SC2086
    run_row "c-$vtag" "$vrcs" "$vmark" "$BIN" "$FX" $vflag
    case "$RVERDICT" in
        ok)     ok "(C) $vtag: exit $RRC, answered, and the clean filter did not run (0 firings)" ;;
        rc)     no "(C) $vtag: exit $RRC, expected one of {$vrcs}: $( head -c 200 "$TMP/r-c-$vtag.err" )" ;;
        marker) no "(C) $vtag: stdout lacks '$vmark' — the verb did not answer" ;;
        *)      no "(C) $vtag: the clean filter ran $RFIRED time(s): $( head -2 "$MARK" | tr '\n' ';' )" ;;
    esac
done <<< "$VERBS"
run_row "c-control-bad-flag" "0" "" "$BIN" "$FX" --no-such-flag-gitchildoptscheck
[ "$RVERDICT" = rc ] \
    && ok "(C) control: an unrecognised flag (exit $RRC, 0 firings) fails on its exit status, so a row cannot pass by not running" \
    || no "(C) control: an unrecognised flag was not caught (verdict $RVERDICT)"

# ── (D) disclosure ──────────────────────────────────────────────────────────────────────────────────────
grep -q 'git_harden=filter-driver' "$TMP/r-c-situ.err" \
    && ok "(D) stderr discloses the neutralisation: $( grep -m1 -o 'git_harden=filter-driver' "$TMP/r-c-situ.err" )" \
    || no "(D) stderr carries no git_harden=filter-driver line: $( head -c 300 "$TMP/r-c-situ.err" )"
grep -q 'git_harden=' "$TMP/r-c-situ.out" \
    && no "(D) stdout carries a git_harden= line (the disclosure leaked into the answer)" \
    || ok "(D) stdout carries no git_harden= (disclosure stays on stderr)"
DOCROW="$( grep -oE '<c n="git-config-trust"[^>]*/>' "$TMP/r-c-doctor.out" )"
case "$DOCROW" in
    *'filter_drivers="1" filter_scope="local" filter_pinned="1"'*) ok "(D) --doctor reports the pinning: $DOCROW" ;;
    *) no "(D) --doctor's git-config-trust row lacks filter_drivers=\"1\" filter_scope=\"local\" filter_pinned=\"1\": '${DOCROW:-<absent>}'" ;;
esac

# ── (E) MCP roots named only in a request ──────────────────────────────────────────────────────────────
# A second, plain repository: the other member of the multi-root row, and the startup root of the last row.
SECOND="$TMP/second"
( mkdir -p "$SECOND" && cd "$SECOND" && "$REALGIT" init -q && printf 'int g(void){return 2;}\n' > g.c && "$REALGIT" add -A \
    && "$REALGIT" -c user.name=g -c user.email=g@g commit -qm i ) >/dev/null 2>&1 || no "(E) could not build the second repository"
mcpsession(){   # ARGUMENTS-JSON for one tools/call (id 2) after the handshake
    printf '%s\n' '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2024-11-05","capabilities":{},"clientInfo":{"name":"gitchildoptscheck","version":"1"}}}'
    printf '%s\n' '{"jsonrpc":"2.0","method":"notifications/initialized"}'
    printf '{"jsonrpc":"2.0","id":2,"method":"tools/call","params":{"name":"%s","arguments":%s}}\n' "$1" "$2"
}
MCPROWS="situational_awareness|{\"path\":\"$FX\"}|
quality_delta|{\"path\":\"$FX\"}|
stray_content|{\"path\":\"$FX\"}|
edit_check|{\"path\":\"$FX\",\"symbol\":\"f\"}|
quality_baseline|{\"path\":\"$FX\"}|
analyze|{\"paths\":[\"$FX\",\"$SECOND\"]}|
batch|{\"path\":\"$FX\",\"queries\":[{\"verb\":\"stray_content\"},{\"verb\":\"callers\",\"symbol\":\"f\"}]}|
quality_delta|{\"path\":\"$FX\"}|$SECOND"
erow=0
while IFS='|' read -r everb eargs estart; do
    erow=$(( erow + 1 ))
    mcpsession "$everb" "$eargs" > "$TMP/mcp-$erow.in"
    if [ -n "$estart" ]; then
        run_row "e-$erow" "0" '"id":2,"result"' sh -c '"$1" "$3" --mcp < "$2"' _ "$BIN" "$TMP/mcp-$erow.in" "$estart"
        elabel="$everb (server started on another root)"
    else
        run_row "e-$erow" "0" '"id":2,"result"' sh -c '"$1" --mcp < "$2"' _ "$BIN" "$TMP/mcp-$erow.in"
        elabel="$everb"
        case "$eargs" in *'"paths"'*) elabel="$everb (two-root paths workspace)" ;; esac
    fi
    case "$RVERDICT" in
        ok) ok "(E) MCP request-named root, $elabel: answered (id 2 result) and the clean filter did not run" ;;
        *)  no "(E) MCP request-named root, $elabel: verdict $RVERDICT, exit $RRC, firings $RFIRED: $( head -c 200 "$TMP/r-e-$erow.err" )" ;;
    esac
done <<< "$MCPROWS"
DOCPLAIN="$( "$BIN" "$SECOND" --doctor 2>/dev/null | grep -oE '<c n="git-config-trust"[^>]*/>' )"
case "$DOCPLAIN" in
    *'filter_drivers="0"'*filter_scope*) no "(D) --doctor on a repository with no filter driver still names a scope: $DOCPLAIN" ;;
    *'filter_drivers="0"'*)              ok "(D) --doctor on a repository with no filter driver: $DOCPLAIN" ;;
    *)                                   no "(D) --doctor on a repository with no filter driver: '${DOCPLAIN:-<absent>}'" ;;
esac

# ── (S) the governing config file is a symlink ──────────────────────────────────────────────────────────
mkfx_symcfg(){
    mkfx
    mkdir -p "$TMP/cfgstore" && mv "$FX/.git/config" "$TMP/cfgstore/config" && ln -s "$TMP/cfgstore/config" "$FX/.git/config" \
        || { echo "symlinked-config fixture build failed"; exit 2; }
}
mkfx_symcfg
"$REALGIT" --no-optional-locks -C "$FX" status --porcelain >/dev/null 2>&1
[ "$( fired )" -ge 1 ] \
    && ok "(S) presence: with .git/config a symlink, plain git status still runs the clean filter (git follows the link)" \
    || no "(S) presence: git did not follow the symlinked config — the arm cannot conclude"
srow(){   # TAG CMD…: like run_row, on the symlinked-config fixture
    stag="$1"; shift
    mkfx_symcfg
    "$@" > "$TMP/r-$stag.out" 2> "$TMP/r-$stag.err"
    SRC=$?; SFIRED="$( fired )"
}
srow s-situ "$BIN" "$FX" --situ
{ [ "$SRC" -eq 0 ] && grep -qF situational-awareness "$TMP/r-s-situ.out" && [ "$SFIRED" -eq 0 ]; } \
    && ok "(S) --situ with a symlinked .git/config: answered and the clean filter did not run" \
    || no "(S) --situ with a symlinked .git/config: exit $SRC, firings $SFIRED"
mcpsession quality_delta "{\"path\":\"$FX\"}" > "$TMP/mcp-s.in"
srow s-mcp sh -c '"$1" --mcp < "$2"' _ "$BIN" "$TMP/mcp-s.in"
{ [ "$SRC" -eq 0 ] && grep -qF '"id":2,"result"' "$TMP/r-s-mcp.out" && [ "$SFIRED" -eq 0 ]; } \
    && ok "(S) MCP quality_delta with a symlinked .git/config: answered and the clean filter did not run" \
    || no "(S) MCP quality_delta with a symlinked .git/config: exit $SRC, firings $SFIRED"

# ── (F) the PATH shim: argv + GIT_* environment of every child ──────────────────────────────────────────
mkdir -p "$TMP/shim"
cat > "$TMP/shim/git" <<EOF
#!/bin/bash
# records only: one ARGV line (arguments joined by \\x1f) and one ENV line (GIT_* names=values), then the real git.
# Built in one variable and appended with ONE printf, so children started by concurrent threads do not interleave.
sep=\$'\\037'; rec='ARGV'; for a in "\$@"; do rec="\$rec\$sep\$a"; done
envs="\$( env | LC_ALL=C grep '^GIT_' | LC_ALL=C sort | tr '\\n' '\\037' )"
printf '%s\\nENV%s\\n' "\$rec" "\$envs" >> "\${GITCHILD_LOG:?}"
exec "$REALGIT" "\$@"
EOF
chmod +x "$TMP/shim/git"

# A plain repository with history, a rename and a working-tree edit, so log/diff/blame paths run too.
RICH="$TMP/rich"
( mkdir -p "$RICH" && cd "$RICH" && "$REALGIT" init -q \
    && printf 'int a(void){return 1;}\nint b(void){return a();}\n' > a.c && "$REALGIT" add -A && "$REALGIT" -c user.name=g -c user.email=g@g commit -qm one \
    && printf 'int c(void){return 3;}\n' > c.c && "$REALGIT" add -A && "$REALGIT" -c user.name=g -c user.email=g@g commit -qm two \
    && "$REALGIT" mv c.c d.c && "$REALGIT" -c user.name=g -c user.email=g@g commit -qm three \
    && printf '### T1\nIn progress.\n\xe2\x8f\xb3 pending\n' > wave.md && "$REALGIT" add -A && "$REALGIT" -c user.name=g -c user.email=g@g commit -qm four \
    && printf 'int e(void){return b();}\n' >> a.c ) >/dev/null 2>&1 || no "(F) could not build the plain history repository"
PLAINVERBS="$VERBS
plan-lint|--plan-lint=$RICH/wave.md|0|plan-lint"

# check_log FILE PINS: prints one line per violation, "F1|F2|F3|F4 <subcommand> <argv>". PINS=1 enforces F2 on every
# child after the filter-driver probe; PINS=2 on every child but a config read (the control for a binary with no probe).
subcmd_of(){   # argv fields (\x1f-separated, leading empty field) → the subcommand: skip global options and their values
    printf '%s' "$1" | awk -F '\037' '{ for( i = 2; i <= NF; ++i ) { if( $i == "-c" || $i == "-C" ) { ++i; continue } if( $i ~ /^-/ ) { continue } print $i; exit } }'
}
check_log(){
    clog="$1"; cpins="$2"; seenprobe=0
    while IFS= read -r line; do
        case "$line" in
            ENV*)
                for v in GIT_EXTERNAL_DIFF GIT_ATTR_SOURCE GIT_CONFIG_PARAMETERS; do
                    case "$line" in *"$v="*) printf 'F4 env %s\n' "$v" ;; esac
                done
                continue ;;
        esac
        argv="${line#ARGV}"
        sub="$( subcmd_of "$argv" )"
        flat="$( printf '%s' "$argv" | tr '\037' ' ' )"
        case "$flat" in *" config --get core.fsmonitor") continue ;; esac
        case "$flat" in
            " --no-optional-locks -c core.fsmonitor=false -c log.showSignature=false"*) ;;
            *) printf 'F1 %s %s\n' "$sub" "$flat" ;;
        esac
        if [ "$sub" = config ]; then
            case "$flat" in *"--show-scope"*|*"filter\\."*) seenprobe=1 ;; esac
            continue
        fi
        if { [ "$cpins" = 1 ] && [ "$seenprobe" = 1 ]; } || [ "$cpins" = 2 ]; then
            for k in clean= smudge= process= required=false; do
                case "$flat" in *" -c filter.ev.$k"*) ;; *) printf 'F2 %s %s\n' "$sub" "$flat"; break ;; esac
            done
        fi
        case "$sub" in
            diff|log|show|blame)
                case "$flat" in *" --no-ext-diff"*) ;; *) printf 'F3 %s %s\n' "$sub" "$flat" ;; esac
                case "$flat" in *" --no-textconv"*) ;; *) printf 'F3 %s %s\n' "$sub" "$flat" ;; esac ;;
        esac
        case "$sub" in
            diff|status)
                case "$flat" in *" --ignore-submodules=dirty"*) ;; *) printf 'F3 %s %s\n' "$sub" "$flat" ;; esac ;;
        esac
    done < "$clog"
    return 0
}

# shim_sweep BINARY LABEL REPO PINS VERBLIST: one log per verb; prints the table rows; accumulates violations in $TMP/viol-LABEL.
shim_sweep(){
    sbin="$1"; slabel="$2"; srepo="$3"; spins="$4"; slist="$5"
    : > "$TMP/viol-$slabel"
    while IFS='|' read -r vtag vflag vrcs vmark; do
        [ "$srepo" = "$FX" ] && mkfx
        GITCHILD_LOG="$TMP/shim-$slabel-$vtag.log"; : > "$GITCHILD_LOG"; export GITCHILD_LOG
        # shellcheck disable=SC2086
        PATH="$TMP/shim:$PATH" "$sbin" "$srepo" $vflag >/dev/null 2>&1
        unset GITCHILD_LOG
        n="$( grep -c '^ARGV' "$TMP/shim-$slabel-$vtag.log" )"
        subs="$( grep '^ARGV' "$TMP/shim-$slabel-$vtag.log" | while IFS= read -r l; do subcmd_of "${l#ARGV}"; done | sort | uniq -c | awk '{ printf "%s%s×%s", sep, $2, $1; sep=" " }' )"
        check_log "$TMP/shim-$slabel-$vtag.log" "$spins" > "$TMP/viol-$slabel-$vtag"
        cat "$TMP/viol-$slabel-$vtag" >> "$TMP/viol-$slabel"
        f1="$( grep -c '^F1' "$TMP/viol-$slabel-$vtag" )"; f2="$( grep -c '^F2' "$TMP/viol-$slabel-$vtag" )"
        f3="$( grep -c '^F3' "$TMP/viol-$slabel-$vtag" )"; f4="$( grep -c '^F4' "$TMP/viol-$slabel-$vtag" )"
        printf '        | %-8s | %-14s | %3s | F1=%s F2=%s F3=%s F4=%s | %s\n' "$slabel" "$vtag" "$n" "$f1" "$f2" "$f3" "$f4" "$subs"
        echo "$n" >> "$TMP/count-$slabel"
    done <<< "$slist"
    return 0
}

echo "  ----  (F) shim table: repo | verb | children | violations | subcommands"
rm -f "$TMP"/count-*
shim_sweep "$BIN" fixture "$FX" 1 "$VERBS"
shim_sweep "$BIN" plain "$RICH" 0 "$PLAINVERBS"
for lbl in fixture plain; do
    total="$( awk '{ s += $1 } END { print s + 0 }' "$TMP/count-$lbl" )"
    nv="$( wc -l < "$TMP/viol-$lbl" | tr -d ' ' )"
    [ "$total" -ge 10 ] \
        && ok "(F) presence ($lbl): the shim logged $total git children across the verb sweep" \
        || no "(F) presence ($lbl): only $total git children logged — the arm cannot conclude"
    [ "$nv" -eq 0 ] \
        && ok "(F) every git child ($lbl) carried F1-F3 and saw no injected F4 variable" \
        || no "(F) $nv violation(s) ($lbl): $( head -3 "$TMP/viol-$lbl" | tr '\n' ';' )"
done
for s in diff log blame status; do
    grep -h '^ARGV' "$TMP"/shim-plain-*.log "$TMP"/shim-fixture-*.log 2>/dev/null | while IFS= read -r l; do subcmd_of "${l#ARGV}"; done | grep -qx "$s" \
        && ok "(F) coverage: the sweep exercised at least one '$s' child" \
        || no "(F) coverage: no '$s' child was exercised — F3 is unproven for it"
done

# Controls: the checker flags each missing option.
CTRL="$TMP/control.log"
printf 'ARGV\037--no-optional-locks\037-c\037core.fsmonitor=false\037-C\037/x\037status\037--porcelain\n' > "$CTRL"                                         # F1 + F3 (status)
printf 'ARGV\037--no-optional-locks\037-c\037core.fsmonitor=false\037-c\037log.showSignature=false\037config\037--show-scope\n' >> "$CTRL"                    # the probe
printf 'ARGV\037--no-optional-locks\037-c\037core.fsmonitor=false\037-c\037log.showSignature=false\037-C\037/x\037log\037-1\n' >> "$CTRL"                     # F2 + F3 (log)
printf 'ENV\037GIT_EXTERNAL_DIFF=/x\037\n' >> "$CTRL"                                                                                                       # F4
check_log "$CTRL" 1 > "$TMP/control.viol"
for want in F1 F2 F3 F4; do
    grep -q "^$want" "$TMP/control.viol" \
        && ok "(F) control: the checker flags a synthetic child missing its $want option" \
        || no "(F) control: the checker did NOT flag $want — the arm cannot fail there"
done
if [ -n "$BASE" ]; then
    rm -f "$TMP"/count-base*
    shim_sweep "$BASE" base "$FX" 2 "$VERBS" > "$TMP/base-table" 2>&1
    sed 's/^/  /' "$TMP/base-table"
    grep -q '^F2' "$TMP/viol-base" && grep -q '^F3' "$TMP/viol-base" \
        && ok "(F) control: the pre-fix binary's children fail F2 ($( grep -c '^F2' "$TMP/viol-base" )) and F3 ($( grep -c '^F3' "$TMP/viol-base" )) under the same checker" \
        || no "(F) control: the pre-fix binary passed the checker — it cannot tell fixed from unfixed"
fi

# ── (G) structural ───────────────────────────────────────────────────────────────────────────────────────
# A line spells an executed git invocation when it hands a "log|diff|show|blame …" literal to one of the git
# entry points (gitCmd and the wrappers that prepend it). Display-only strings (situ.h's printed hint,
# editplan.h's next= hint) do not go through them.
structural(){
    grep -rnE '(gitCmd|gitOneLine|gitCapture|gitListChecked|pinned \+|shSingleQuote\( (root|repoRoot) \) \+).*"[[:space:]]*(log|diff|show|blame)( |")|^[[:space:]]*\+ "[[:space:]]*(log|diff|blame)[[:space:]]' "$1/src" 2>/dev/null \
        | grep -v -- '--no-textconv' | sed "s|^$1/||"
    grep -rnE '"status --porcelain' "$1/src" 2>/dev/null | grep -v -- '--ignore-submodules=dirty' | sed "s|^$1/||"
}
GOFF="$( structural "$ROOT" )"
GSEEN="$( grep -rnE -- '--no-textconv' "$ROOT/src" | wc -l | tr -d ' ' )"
[ "$GSEEN" -ge 20 ] \
    && ok "(G) presence: $GSEEN source lines carry --no-textconv" \
    || no "(G) presence: only $GSEEN source lines carry --no-textconv — the scan cannot be reading src/"
[ -z "$GOFF" ] \
    && ok "(G) every executed git log/diff/show/blame spelling carries --no-textconv; every status --porcelain carries --ignore-submodules=dirty" \
    || no "(G) unhardened spelling(s): $( printf '%s' "$GOFF" | head -3 | tr '\n' ';' )"
mkdir -p "$TMP/gmut" && cp -R "$ROOT/src" "$TMP/gmut/src"
printf '\ninline std::string gitGateMutationD( const std::string& root ) { return gitOneLine( root, "log -1 --format=%%H" ); }\n' >> "$TMP/gmut/src/quality.h"
printf '\ninline std::string gitGateMutationE( const std::string& root ) { return gitOneLine( root, "status --porcelain" ); }\n' >> "$TMP/gmut/src/quality.h"
GMUT="$( structural "$TMP/gmut" )"
for gm in gitGateMutationD gitGateMutationE; do
    printf '%s\n' "$GMUT" | grep -q "$gm" \
        && ok "(G) mutation control: the same scan finds $gm" \
        || no "(G) mutation control: the scan did not find $gm — the arm cannot fail there"
done

[ "$fail" -eq 0 ] && echo "gitchildoptscheck: PASS" || echo "gitchildoptscheck: FAIL"
exit "$fail"
