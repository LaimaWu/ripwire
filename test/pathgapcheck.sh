#!/usr/bin/env bash
# pathgapcheck.sh — a reachable="0" path answer never claims "no path" when its search met a call the graph holds no
# edge for (src/pathgaps.h).
#
# THE DEFECT. --path=A,B answered `reachable="0" hint="no directed call path"` whenever the BFS over resolved call edges
# did not reach B — also when that search had walked past a DECLINED call (several same-language candidates, none
# local, nothing chose one), an UNRESOLVED call, a function handed off as a VALUE (`.then( handleResponse )`) or a call
# THROUGH a parameter. Any of those may be the hop that joins A to B, so the answer's wholeness claim was false.
#
# THE CONTRACT. When the search cone met such a call: searched= gaps= gap_syms= on the root, <gap t= n= p= gaps=> rows
# (nearest from= first, at most 3, gap_syms_capped="1" past that), a hint that says the search is incomplete, a next=
# that reads the rows' bodies — and never the text "no directed call path". When the cone met none, the old answer
# stands byte for byte. Ambiguous calls are NOT a gap (every candidate has an edge the search follows). The MCP twin
# (path_between) carries the same clause.
#
# ARMS
#   (P1) declined: a platform-split callee (two same-language defs in sibling dirs, none in the caller's) — gaps=declined.
#   (P2) value: a callback handed to .then() — gaps=value, the row names the function that hands it off.
#   (P3) through: a call through a parameter — gaps=through.
#   (P4) unresolved: a call whose only same-named definition is in another language — gaps=unresolved.
#   (P5) runaway guard: 6 gap symbols — 3 rows, gap_syms="6", gap_syms_capped="1", nearest first.
#   (P6) next= is pasteable: running it serves the gap row's body.
#   (P7) name: run(obj) calls obj.process(), bound by name alone (via="name") to the same-directory A.process only; the
#        tree's other/B.process (same language, same kind, calls the target) is never searched — gaps=name, row run.
#   (P8) next= is built with nextFlag: a gap row whose path holds a space is quoted, so the selector pastes as one argument.
#   (N1) a fully resolved graph with truly no path keeps "no directed call path" and no gap attribute.
#   (N2) a gap OUTSIDE the search cone (a declined call reachable only from elsewhere) does not count.
#   (N3) ambiguous-only cone (a k-way split the search followed) with no path keeps "no directed call path".
#   (N4) a found path (reachable="1") through a cone with gaps carries no gap clause.
#   (N5) a function passed as a value that the search ALSO reaches by a call is not a gap.
#   (N6) a via="name" call whose namesakes are a DIFFERENT language (Ruby) or a different KIND (a Python variable) is not a gap.
#   (N7) a via="name" call whose unlisted namesake the search REACHED anyway (another cone symbol calls it) is not a gap:
#        helper's B().process() lists only other/B.process, but pkg/x/A.process is in the cone through run, so it was searched.
#   (N3) above doubles as the name-gap near miss: run's obj.process() is via="name" too, but it has an edge to EVERY
#        same-language definition of process, so nothing was left unsearched.
#   (M)  MCP path_between carries the same searched/gaps/gap_syms/rows/next as the CLI (P1-P3, P7), and the same no-gap hint shape.
#   (S1-S12) the SINK-SIDE frontier (lane ix2-slice): every reachable="0" answer carries <near_sink depth= callers= rows= next=>
#        — to='s proven callers walked back to depth 2, each <ns> row classified (why= declined/value/through/name/external)
#        with one site (in= site=, met="1" when the forward search reached in=): S1 declined (htop shape: the platform build
#        the declined call could mean), S2 value (koa shape), S3 through (to= itself), S4 name, S5 external + row order,
#        S6 depth cap, S7 row cap, S8 near miss (another name's declined list), S9 a disconnected pair keeps its plain hint,
#        S10 a found path carries no block, S11 next= pastes, S12 both legends; (M) compares the block byte for byte.
#   (C1-C8) CLOSURE CONTAINMENT (lane ix2-slice, IX-2 P2; src/closures.h): --callees=F carries closures=K and one
#        <in n= p= via="closure" count=> block per named function written inside F's body (any depth), listing that
#        closure's own callees — C1 nunjucks shape (createTemplate → Template), C2 the block is in no count=, C3 Python
#        nested defs at depth 1 and 2, C4 near miss (a class member inside a function is no closure), C5 a closure-free
#        answer carries none of it, C6 --json, C7 both legends, C8 near_sink why=closure (koa shape).
#   (B1-B10) the HEDGED BRIDGE (IX-2 P1): on reachable="0", <bridge proven="0" crossings= hops=> — the fewest-crossings chain
#        over at most 2 hops with no edge (declined / value / closure / namesake), each fanning out to at most 16 — B1 htop
#        shape, B2 koa shape (two closures), B3 value, B4 namesake, B5 three crossings needed (none), B6/B7 fan-out 17 (none)
#        vs 16 (crossed), B8 a disconnected pair, B9 a found path, B10 both legends. (G1-G3) GLOB endpoints (IX-2 P4): a `*`
#        endpoint naming no definition stands for every function/method definition fitting it (to_glob="1"); G2 no match is
#        refused; G3 a plain endpoint carries no glob attribute. (M) compares <bridge> and the glob root MCP vs CLI.
#   (H1-H4) hedge-quality fixes the acceptance hand-check found: H1 `compile: function compile` (one definition spelled
#        twice) is no closure; H2 C has no named nested function, so a nested span is never a closure; H3 a bridge never
#        routes through a test/bench/demo/generated/minified symbol; H4 a chain past 8 hops is no bridge; H5 a declined member
#        call spelled like a builtin type's method (`ids.join`) is not crossed, H6 its non-builtin twin is.
#        H7/H8 (near_sink): a builtin-spelled declined call and a wrapper decorator (@property) are no way in either.
#   (K1-K4) by-name hedge rules (card K5): near_sink why=name and a bridge's namesake crossing need (1) a call of the name not
#        spelled like a builtin type's method and (2) the namesake's owner (its class in its file, else its file) owning
#        something the forward search reached — K1 owner out of reach, K2 builtin-spelled (owner in reach), K3 both,
#        K4 near miss (owner in reach, not builtin: both hedges stay; S4/B4 run on the same fixture).
#
# Usage: test/pathgapcheck.sh [BIN]   (BIN defaults to RIPWIRE_BIN, then build/ripwire)
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }

# ── fixtures ────────────────────────────────────────────────────────────────────────────────────────────────────────
mkdir -p "$TMP/decl/core" "$TMP/decl/linux" "$TMP/decl/darwin" "$TMP/val" "$TMP/thr" "$TMP/unr" "$TMP/clean" \
         "$TMP/out/core" "$TMP/out/linux" "$TMP/out/darwin" "$TMP/amb" "$TMP/many/core" "$TMP/many/linux" "$TMP/many/darwin" \
         "$TMP/found" "$TMP/valin" "$TMP/lpy/pkg/x" "$TMP/lpy/other" "$TMP/lpyx/pkg/x" "$TMP/lpyx/other" \
         "$TMP/lpyin/pkg/x" "$TMP/lpyin/other"
cat > "$TMP/decl/core/machine.c" <<'EOF'
void Machine_scan( void );
void Machine_run( void ) { Machine_scan(); }
EOF
cat > "$TMP/decl/linux/scan.c" <<'EOF'
void Process_update( void ) { }
void Machine_scan( void ) { Process_update(); }
EOF
cat > "$TMP/decl/darwin/scan.c" <<'EOF'
void Machine_scan( void ) { }
EOF
cat > "$TMP/val/app.js" <<'EOF'
function respond(ctx) { return ctx; }
function handleResponse(ctx) { return respond(ctx); }
function runMiddleware(ctx) { return Promise.resolve(ctx); }
function handleRequest(ctx) { return runMiddleware(ctx).then(handleResponse); }
function listen() { return handleRequest({}); }
module.exports = { listen };
EOF
cat > "$TMP/thr/t.py" <<'EOF'
def target():
    return 1

def apply(fn):
    return fn()

def start():
    return apply(None)

def wire():
    return apply(target)
EOF
cat > "$TMP/unr/main.py" <<'EOF'
def finish():
    return 0

def begin():
    return render_page()
EOF
cat > "$TMP/unr/render.rb" <<'EOF'
def render_page
  finish
end
EOF
cat > "$TMP/clean/c.c" <<'EOF'
static int d( void ) { return 1; }
static int c( void ) { return d(); }
static int b( void ) { return 2; }
int a( void ) { return b(); }
int z( void ) { return c(); }
EOF
# N2: the declined call lives in other(), which only elsewhere() reaches; start() -> helper() is fully resolved.
cat > "$TMP/out/core/m.c" <<'EOF'
void scan( void );
static int sink( void ) { return 1; }
static int helper( void ) { return 2; }
int start( void ) { return helper(); }
int other( void ) { scan(); return 0; }
int elsewhere( void ) { return other() + sink(); }
EOF
cat > "$TMP/out/linux/s.c" <<'EOF'
void scan( void ) { }
EOF
cat > "$TMP/out/darwin/s.c" <<'EOF'
void scan( void ) { }
EOF
# N3: two same-file definitions of one method name: obj.process() splits over both (ambiguous, every arm an edge).
cat > "$TMP/amb/a.py" <<'EOF'
class A:
    def process(self):
        return 1

class B:
    def process(self):
        return 2

def unrelated():
    return 3

def run(obj):
    return obj.process()
EOF
# P5: six functions each with a declined call (f1..f4, mid, root), all reachable from root().
cat > "$TMP/many/core/m.c" <<'EOF'
void scan( void );
static int end_( void ) { return 0; }
int f1( void ) { scan(); return 1; }
int f2( void ) { scan(); return 2; }
int f3( void ) { scan(); return 3; }
int f4( void ) { scan(); return 4; }
int mid( void ) { scan(); return f3() + f4(); }
int root( void ) { scan(); return f1() + f2() + mid(); }
EOF
cat > "$TMP/many/linux/s.c" <<'EOF'
void scan( void ) { }
EOF
cat > "$TMP/many/darwin/s.c" <<'EOF'
void scan( void ) { }
EOF
# N4: a real path through a cone that also holds a declined call.
mkdir -p "$TMP/found/core" "$TMP/found/linux" "$TMP/found/darwin"
cat > "$TMP/found/core/m.c" <<'EOF'
void scan( void );
static int goal( void ) { return 1; }
int step( void ) { scan(); return goal(); }
int go( void ) { return step(); }
EOF
cat > "$TMP/found/linux/s.c" <<'EOF'
void scan( void ) { }
EOF
cat > "$TMP/found/darwin/s.c" <<'EOF'
void scan( void ) { }
EOF
# N5: handleResponse is passed as a value AND called directly, so the search reaches it anyway.
cat > "$TMP/valin/app.js" <<'EOF'
function other() { return 1; }
function handleResponse(ctx) { return ctx; }
function runMiddleware(ctx) { return Promise.resolve(ctx); }
function handleRequest(ctx) { handleResponse(ctx); return runMiddleware(ctx).then(handleResponse); }
module.exports = { handleRequest, other };
EOF

# P7 / N6: a call bound by name alone (FE-B via="name"). In lpy the namesake other/B.process is Python, a method, and
# calls the target; in lpyx the only other process definitions are a Ruby method and a Python variable.
cat > "$TMP/lpy/pkg/x/a.py" <<'EOF'
class A:
    def process(self):
        return 1
EOF
cat > "$TMP/lpy/pkg/x/c.py" <<'EOF'
def run(obj):
    return obj.process()
EOF
cat > "$TMP/lpy/other/b.py" <<'EOF'
def target():
    return 2

class B:
    def process(self):
        return target()
EOF
cp "$TMP/lpy/pkg/x/a.py" "$TMP/lpy/pkg/x/c.py" "$TMP/lpyx/pkg/x/"
cat > "$TMP/lpyx/other/b.rb" <<'EOF'
def target
  2
end

class B
  def process
    target
  end
end
EOF
cat > "$TMP/lpyx/other/v.py" <<'EOF'
process = 3
EOF
# N7: every namesake is in the cone. run reaches A.process (by name) and helper; helper's B().process() lists B.process
# only, and its unlisted namesake A.process is searched through run. Nothing reaches target.
cp "$TMP/lpy/pkg/x/a.py" "$TMP/lpyin/pkg/x/"
cat > "$TMP/lpyin/pkg/x/c.py" <<'EOF'
from other.b import helper
def run(obj):
    obj.process()
    helper()
EOF
cat > "$TMP/lpyin/other/b.py" <<'EOF'
class B:
    def process(self):
        return 2
def helper():
    B().process()
def target():
    return 3
EOF

# One --path answer, run unpiped so rc is ripwire's; the <path ...> head and the rest are read off the saved file.
path(){   # $1 dir, $2 FROM,TO, $3 out file
    "$BIN" "$1" --no-cache --path="$2" > "$3" 2>&1
}
attr(){   # $1 file, $2 attribute name -> its value on the <path> root (empty if absent)
    python3 - "$1" "$2" <<'PY'
import re, sys
t = open(sys.argv[1], encoding="utf-8", errors="replace").read()
m = re.search(r'<path [^>]*>', t)
if not m: sys.exit(0)
a = re.search(r'\s' + re.escape(sys.argv[2]) + r'="([^"]*)"', m.group(0))
print(a.group(1) if a else "")
PY
}
rows(){ grep -o '<gap [^>]*/>' "$1"; }
premise(){   # $1 label, $2 rc, $3 file — the answer ran and printed a <path> root (checklist 14: an absence is read only off a real answer)
    if [ "$2" -ne 0 ] || ! grep -q '<path ' "$3"; then no "$1 (premise) --path ran (rc=$2) and printed a <path> root: $( head -c 300 "$3" )"; return 1; fi
    return 0
}
gapArm(){   # $1 label, $2 dir, $3 FROM,TO, $4 expected gaps=, $5 expected first row name
    local f="$TMP/$( echo "$1" | tr -cd 'A-Za-z0-9' ).xml" rc
    path "$2" "$3" "$f"; rc=$?
    premise "$1" "$rc" "$f" || return
    local g; g="$( attr "$f" gaps )"
    local first; first="$( rows "$f" | head -1 | sed 's/.* n="\([^"]*\)".*/\1/' )"
    if [ "$( attr "$f" reachable )" = "0" ] && [ "$g" = "$4" ] && ! grep -q 'no directed call path' "$f" \
       && [ -n "$( attr "$f" searched )" ] && [ "$first" = "$5" ] && grep -q 'search is incomplete' "$f"; then
        ok "$1 reachable=0, gaps=\"$g\", first row $first, no \"no directed call path\""
    else
        no "$1 expected gaps=\"$4\" first row $5 and no 'no directed call path'; got gaps=\"$g\" first=[$first]: $( grep -o '<path .*' "$f" | head -c 700 )"
    fi
}
cleanArm(){   # $1 label, $2 dir, $3 FROM,TO — the old "no path" answer, no gap clause
    local f="$TMP/$( echo "$1" | tr -cd 'A-Za-z0-9' ).xml" rc
    path "$2" "$3" "$f"; rc=$?
    premise "$1" "$rc" "$f" || return
    if [ "$( attr "$f" reachable )" = "0" ] && grep -q 'hint="no directed call path' "$f" && [ -z "$( attr "$f" searched )" ] \
       && [ -z "$( attr "$f" gaps )" ] && [ -z "$( rows "$f" )" ] && ! grep -q 'search is INCOMPLETE\|search is incomplete' "$f"; then
        ok "$1 keeps \"no directed call path\" with no gap clause"
    else
        no "$1 should keep the plain no-path answer: $( grep -o '<path .*' "$f" | head -c 700 )"
    fi
}

echo "(P) positive arms: a gap in the search cone"
gapArm "(P1) declined" "$TMP/decl" "Machine_run,Process_update" "declined:1" "Machine_run"
gapArm "(P2) value"    "$TMP/val"  "listen,respond"             "value:1"    "handleRequest"
gapArm "(P3) through"  "$TMP/thr"  "start,target"               "through:1"  "apply"
gapArm "(P4) unresolved" "$TMP/unr" "begin,finish"              "unresolved:1" "begin"

# P7's premise: the call really is bound by name alone to A.process only (else the arm would test the old resolver)
f="$TMP/lpycallees.xml"; "$BIN" "$TMP/lpy" --no-cache --callees=run > "$f" 2>&1
if [ "$( grep -o '<s [^>]*n="process"[^>]*/>' "$f" | wc -l | tr -d ' ' )" = "1" ] && grep -q '<s [^>]*n="process" p="pkg/x/a.py:2" via="name"' "$f"; then
    ok "(P7) premise: run's obj.process() is via=\"name\" to pkg/x/a.py's A.process only"
    gapArm "(P7) name" "$TMP/lpy" "run,target" "name:1" "run"
else
    no "(P7) premise: run's callees are not the one via=name A.process row: $( grep -o '<callees .*' "$f" | head -c 500 )"
fi

# P8 (CodeRabbit on #383, pathgaps.h:342): lpy with a space in a directory name. The row's p= is the same, and next= must
# quote the selector (nextFlag), else `--expand=pkg x/c.py:run` splits into two argv words when pasted.
mkdir -p "$TMP/lpysp/pkg x" "$TMP/lpysp/other"
cp "$TMP/lpy/pkg/x/a.py" "$TMP/lpy/pkg/x/c.py" "$TMP/lpysp/pkg x/"
cp "$TMP/lpy/other/b.py" "$TMP/lpysp/other/"
f="$TMP/P8space.xml"; path "$TMP/lpysp" "run,target" "$f"; rc=$?
if premise "(P8) space" "$rc" "$f"; then
    nx="$( attr "$f" next )"   # the raw attribute text: the shell quote is &apos; on the wire, as every next= with a quote is
    if [ "$( attr "$f" gaps )" = "name:1" ] && [ "$nx" = "--expand=&apos;pkg x/c.py:run&apos;" ]; then
        ok "(P8) a gap row under a directory with a space: next=\"$nx\" is one quoted selector"
        # pasteable: XML-unescape, split the way a shell would, run it, and the gap row's body is served
        python3 -c 'import html,shlex,subprocess,sys; sys.exit(subprocess.run([sys.argv[1], sys.argv[2], "--no-cache"] + shlex.split(html.unescape(sys.argv[3])), stdout=open(sys.argv[4], "wb"), stderr=subprocess.STDOUT).returncode)' \
            "$BIN" "$TMP/lpysp" "$nx" "$TMP/P8next.xml"; rc=$?
        if [ "$rc" -eq 0 ] && grep -q 'obj.process()' "$TMP/P8next.xml"; then
            ok "(P8) the quoted next= pastes as one argument and serves run's body"
        else
            no "(P8) the quoted next= did not serve run's body (rc=$rc): $( head -c 300 "$TMP/P8next.xml" )"
        fi
    else
        no "(P8) want gaps=\"name:1\" and next=\"--expand=&apos;pkg x/c.py:run&apos;\"; got gaps=\"$( attr "$f" gaps )\" next=\"$nx\""
    fi
fi

echo "(P5) runaway guard: rows capped at 3, nearest first, the cut disclosed"
f="$TMP/many.xml"; path "$TMP/many" "root,end_" "$f"; rc=$?
if premise "(P5)" "$rc" "$f"; then
    names="$( rows "$f" | sed 's/.* n="\([^"]*\)".*/\1/' | tr '\n' ' ' )"
    if [ "$( attr "$f" gap_syms )" = "6" ] && [ "$( attr "$f" gap_syms_capped )" = "1" ] && [ "$( attr "$f" gaps )" = "declined:6" ] \
       && [ "$names" = "root f1 f2 " ]; then
        ok "(P5) gap_syms=6 gap_syms_capped=1 gaps=declined:6, rows [$names] (depth 0, then depth 1 by id)"
    else
        no "(P5) gap_syms=$( attr "$f" gap_syms ) capped=$( attr "$f" gap_syms_capped ) gaps=$( attr "$f" gaps ) rows [$names]"
    fi
fi

echo "(P6) next= is pasteable and serves the gap row's body"
f="$TMP/P2value.xml"
nx="$( attr "$f" next )"
if [ "$nx" = "--expand=app.js:handleRequest" ]; then
    # shellcheck disable=SC2086
    "$BIN" "$TMP/val" --no-cache $nx > "$TMP/next.xml" 2>&1; rc=$?
    if [ "$rc" -eq 0 ] && grep -q 'then(handleResponse)' "$TMP/next.xml"; then
        ok "(P6) next=\"$nx\" serves handleRequest's body, where the value hand-off is"
    else
        no "(P6) next=\"$nx\" rc=$rc did not serve the body: $( head -c 300 "$TMP/next.xml" )"
    fi
else
    no "(P6) next= on the value arm is [$nx], expected --expand=app.js:handleRequest"
fi

echo "(N) near-miss negatives"
cleanArm "(N1) fully resolved, no path" "$TMP/clean" "a,d"
cleanArm "(N2) a gap outside the cone"  "$TMP/out"   "start,sink"
f="$TMP/ambprobe.xml"; "$BIN" "$TMP/amb" --no-cache > "$f" 2>&1
if grep -q '<s [^>]*n="run"[^>]*amb="1"' "$f"; then
    ok "(N3) premise: run's obj.process() is an ambiguous split (amb=1 on its row)"
    cleanArm "(N3) ambiguous-only cone" "$TMP/amb" "run,unrelated"
else
    no "(N3) premise: run's call is not an ambiguous split here, the arm would assert nothing: $( grep -o '<s [^>]*n="run"[^>]*>' "$f" )"
fi
f="$TMP/found.xml"; path "$TMP/found" "go,goal" "$f"; rc=$?
if premise "(N4)" "$rc" "$f"; then
    if [ "$( attr "$f" reachable )" = "1" ] && [ -z "$( attr "$f" gaps )" ] && [ -z "$( attr "$f" hint )" ] && [ -z "$( rows "$f" )" ]; then
        ok "(N4) a found path carries no gap clause and no hint"
    else
        no "(N4) found path: $( grep -o '<path .*' "$f" | head -c 500 )"
    fi
fi
cleanArm "(N5) a value the search also calls" "$TMP/valin" "handleRequest,other"
f="$TMP/lpyxcallees.xml"; "$BIN" "$TMP/lpyx" --no-cache --callees=run > "$f" 2>&1
if grep -q '<s [^>]*n="process" p="pkg/x/a.py:2" via="name"' "$f"; then
    cleanArm "(N6) via=name, namesakes of another language/kind" "$TMP/lpyx" "run,target"
else
    no "(N6) premise: run's obj.process() is not via=name to A.process here: $( grep -o '<callees .*' "$f" | head -c 500 )"
fi
f="$TMP/lpyinhelper.xml"; "$BIN" "$TMP/lpyin" --no-cache --callees=helper > "$f" 2>&1; fh_rc=$?
f2="$TMP/lpyinrun.xml"; "$BIN" "$TMP/lpyin" --no-cache --callees=run > "$f2" 2>&1; fr_rc=$?
if [ "$fh_rc" -eq 0 ] && [ "$fr_rc" -eq 0 ] \
   && grep -q '<s [^>]*n="process" p="other/b.py:2" via="name"' "$f" && ! grep -q 'p="pkg/x/a.py:2"' "$f" \
   && grep -q '<s [^>]*n="process" p="pkg/x/a.py:2"' "$f2"; then
    cleanArm "(N7) via=name, the unlisted namesake is in the searched cone" "$TMP/lpyin" "run,target"
else
    no "(N7) premise: helper lists only other/B.process by name and run reaches pkg/x/A.process (rc=$fh_rc/$fr_rc): $( grep -o '<callees .*' "$f" | head -c 300 ) | $( grep -o '<callees .*' "$f2" | head -c 300 )"
fi

echo "(S) sink-side frontier: <near_sink> on every reachable=0 answer (lane ix2-slice)"
# fixtures: a 4-deep chain (depth cap) and a sink with six caller-less callers (row cap)
mkdir -p "$TMP/deep" "$TMP/wide"
cat > "$TMP/deep/d.c" <<'EOF'
static int sink( void ) { return 1; }
static int z( void ) { return sink(); }
static int y( void ) { return z(); }
static int x( void ) { return y(); }
int w( void ) { return x(); }
int lone( void ) { return 0; }
EOF
cat > "$TMP/wide/w.c" <<'EOF'
static int sink( void ) { return 1; }
int c1( void ) { return sink(); }
int c2( void ) { return sink(); }
int c3( void ) { return sink(); }
int c4( void ) { return sink(); }
int c5( void ) { return sink(); }
int c6( void ) { return sink(); }
int lone( void ) { return 0; }
EOF
# K5 fixtures (by-name hedge rules): run(obj) calls obj.NAME(), bound by name alone to the imported A.NAME; the namesake
# other/b.py B.NAME calls target. *_in: run also reaches B.make through helper (other/h.py imports B; c.py never names B),
# so B — the namesake's owner — is in the forward reach; *_out: nothing run reaches is owned by B. NAME = process
# (no builtin type has it) or update (a dict/set method: builtinSpelledCall).
for nm in process update; do for own in in out; do
    d="$TMP/k_${nm}_$own"; mkdir -p "$d/pkg/x" "$d/other"
    printf 'class A:\n    def %s(self):\n        return 1\n' "$nm" > "$d/pkg/x/a.py"
    if [ "$own" = in ]; then
        printf 'from pkg.x.a import A\nfrom other.h import helper\ndef run(obj):\n    helper()\n    return obj.%s()\n' "$nm" > "$d/pkg/x/c.py"
        printf 'from other.b import B\ndef helper():\n    return B.make()\n' > "$d/other/h.py"
    else
        printf 'from pkg.x.a import A\ndef run(obj):\n    return obj.%s()\n' "$nm" > "$d/pkg/x/c.py"
    fi
    printf 'def target():\n    return 2\n\nclass B:\n    @staticmethod\n    def make():\n        return 0\n\n    def %s(self):\n        return target()\n' "$nm" > "$d/other/b.py"
done; done
nsblock(){ grep -o '<near_sink [^>]*>.*</near_sink>' "$1"; }
nshead(){ grep -o '<near_sink [^>]*>' "$1"; }
nsrows(){ grep -o '<ns [^>]*/>' "$1"; }
nsArm(){   # $1 label, $2 dir, $3 FROM,TO, $4 expected <near_sink …> head, $5 expected rows (concatenated, exact)
    local f="$TMP/S$( echo "$1" | tr -cd 'A-Za-z0-9' ).xml" rc
    path "$2" "$3" "$f"; rc=$?
    premise "$1" "$rc" "$f" || return
    local h r; h="$( nshead "$f" )"; r="$( nsrows "$f" | tr -d '\n' )"
    if [ "$( attr "$f" reachable )" = "0" ] && [ "$h" = "$4" ] && [ "$r" = "$5" ]; then
        ok "$1 $h $r"
    else
        no "$1 want [$4][$5]; got [$h][$r]"
    fi
}
nsArm "(S1) declined: the sink's caller is a candidate of the declined call the search met (htop shape)" "$TMP/decl" "Machine_run,Process_update" \
    '<near_sink depth="2" callers="1" rows="1" next="--callers=linux/scan.c:Machine_scan">' \
    '<ns d="1" t="fn" n="Machine_scan" p="linux/scan.c:2" why="declined:1" in="Machine_run" site="core/machine.c:2" met="1"/>'
nsArm "(S2) value: the sink's caller is handed to .then() where the search went (koa shape)" "$TMP/val" "listen,respond" \
    '<near_sink depth="2" callers="1" rows="1" next="--callers=app.js:handleResponse">' \
    '<ns d="1" t="fn" n="handleResponse" p="app.js:2" why="value:1" in="handleRequest" site="app.js:4" met="1"/>'
nsArm "(S3) through: to= itself, invoked through apply's parameter" "$TMP/thr" "start,target" \
    '<near_sink depth="2" callers="0" rows="1" next="--callers=t.py:target">' \
    '<ns d="0" t="fn" n="target" p="t.py:1" why="through:1" in="apply" site="t.py:5" met="1"/>'
nsArm "(S4) name: a by-name call went to the namesake A.process, not to the sink's caller B.process (B is in reach: K5)" "$TMP/k_process_in" "run,target" \
    '<near_sink depth="2" callers="1" rows="1" next="--callers=other/b.py:process">' \
    '<ns d="1" t="fn" n="process" p="other/b.py:9" why="name:1" in="run" site="pkg/x/c.py:5" met="1"/>'
nsArm "(S5) external: the frontier row first, then the proven caller it reaches to= through" "$TMP/clean" "a,d" \
    '<near_sink depth="2" callers="2" rows="2" next="--callers=c.c:z">' \
    '<ns d="2" t="fn" n="z" p="c.c:5" why="external"/><ns d="1" t="fn" n="c" p="c.c:2" callers="1"/>'
nsArm "(S6) depth cap: callers at depth 1 and 2 only (x, at depth 3, is not a row)" "$TMP/deep" "lone,sink" \
    '<near_sink depth="2" callers="2" rows="2" next="--callers=d.c:z">' \
    '<ns d="1" t="fn" n="z" p="d.c:2" callers="1"/><ns d="2" t="fn" n="y" p="d.c:3" callers="1"/>'
nsArm "(S7) row cap: six caller-less callers, four rows, the cut disclosed" "$TMP/wide" "lone,sink" \
    '<near_sink depth="2" callers="6" rows="6" rows_capped="1" next="--callers=w.c:c1">' \
    '<ns d="1" t="fn" n="c1" p="w.c:2" why="external"/><ns d="1" t="fn" n="c2" p="w.c:3" why="external"/><ns d="1" t="fn" n="c3" p="w.c:4" why="external"/><ns d="1" t="fn" n="c4" p="w.c:5" why="external"/>'
nsArm "(S8) near miss: a declined call whose candidates are another name gives no declined reason (N2's fixture)" "$TMP/out" "start,sink" \
    '<near_sink depth="2" callers="1" rows="1" next="--callers=core/m.c:elsewhere">' \
    '<ns d="1" t="fn" n="elsewhere" p="core/m.c:6" why="external"/>'
f="$TMP/SN1.xml"; path "$TMP/clean" "a,d" "$f"; rc=$?
if premise "(S9)" "$rc" "$f"; then
    if grep -q 'hint="no directed call path' "$f" && [ -z "$( attr "$f" gaps )" ] && [ -n "$( nsblock "$f" )" ]; then
        ok "(S9) a disconnected pair keeps \"no directed call path\" (no gap clause) beside its <near_sink>"
    else
        no "(S9) the disconnected pair lost its plain hint or its block: $( grep -o '<path .*' "$f" | head -c 600 )"
    fi
fi
f="$TMP/found.xml"
if [ -s "$f" ] && [ "$( attr "$f" reachable )" = "1" ] && ! grep -q 'near_sink' "$f"; then
    ok "(S10) a found path (N4) carries no <near_sink> and no legend clause for it"
else
    no "(S10) found path carries near_sink or did not run: $( grep -o '<path .*' "$f" | head -c 400 )"
fi
f="$TMP/SS1declinedthesinkscallerisacandidateofthedeclinedcallthesearchmethtopshape.xml"
nx="$( python3 -c 'import re,sys,html; t=open(sys.argv[1]).read(); m=re.search(r"<near_sink [^>]*next=\"([^\"]*)\"",t); print(html.unescape(m.group(1)) if m else "")' "$f" )"
# shellcheck disable=SC2086
if [ -n "$nx" ] && "$BIN" "$TMP/decl" --no-cache $nx > "$TMP/Snext.xml" 2>&1 && grep -q '<dc in="Machine_run" p="core/machine.c:2"' "$TMP/Snext.xml"; then
    ok "(S11) the block's next=\"$nx\" pastes and lists the declined site in Machine_run"
else
    no "(S11) next=[$nx] did not serve the <dc in=Machine_run> row: $( head -c 400 "$TMP/Snext.xml" 2>/dev/null )"
fi
"$BIN" "$TMP/decl" --no-cache --path=Machine_run,Process_update --legend=full > "$TMP/Sfull.xml" 2>&1
if grep -q '<near_sink callers= rows=>: the SINK-SIDE frontier' "$TMP/Sfull.xml" && grep -q 'never a hop and not proof a path exists' "$TMP/Sfull.xml" \
   && grep -q 'ns d= t= n= p= callers= why=' "$f" && grep -q 'ns in= site=' "$f" && grep -q 'near_sink depth= callers= rows=' "$f"; then
    ok "(S12) both legends define the block (full: never a hop, not proof a path exists; compact: near_sink/ns readings)"
else
    no "(S12) a legend lacks the near_sink clause (full: $( grep -c 'SINK-SIDE' "$TMP/Sfull.xml" ), compact: $( grep -c 'ns in= site=' "$f" ))"
fi

echo "(C) closure containment: the callees form's <in> blocks and the near-sink why=closure (lane ix2-slice, IX-2 P2)"
mkdir -p "$TMP/clo" "$TMP/clopy" "$TMP/clokoa"
# nunjucks shape: createTemplate is an arrow written in getTemplate's body; only it calls Template
cat > "$TMP/clo/env.js" <<'EOF'
class Template {
  constructor(src) { this.src = src; }
}
function iter(items, fn, done) { return done(null, items); }
function plain(x) { return iter([], x, x); }
class Env {
  getTemplate(name, cb) {
    const createTemplate = (err, info) => {
      return new Template(info);
    };
    iter([], cb, createTemplate);
    return name;
  }
}
module.exports = { Env, plain };
EOF
# Python: a nested def (a closure, at depth 2 too) and a method of a class inside a function (a member, not a closure)
cat > "$TMP/clopy/m.py" <<'EOF'
def target():
    return 1

def other():
    return 2

def outer():
    def helper():
        def deeper():
            return other()
        return target()
    return helper

def factory():
    class K:
        def m(self):
            return target()
    return K
EOF
# koa shape: handleResponse is a closure of handleRequest and the sink's only caller
cat > "$TMP/clokoa/app.js" <<'EOF'
function respond(ctx) { return ctx; }
class App {
  handleRequest(ctx) {
    const handleResponse = () => respond(ctx);
    return 0;
  }
  listen() { return 1; }
}
module.exports = { App };
EOF
cal(){ "$BIN" "$1" --no-cache --callees="$2" > "$3" 2>&1; }
f="$TMP/C1.xml"; cal "$TMP/clo" getTemplate "$f"; rc=$?
blk="$( grep -o '<in [^>]*>.*</in>' "$f" )"
if [ "$rc" -eq 0 ] && grep -q '<callees [^>]*closures="1"' "$f" \
   && [ "$blk" = '<in n="createTemplate" p="env.js:8" via="closure" count="1"><s t="cls" n="Template" p="env.js:1"/></in>' ] \
   && ! grep -o '<callees .*<in ' "$f" | grep -q '<s [^>]*n="Template"[^>]*/><in' ; then
    ok "(C1) --callees=getTemplate carries closures=\"1\" and the createTemplate block naming Template (nunjucks shape)"
else
    no "(C1) want closures=1 and the createTemplate→Template block; got rc=$rc [$blk]: $( grep -o '<callees .*' "$f" | head -c 600 )"
fi
cnt="$( grep -o '<callees [^>]*count="[0-9]*"' "$f" | sed 's/.*count="\([0-9]*\)"/\1/' )"
if [ "$cnt" = "1" ] && ! grep -o '<callees [^>]*>.*<in ' "$f" | grep -q 'n="Template"'; then
    ok "(C2) count=\"1\" (iter only): the block's rows are in no count and in no row above it"
else
    no "(C2) count=[$cnt], or Template leaked into the call rows: $( grep -o '<callees .*' "$f" | head -c 400 )"
fi
f="$TMP/C3.xml"; cal "$TMP/clopy" outer "$f"; rc=$?
blk="$( grep -o '<in .*</in>' "$f" | tr -d '\n' )"
if [ "$rc" -eq 0 ] && grep -q '<callees [^>]*closures="2"' "$f" \
   && [ "$blk" = '<in n="helper" p="m.py:8" via="closure" count="1"><s t="fn" n="target" p="m.py:1"/></in><in n="deeper" p="m.py:9" via="closure" count="1"><s t="fn" n="other" p="m.py:4"/></in>' ]; then
    ok "(C3) Python nested defs, any depth: outer's blocks are helper (→ target) and deeper (→ other), source order"
else
    no "(C3) want helper and deeper blocks; got rc=$rc [$blk]"
fi
f="$TMP/C4.xml"; cal "$TMP/clopy" factory "$f"; rc=$?
if [ "$rc" -eq 0 ] && grep -q '<callees ' "$f" && ! grep -q 'closures=\|<in ' "$f"; then
    ok "(C4) near miss: a method of a class written inside factory is the class's member, not a closure (no block)"
else
    no "(C4) factory's class member read as a closure: $( grep -o '<callees .*' "$f" | head -c 400 )"
fi
f="$TMP/C5.xml"; cal "$TMP/clo" plain "$f"; rc=$?
if [ "$rc" -eq 0 ] && grep -q '<callees [^>]*count="1"' "$f" && ! grep -q 'closures=\|<in \|closure' "$f"; then
    ok "(C5) a function with no closure carries no closures=, no <in> and no closure legend"
else
    no "(C5) closure vocabulary on a closure-free answer: $( head -c 400 "$f" )"
fi
"$BIN" "$TMP/clo" --no-cache --callees=getTemplate --json > "$TMP/C6.json" 2>&1; rc=$?
if [ "$rc" -eq 0 ] && python3 -c 'import json,sys; d=json.load(open(sys.argv[1])); b=d["in"]; sys.exit(0 if d["closures"]==1 and len(b)==1 and b[0]["n"]=="createTemplate" and b[0]["via"]=="closure" and b[0]["count"]==1 and b[0]["calls"][0]["n"]=="Template" else 1)' "$TMP/C6.json"; then
    ok "(C6) --json carries \"closures\":1 and the same block (keys = the XML attribute names)"
else
    no "(C6) --json lacks the closure block (rc=$rc): $( head -c 400 "$TMP/C6.json" )"
fi
"$BIN" "$TMP/clo" --no-cache --callees=getTemplate --legend=full > "$TMP/C7.xml" 2>&1
if grep -q 'closures=K: K named functions are written INSIDE' "$TMP/C7.xml" && grep -q 'containment, not a call edge' "$TMP/C7.xml" \
   && grep -q "closures=K: named functions written inside of=" "$TMP/C1.xml" && grep -q 'in n= p= via=closure count=' "$TMP/C1.xml"; then
    ok "(C7) both legends define closures= and the <in> block (full: containment, not a call edge)"
else
    no "(C7) a legend lacks the closure reading"
fi
nsArm "(C8) near_sink why=closure: the sink's only caller is a closure of handleRequest (koa shape)" "$TMP/clokoa" "listen,respond" \
    '<near_sink depth="2" callers="1" rows="1" next="--callers=app.js:handleResponse">' \
    '<ns d="1" t="fn" n="handleResponse" p="app.js:4" why="closure:1" in="handleRequest" site="app.js:4"/>'

echo "(B) hedged bridge <bridge proven=\"0\"> and (G) glob endpoints (lane ix2-slice, IX-2 P1/P4)"
mkdir -p "$TMP/brkoa" "$TMP/brdeep" "$TMP/glob"
cat > "$TMP/brkoa/app.js" <<'EOF'
function respond(ctx) { return ctx; }
class App {
  listen() { return this.callback(); }
  callback() {
    const handler = (req) => { return this.handleRequest(req); };
    return handler;
  }
  handleRequest(ctx) {
    const handleResponse = () => respond(ctx);
    return handleResponse;
  }
}
module.exports = { App };
EOF
# three closure crossings needed: past the bound, so no bridge
cat > "$TMP/brdeep/m.py" <<'EOF'
def target():
    return 1

def outer():
    def m1():
        def m2():
            def m3():
                return target()
            return m3
        return m2
    return m1
EOF
# fan-out: a declined scan() with 17 candidates (past the bound) and, in wide16, 16 (inside it)
for n in 16 17; do
    mkdir -p "$TMP/fan$n/core"
    printf 'void scan( void );\nvoid start( void ) { scan(); }\n' > "$TMP/fan$n/core/m.c"
    i=1; while [ "$i" -le "$n" ]; do
        mkdir -p "$TMP/fan$n/p$i"
        if [ "$i" -eq 1 ]; then printf 'void sink( void ) { }\nvoid scan( void ) { sink(); }\n' > "$TMP/fan$n/p$i/s.c"; else printf 'void scan( void ) { }\n' > "$TMP/fan$n/p$i/s.c"; fi
        i=$(( i + 1 ))
    done
done
cat > "$TMP/glob/h.py" <<'EOF'
def on_click():
    return 1

def on_key():
    return 2

def dispatch():
    return on_key()

def post():
    return dispatch()
EOF
brblock(){ grep -o '<bridge [^>]*>.*</bridge>' "$1"; }
brArm(){   # $1 label, $2 dir, $3 FROM,TO, $4 expected <bridge>…</bridge> block exactly ("" = none)
    local f="$TMP/B$( echo "$1" | tr -cd 'A-Za-z0-9' ).xml" rc
    path "$2" "$3" "$f"; rc=$?
    premise "$1" "$rc" "$f" || return
    local b; b="$( brblock "$f" )"
    if [ "$( attr "$f" reachable )" = "0" ] && [ "$( attr "$f" hops )" = "0" ] && [ "$b" = "$4" ]; then
        ok "$1 ${b:-<no bridge>}"
    else
        no "$1 want [$4]; got reachable=$( attr "$f" reachable ) [$b]"
    fi
}
brArm "(B1) declined crossing to the platform build that reaches the sink (htop shape)" "$TMP/decl" "Machine_run,Process_update" \
    '<bridge proven="0" crossings="1" hops="2"><s t="fn" n="Machine_run" p="core/machine.c:2"/><s t="fn" n="Machine_scan" p="linux/scan.c:2" via="declined" cands="2"/><s t="fn" n="Process_update" p="linux/scan.c:1"/></bridge>'
brArm "(B2) two closure crossings: listen → callback ⊃ handler → handleRequest ⊃ handleResponse → respond (koa shape)" "$TMP/brkoa" "listen,respond" \
    '<bridge proven="0" crossings="2" hops="5"><s t="method" n="listen" p="app.js:3"/><s t="method" n="callback" p="app.js:4"/><s t="fn" n="handler" p="app.js:5" via="closure" cands="1"/><s t="method" n="handleRequest" p="app.js:8"/><s t="fn" n="handleResponse" p="app.js:9" via="closure" cands="1"/><s t="fn" n="respond" p="app.js:1"/></bridge>'
brArm "(B3) value crossing: the callback handed to .then()" "$TMP/val" "listen,respond" \
    '<bridge proven="0" crossings="1" hops="3"><s t="fn" n="listen" p="app.js:5"/><s t="fn" n="handleRequest" p="app.js:4"/><s t="fn" n="handleResponse" p="app.js:2" via="value" cands="1"/><s t="fn" n="respond" p="app.js:1"/></bridge>'
brArm "(B4) namesake crossing: run's by-name obj.process() to the unsearched B.process (B is in reach: K5)" "$TMP/k_process_in" "run,target" \
    '<bridge proven="0" crossings="1" hops="2"><s t="fn" n="run" p="pkg/x/c.py:3"/><s t="fn" n="process" p="other/b.py:9" via="namesake" cands="1"/><s t="fn" n="target" p="other/b.py:1"/></bridge>'
brArm "(B5) three crossings needed: past the bound, no bridge" "$TMP/brdeep" "outer,target" ""
brArm "(B6) a declined call with 17 candidates is not crossed (fan-out bound 16)" "$TMP/fan17" "start,sink" ""
brArm "(B7) the same shape with 16 candidates is crossed" "$TMP/fan16" "start,sink" \
    '<bridge proven="0" crossings="1" hops="2"><s t="fn" n="start" p="core/m.c:2"/><s t="fn" n="scan" p="p1/s.c:2" via="declined" cands="16"/><s t="fn" n="sink" p="p1/s.c:1"/></bridge>'
brArm "(B8) a disconnected pair has no bridge (no crossing to take)" "$TMP/clean" "a,d" ""
if [ -s "$TMP/found.xml" ] && ! grep -q '<bridge' "$TMP/found.xml"; then
    ok "(B9) a found path (N4) carries no <bridge>"
else
    no "(B9) a found path carries a bridge or did not run"
fi
"$BIN" "$TMP/decl" --no-cache --path=Machine_run,Process_update --legend=full > "$TMP/Bfull.xml" 2>&1
if grep -q 'a HEDGED chain from= to to= of at most 8 hops that crosses at most 2' "$TMP/Bfull.xml" && grep -q 'It is NOT a path' "$TMP/Bfull.xml" \
   && grep -q 'bridge proven=0 crossings= hops=: a HEDGED chain, NOT a path' "$TMP/BB1declinedcrossingtotheplatformbuildthatreachesthesinkhtopshape.xml"; then
    ok "(B10) both legends define <bridge> and say it is not a path"
else
    no "(B10) a legend lacks the bridge clause"
fi
f="$TMP/G1.xml"; path "$TMP/glob" 'post,on_*' "$f"; rc=$?
if [ "$rc" -eq 0 ] && [ "$( attr "$f" to_glob )" = "1" ] && [ "$( attr "$f" to_defs )" = "2" ] && [ "$( attr "$f" reachable )" = "1" ] \
   && [ "$( grep -o '<s [^>]*/>' "$f" | sed 's/.* n="\([^"]*\)".*/\1/' | tr '\n' ' ' )" = "post dispatch on_key " ] && grep -q 'to_glob=1' "$f"; then
    ok "(G1) --path=post,on_* (textual shape): rc=0, to_glob=\"1\" to_defs=\"2\", the path post → dispatch → on_key"
else
    no "(G1) glob endpoint: rc=$rc $( grep -o '<path .*' "$f" | head -c 500 )"
fi
f="$TMP/G2.xml"; "$BIN" "$TMP/glob" --no-cache '--path=post,zz_*' > "$f" 2>&1; rc=$?
if [ "$rc" -ne 0 ] && ! grep -q 'to_glob' "$f"; then
    ok "(G2) a pattern matching no definition is still refused (rc=$rc)"
else
    no "(G2) zz_* answered rc=$rc: $( head -c 300 "$f" )"
fi
f="$TMP/G3.xml"; path "$TMP/glob" 'post,dispatch' "$f"; rc=$?
if [ "$rc" -eq 0 ] && ! grep -q '_glob' "$f"; then
    ok "(G3) a resolvable endpoint carries no *_glob attribute"
else
    no "(G3) a plain endpoint carries a glob attribute: $( grep -o '<path [^>]*>' "$f" )"
fi

echo "(H) hedge-quality fixes found by the acceptance hand-check (lane ix2-slice)"
mkdir -p "$TMP/twin" "$TMP/cnest" "$TMP/brtest/core" "$TMP/brtest/linux" "$TMP/brtest/tests" "$TMP/brlong"
cat > "$TMP/twin/f.js" <<'EOF'
function helper(u) { return u; }
function build() {
  const fastify = {
    post: function _post (url) {
      return helper(url)
    },
  }
  return fastify
}
module.exports = build
EOF
cat > "$TMP/cnest/n.c" <<'EOF'
int target( void ) { return 1; }
int outer( void ) { int inner( void ) { return target(); } return inner(); }
EOF
printf 'void scan( void );\nvoid start( void ) { scan(); }\n' > "$TMP/brtest/core/m.c"
printf 'void scan( void ) { }\n' > "$TMP/brtest/linux/s.c"
printf 'void sink( void ) { }\nvoid scan( void ) { sink(); }\n' > "$TMP/brtest/tests/s.c"
{ echo 'def target():'; echo '    return 0'; echo; echo 'def h9():'; echo '    return target()'; echo
  i=8; while [ "$i" -ge 1 ]; do echo "def h$i():"; echo "    return h$(( i + 1 ))()"; echo; i=$(( i - 1 )); done
  echo 'def start():'; echo '    def inner():'; echo '        return h1()'; echo '    return 0'; } > "$TMP/brlong/m.py"
f="$TMP/H1.xml"; cal "$TMP/twin" build "$f"; rc=$?
if [ "$rc" -eq 0 ] && grep -q '<callees [^>]*closures="1"' "$f" \
   && [ "$( grep -o '<in .*</in>' "$f" )" = '<in n="_post" p="f.js:4" via="closure" count="1"><s t="fn" n="helper" p="f.js:1"/></in>' ]; then
    ok "(H1) \`post: function _post\` is ONE definition: build's one closure is _post (its calls), never the property twin as well"
else
    no "(H1) the property twin read as a closure: $( grep -o '<callees .*' "$f" | head -c 400 )"
fi
f="$TMP/H2.xml"; cal "$TMP/cnest" outer "$f"; rc=$?
if [ "$rc" -eq 0 ] && grep -q '<callees ' "$f" && ! grep -q 'closures=\|<in ' "$f"; then
    ok "(H2) C has no named nested function (langfacts noNestedNamedFunctions): a function span inside one is never a closure"
else
    no "(H2) a C nested span read as a closure: $( grep -o '<callees .*' "$f" | head -c 400 )"
fi
brArm "(H3) a bridge never routes through a test file (the only scan that reaches sink is under tests/)" "$TMP/brtest" "start,sink" ""
brArm "(H4) a chain longer than 8 hops is no bridge (start ⊃ inner → h1 … h9 → target: 11 hops)" "$TMP/brlong" "start,target" ""

for nm in join combine; do
    mkdir -p "$TMP/brb_$nm/core" "$TMP/brb_$nm/a" "$TMP/brb_$nm/b"
    printf 'function start(ids) { return ids.%s(","); }\nmodule.exports = { start };\n' "$nm" > "$TMP/brb_$nm/core/m.js"
    printf 'function sink() { return 0; }\nfunction %s(arr) { return sink(); }\nmodule.exports = { %s, sink };\n' "$nm" "$nm" > "$TMP/brb_$nm/a/x.js"
    printf 'function %s(arr) { return 1; }\nmodule.exports = { %s };\n' "$nm" "$nm" > "$TMP/brb_$nm/b/y.js"
done
brArm "(H5) a declined member call spelled like a builtin method (ids.join) is not crossed" "$TMP/brb_join" "start,sink" ""
brArm "(H6) the same shape under a non-builtin name (ids.combine) is crossed" "$TMP/brb_combine" "start,sink" \
    '<bridge proven="0" crossings="1" hops="2"><s t="fn" n="start" p="core/m.js:1"/><s t="fn" n="combine" p="a/x.js:2" via="declined" cands="2"/><s t="fn" n="sink" p="a/x.js:1"/></bridge>'

mkdir -p "$TMP/nsprop"
cat > "$TMP/nsprop/m.py" <<'EOF'
def target():
    return 1

class C:
    @property
    def size(self):
        return target()

def start():
    return 0
EOF
f="$TMP/H7.xml"; path "$TMP/brb_join" "start,sink" "$f"; rc=$?
if premise "(H7)" "$rc" "$f"; then
    if nsrows "$f" | grep -q '<ns d="1" t="fn" n="join" p="a/x.js:2"' && ! nsrows "$f" | grep -q 'n="join"[^>]*declined'; then
        ok "(H7) near_sink: a declined ids.join (builtin-spelled) is no way in to the in-tree join"
    else
        no "(H7) near_sink still reads ids.join as a way in: $( nsrows "$f" | tr -d '\n' )"
    fi
fi
f="$TMP/H8.xml"; path "$TMP/nsprop" "start,target" "$f"; rc=$?
if premise "(H8)" "$rc" "$f"; then
    r="$( nsrows "$f" | tr -d '\n' )"
    if echo "$r" | grep -q 'n="size"' && ! echo "$r" | grep -q 'why="value'; then
        ok "(H8) near_sink: @property is a wrapper decorator, not a value hand-off ($r)"
    else
        no "(H8) a wrapper decorator read as a value way in: $r"
    fi
fi

# K5 (card K5): the by-name hedges' two rules. Each arm first proves its premise — the call really is bound by name alone
# (gaps="name:1" on the root) — so an arm cannot pass because the resolver stopped making the hedged edge.
k5premise(){   # $1 label, $2 answer file
    if [ "$( attr "$2" gaps )" != "name:1" ]; then
        no "$1 premise: the by-name edge is gone (gaps=$( attr "$2" gaps )), the arm would assert nothing"; return 1
    fi
}
for arm in "K1|process_out|owner" "K2|update_in|builtin" "K3|update_out|both"; do
    IFS='|' read -r id fx why <<< "$arm"
    f="$TMP/$id.xml"; path "$TMP/k_$fx" "run,target" "$f"; rc=$?
    premise "($id)" "$rc" "$f" || continue
    k5premise "($id)" "$f" || continue
    r="$( nsrows "$f" | tr -d '\n' )"; b="$( brblock "$f" )"
    if echo "$r" | grep -q 'n="update"\|n="process"' && ! echo "$r" | grep -q 'why="[^"]*name' && [ -z "$b" ]; then
        ok "($id) k_$fx ($why): no near_sink why=name and no namesake bridge — $r"
    else
        no "($id) k_$fx ($why): a by-name hedge survived: rows [$r] bridge [$b]"
    fi
done
f="$TMP/K4.xml"; path "$TMP/k_process_in" "run,target" "$f"; rc=$?
if premise "(K4)" "$rc" "$f" && k5premise "(K4)" "$f"; then
    if nsrows "$f" | grep -q 'n="process"[^>]*why="name:1"' && brblock "$f" | grep -q 'via="namesake"'; then
        ok "(K4) near miss: the namesake's owner B is in reach (run → helper → B.make) and process is not builtin-spelled: both hedges stay"
    else
        no "(K4) the in-reach, non-builtin by-name hedge was dropped: $( nsrows "$f" | tr -d '\n' ) $( brblock "$f" )"
    fi
fi
# K5x (review M1): a by-name way in the K5 rules WITHHOLD is not "no indexed way in was found" — the row says withheld="name:N"
# and never why="external". Near miss: S5's z has no by-name hit at all and keeps why="external" (pinned exactly above).
f="$TMP/K1.xml"
if [ -s "$f" ] && grep -q '<path ' "$f"; then
    r="$( nsrows "$f" | grep 'n="process"' | tr -d '\n' )"
    if echo "$r" | grep -q 'withheld="name:1"' && ! echo "$r" | grep -q 'why="external"'; then
        ok "(K5x) k_process_out: the withheld by-name way in is marked, not read as external — $r"
    else
        no "(K5x) k_process_out: a withheld by-name way in reads as external or carries no withheld mark: [$r]"
    fi
else
    no "(K5x) premise: K1's answer is missing"
fi

echo "(M) MCP path_between carries the same clause"
# Two MCP legend postures (lean-answers): the unchanged assertions run on --mcp-legend=inline, the pre-posture spelling
# where every answer carries its root facts on <path>; a TWIN runs the default session posture, whose answers after the
# first are legend="ref" and carry their root facts on the closing <about …/> instead. Both must match the CLI.
for MPOST in inline session; do
python3 - "$BIN" "$TMP" "$MPOST" <<'PY'
import json, re, subprocess, sys
binp, tmp, posture = sys.argv[1], sys.argv[2], sys.argv[3]
cases = [ ( "decl", "Machine_run", "Process_update", True ), ( "val", "listen", "respond", True ),
          ( "thr", "start", "target", True ), ( "lpy", "run", "target", True ), ( "clean", "a", "d", False ),
          ( "glob", "on_key", "on_c*", False ) ]
reqs = [ { "jsonrpc": "2.0", "id": 0, "method": "initialize", "params": { "protocolVersion": "2024-11-05", "capabilities": {}, "clientInfo": { "name": "g", "version": "1" } } } ]
for i, ( d, a, b, _ ) in enumerate( cases ):
    reqs.append( { "jsonrpc": "2.0", "id": i + 1, "method": "tools/call", "params": { "name": "path_between", "arguments": { "path": tmp + "/" + d, "from": a, "to": b } } } )
argv = [ binp, "--mcp" ] + ( [ "--mcp-legend=inline" ] if posture == "inline" else [] )
p = subprocess.run( argv, input = "".join( json.dumps( r ) + "\n" for r in reqs ), capture_output = True, text = True, timeout = 120 )
refs = 0
resp = {}
for line in p.stdout.splitlines():
    try:
        o = json.loads( line )
    except ValueError:
        continue
    if isinstance( o.get( "id" ), int ) and o[ "id" ] > 0:
        resp[ o[ "id" ] ] = "".join( c.get( "text", "" ) for c in o.get( "result", {} ).get( "content", [] ) )
def head( t ):
    m = re.search( r'<path [^>]*>', t )
    return m.group( 0 ) if m else ""
def facts( t ):   # the root's facts: <path …>, plus a legend="ref" answer's closing <about …/> (where they move)
    ab = re.search( r'<about [^>]*legend="ref"[^>]*/>', t )
    return head( t ) + ( ab.group( 0 ) if ab else "" )
def pick( t, name ):
    m = re.search( r'\s' + name + r'="([^"]*)"', facts( t ) )
    return m.group( 1 ) if m else None
bad = 0
for i, ( d, a, b, gap ) in enumerate( cases ):
    mcp = resp.get( i + 1, "" )
    refs += 1 if re.search( r'<about [^>]*legend="ref"', mcp ) else 0
    cli = subprocess.run( [ binp, tmp + "/" + d, "--no-cache", "--path=%s,%s" % ( a, b ) ], capture_output = True, text = True ).stdout
    if not head( mcp ):
        print( "  MCP gave no <path> root for", d, repr( mcp[:300] ), p.stderr[:300] ); bad = 1; continue
    for name in ( "searched", "gaps", "gap_syms", "gap_syms_capped", "next", "reachable", "to_glob", "to_defs" ):
        if pick( mcp, name ) != pick( cli, name ):
            print( "  %s: %s= MCP %r vs CLI %r" % ( d, name, pick( mcp, name ), pick( cli, name ) ) ); bad = 1
    if re.findall( r'<gap [^>]*/>', mcp ) != re.findall( r'<gap [^>]*/>', cli ):
        print( "  %s: gap rows differ" % d ); bad = 1
    # ix2-slice: the sink-side frontier block, byte for byte (one emitter, two transports)
    nsm, nsc = re.findall( r'<near_sink .*?</near_sink>', mcp ), re.findall( r'<near_sink .*?</near_sink>', cli )
    if nsm != nsc or len( nsc ) != 1:
        print( "  %s: near_sink differs or is missing: MCP %r vs CLI %r" % ( d, nsm, nsc ) ); bad = 1
    if re.findall( r'<bridge .*?</bridge>', mcp ) != re.findall( r'<bridge .*?</bridge>', cli ):
        print( "  %s: bridge differs: MCP vs CLI" % d ); bad = 1
    if gap and ( "no directed call path" in mcp or "search is incomplete" not in mcp ):
        print( "  %s: MCP hint still claims no path" % d ); bad = 1
    if not gap and "no directed call path" not in mcp:
        print( "  %s: MCP lost the plain no-path hint" % d ); bad = 1
# the posture premise: inline answers never take the ref posture; the session twin's answers after the first all do
if posture == "inline" and refs != 0:
    print( "  inline posture: %d answer(s) came back legend=\"ref\"" % refs ); bad = 1
if posture == "session" and refs != len( cases ) - 1:
    print( "  session posture: %d of the %d later answers are legend=\"ref\" (the twin tests nothing new)" % ( refs, len( cases ) - 1 ) ); bad = 1
sys.exit( bad )
PY
mrc=$?
if [ "$mrc" -eq 0 ]; then
    ok "(M) path_between ($MPOST legend posture): same searched/gaps/gap_syms/rows/next and <near_sink> as the CLI on P1-P3 and P7, plain hint on N1"
else
    no "(M) MCP path_between ($MPOST legend posture) disagrees with the CLI or the helper crashed (rc=$mrc, details above)"
fi
done

if [ "$fail" -eq 0 ]; then echo "ALL PASS"; else echo "FAIL"; fi
exit "$fail"
