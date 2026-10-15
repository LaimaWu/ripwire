#!/usr/bin/env bash
# forbodyslotcheck.sh — the ONE owner body of an explanatory --for question (the "body slot"), on top of the
# stop-when-complete budget semantics (lane/for-body-slot, train 28; design D of lane/for-noise-body-slot
# re-expressed: the body is a SECTION of a complete how-does answer, never paid by dropping sig rows).
#
# THE CONTRACT THIS PINS (arms tagged [P] were RED on the pre-change binary 974dd5b1; [G] arms are twins and
# near-miss negatives that hold on both binaries and must keep holding — PROCESS rule 4, no gate erosion):
#
#   TRIGGER  CLI --for (XML, full and compact legends) and MCP `for`, on the compact posture of the
#            subtoken+body route (root bundle="compact"), when the task carries an explanatory cue
#            (taskroute.h kExplanatoryCues, word-bounded: how does/do/is/are, what does, implementation of).
#            No slot: no cue, --signatures-only, --json (the JSON dialect carries no bodies in any posture —
#            a named floor), --auto-bodies and --detail=N (the caller chose the bodies), --format=candidates,
#            the name-exact route (the auto bundle already serves the anchor's body).
#   OWNER    owner-hop's predicate (verbs_for.h forOwnerHopPlan): a callable definition (Function/Method,
#            isDefinitionNotDeclaration, so never a .d.ts / header declaration), not a test symbol, inside the
#            ranked head, whose whole folded name equals a question word; common words after the rest. Among
#            owners the one with a call edge (resolved or by name) to another owner wins (the entry point),
#            else the best rank. No owner: the best-ranked callable of the same predicate whose name shares a
#            kept question subtoken. None: no slot. Classes/structs/types are never the owner.
#   RENDER   one <bodies><b t= l= p= n=> in the --expand shape (packBodies), <calls> kept. A body over
#            kForBodySlotMaxBytes (6000) is head-cut at a line end: truncated="1" lines="lo-hi/T"
#            next="--expand=P:L:N:A-B" (the cutfix-bodies vocabulary), bodies capped="1"; a FIRST line longer
#            than the cap is served whole with over_ceiling="1" (packBodies' existing rule). Root bodies="1".
#   BUDGETS  default (no explicit budget): the body is a required section of the complete answer. <sigs>,
#            <hops>, <tail> are byte-identical to the slot-off answer (RIPWIRE_NO_BODY_SLOT=1, the A/B handle,
#            as RIPWIRE_NO_OWNER_HOP); est_tokens prices the body; the runaway guard (30,000 B) is unchanged
#            and never fires because of the body. Explicit --token-budget=N / MCP budget_tokens:
#            reserve-sections-first — the body is reserved with the trailing sections at the cut, rows past
#            the cut fill what is left; rows stay a rank prefix, monotone in N, never more than slot-off;
#            the body never shrinks as N grows and equals the default body at a wide N; when its share cannot
#            hold it the slot is skipped with bodies="0" reason="budget" and the owner stays recoverable
#            through a next= naming --expand of it.
#   MCP      `for` serves the same body (parity decision, design D open question 4); budget_tokens as above.
#
# Arms (colour on 974dd5b1 in brackets):
#   D1 [P] T_D: bodies=1, <b n=jwt> holds getCookie / HTTPException(401 / ctx.set('jwtPayload'; <calls> with verify
#   D2 [G] T_D: not verify's body (jwt calls verify: the entry point)
#   D3 [P] est_tokens prices the body (slot vs RIPWIRE_NO_BODY_SLOT=1)
#   D4 [G] sigs/hops/tail byte-identical slot vs slot-off on every fixture question (checklist 23); slot-off has no body
#   D5 [P] legends: full answer explains bodies=1; compact legend defines b t= n= p= l= when a body rides
#   D6 [G] no explanatory cue (where / show documentation / bare noun phrase): bodies=0, no <b
#   D7 [G] postures: --signatures-only, --json, --auto-bodies, --detail=1, --format=candidates, name-exact unchanged
#   D8 [P] Python: body of the METHOD apply, never the class Stylesheet (r1)
#   D9 [P] C: body of cmd_find_target (identifier word); D9b [P] subtoken fallback via "target"
#   D10 [P] oversized C owner: head-cut at a line end, lines= next=; the next= serves the rest; no over_ceiling root
#   D11 [P] one 7.7 KB line of multibyte text: served whole, over_ceiling="1" on the <b>, valid UTF-8
#   L1 [P] Rust: entry point index (calls walk) beats walk at r1     L2 [P] C++: Builder::Build DEFINITION (.cc), not the r1 header declaration
#   L3 [P] Go: method Package, never the type Packager
#   N1 [P] TS: negotiate's definition, not the r1 .d.ts declaration; [G] a .d.ts-only name never yields a .d.ts body
#   N2 [P/G] the only name owner is a tests/ helper: the slot falls back to a non-test callable, never tests/
#   N3 [P] two equal owners in two files: ONE body, the better-ranked one (p= and l= of the r1 row)
#   N4 [P] two same-named methods of two classes in ONE file: ONE body, the r1 row's (entity identity, checklist 16)
#   N5 [G] a struct named by the question is never the body; every <b t=> is fn/method
#   E1 [P] --token-budget=3000: the body rides; rows a rank prefix of the default rows, >= 4, no over_ceiling
#   E2 [P] --token-budget=400: bodies=0 reason=budget; the skipped slot discloses itself on <bodies … next=--expand=…jwt.ts:…>
#   E3 [G/P] N sweep: rows monotone + rank prefix + <= slot-off; body bytes monotone; [P] at 1000000 body == default body
#   MC1-MC4 MCP for: [P] body parity with the CLI; [P] budget_tokens=400 recoverable; [G] no cue, no body
#   M1-M4 mutation controls (canned pre-change shapes are rejected)   X1-X3 determinism, xmllint, no '--' in comments
#   I0-I9 [optional, RIPWIRE_BASE_BIN=<pre-change binary>] byte identity where the slot must not fire (I0: slot off == base)
#
# Usage:  bash test/forbodyslotcheck.sh [BIN]   |   RIPWIRE_BIN=asan/ripwire bash test/forbodyslotcheck.sh
# BIN is $1, else RIPWIRE_BIN, else build/ripwire; the first output line names the binary used.
# Exits non-zero on any failure.

set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
. "$ROOT/test/lib/clean-env.sh"
unset RIPWIRE_NO_BODY_SLOT RIPWIRE_NO_OWNER_HOP
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
echo "forbodyslotcheck: BIN=$BIN"
fail=0
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
skip(){ printf '  SKIP  %s\n' "$*"; return 0; }

[ -x "$BIN" ] || { echo "no ripwire binary at $BIN — build first"; exit 2; }
command -v python3 >/dev/null 2>&1 || { echo "python3 required"; exit 2; }
cd "$ROOT"
FIX="$ROOT/test/forbodyslotfix"
[ -d "$FIX/src" ] || { echo "fixture $FIX missing"; exit 2; }

TMP="$( mktemp -d )"; trap 'rm -rf "$TMP"' EXIT

# ── the checker: one Python reader of a --for answer, one verdict per call (exit 0 = holds) ─────────────────
cat > "$TMP/chk.py" <<'PY'
import html, json, os, re, sys
cmd, args = sys.argv[1], sys.argv[2:]
def load(p):
    return open(p, "rb").read().decode("utf-8", "replace")
def attrs(s):
    return dict((k, html.unescape(v)) for k, v in re.findall(r'(\w+)="([^"]*)"', s))
def root(t):
    m = re.search(r'<ctx ([^>]*)>', t)
    return attrs(m.group(1)) if m else {}
def sigs(t):
    m = re.search(r'<sigs\b([^>]*)>', t)
    return attrs(m.group(1)) if m else {}
def rows(t):
    m = re.search(r'<sigs\b[^>]*>(.*?)</sigs>', t, re.S)
    out = []
    for d in re.finditer(r'<d ([^>]*)>', m.group(1) if m else ""):
        a = attrs(d.group(1))
        out.append((int(a.get("r", "0") or 0), a.get("n", ""), a.get("p", ""), a))
    return sorted(out, key=lambda x: x[0])
def bodies(t):
    """every <b …><![CDATA[…]]>…</b>: (attrs, cdata bytes, the whole element)"""
    out = []
    for m in re.finditer(r'<b ([^>]*)>(.*?)</b>', t, re.S):
        cd = "".join(re.findall(r'<!\[CDATA\[(.*?)\]\]>', m.group(2), re.S))
        out.append((attrs(m.group(1)), cd, m.group(0)))
    return out
def legend(t):
    return " ".join(re.findall(r'<!--(.*?)-->', t, re.S))
def fail(msg):
    print(msg); sys.exit(1)
def first_row(t, name):
    for r, n, p, a in rows(t):
        if n == name:
            return a
    return None
def strip_slot(t):
    """the answer minus everything the body slot may add: legend comments, the <bodies> element, root bodies=/reason=/est_tokens="""
    t = re.sub(r'<!--.*?-->', '', t, flags=re.S)
    t = re.sub(r'<bodies\b.*?</bodies>', '', t, flags=re.S)
    t = re.sub(r'<bodies\b[^>]*/>', '', t)
    t = re.sub(r' (bodies|reason|est_tokens|bundle)="[^"]*"', '', t)
    return t
if cmd == "rootattr":                   # rootattr FILE ATTR REGEX
    t = load(args[0]); v = root(t).get(args[1])
    if v is None or not re.fullmatch(args[2], v): fail("root %s=%r, wanted %s" % (args[1], v, args[2]))
    print("%s=%s" % (args[1], v))
elif cmd == "noroot":                   # noroot FILE ATTR : the root carries no ATTR=
    t = load(args[0]); v = root(t).get(args[1])
    if v is not None: fail("root carries %s=%r" % (args[1], v))
elif cmd == "bodycount":                # bodycount FILE N : exactly N <b> and root bodies=N
    t = load(args[0]); n = len(bodies(t)); v = root(t).get("bodies")
    if n != int(args[1]) or v != args[1]: fail("<b> count=%d root bodies=%r, wanted %s" % (n, v, args[1]))
    print("bodies=%s" % v)
elif cmd == "nobody":                   # nobody FILE : no <bodies and no <b
    t = load(args[0])
    if "<bodies" in t or re.search(r'<b ', t): fail("a bodies element rides: %s" % [b[0].get("n") for b in bodies(t)])
elif cmd == "body":                     # body FILE NAME PATH STR... : one <b n=NAME p=PATH> whose CDATA holds every STR
    t = load(args[0]); bs = bodies(t)
    hit = [b for b in bs if b[0].get("n") == args[1] and b[0].get("p") == args[2]]
    if not hit: fail("no <b n=%s p=%s> (bodies: %s)" % (args[1], args[2], [(b[0].get("n"), b[0].get("p")) for b in bs]))
    a, cd, el = hit[0]
    miss = [s for s in args[3:] if s not in cd]
    if miss: fail("body %s lacks %s" % (args[1], miss))
    print("<b n=%s p=%s l=%s> %d B" % (args[1], args[2], a.get("l"), len(cd.encode())))
elif cmd == "nobodyof":                 # nobodyof FILE NAME : no <b n=NAME>
    t = load(args[0])
    if any(b[0].get("n") == args[1] for b in bodies(t)): fail("a <b n=%s> rides" % args[1])
elif cmd == "nobodyat":                 # nobodyat FILE REGEX : no <b p=~REGEX>
    t = load(args[0]); bad = [b[0].get("p") for b in bodies(t) if re.search(args[1], b[0].get("p", ""))]
    if bad: fail("<b p=> matched %s: %s" % (args[1], bad))
elif cmd == "callsin":                  # callsin FILE NAME CALLEE : the <b n=NAME> carries <calls> with <c n=CALLEE>
    t = load(args[0]); hit = [b for b in bodies(t) if b[0].get("n") == args[1]]
    if not hit: fail("no <b n=%s>" % args[1])
    el = hit[0][2]
    if "<calls" not in el or ('<c n="%s"' % args[2]) not in el: fail("<b n=%s> has no <calls> naming %s" % (args[1], args[2]))
elif cmd == "bodyattr":                 # bodyattr FILE NAME ATTR REGEX
    t = load(args[0]); hit = [b for b in bodies(t) if b[0].get("n") == args[1]]
    if not hit: fail("no <b n=%s>" % args[1])
    v = hit[0][0].get(args[2])
    if v is None or not re.search(args[3], v): fail("<b n=%s %s=%r>, wanted %s" % (args[1], args[2], v, args[3]))
    print("<b %s %s=%s>" % (args[1], args[2], v[:90]))
elif cmd == "bodyisrow":                # bodyisrow FILE NAME : the one <b n=NAME> has the p= and l= of the best-ranked NAME row
    t = load(args[0]); hit = [b[0] for b in bodies(t) if b[0].get("n") == args[1]]
    if len(hit) != 1: fail("%d <b n=%s>, wanted exactly 1" % (len(hit), args[1]))
    r = first_row(t, args[1])
    if r is None: fail("no <d n=%s> row to compare against" % args[1])
    rp = r.get("p", ""); bp = hit[0].get("p", "")
    if (bp, hit[0].get("l")) != (rp, r.get("l")): fail("<b p=%s l=%s> is not the best-ranked row p=%s l=%s r=%s" % (bp, hit[0].get("l"), rp, r.get("l"), r.get("r")))
    print("<b p=%s l=%s> = r%s" % (bp, hit[0].get("l"), r.get("r")))
elif cmd == "bodykinds":                # bodykinds FILE : every <b t=> is a callable kind
    t = load(args[0]); bad = [(b[0].get("n"), b[0].get("t")) for b in bodies(t) if b[0].get("t") not in ("fn", "method")]
    if bad: fail("non-callable bodies: %s" % bad)
elif cmd == "rowis":                    # rowis FILE R NAME PATH : the row at rank R is NAME in PATH (an arm's premise)
    t = load(args[0]); hit = [x for x in rows(t) if x[0] == int(args[1])]
    if not hit or hit[0][1] != args[2] or hit[0][2] != args[3]: fail("premise: r%s is %s, wanted %s %s" % (args[1], hit[0][1:3] if hit else None, args[2], args[3]))
elif cmd == "cutatline":                # cutatline FILE NAME MAXB : a head-cut body that is the def's first K whole source lines, <= MAXB
    t = load(args[0]); hit = [b for b in bodies(t) if b[0].get("n") == args[1]]
    if not hit: fail("no <b n=%s>" % args[1])
    a, cd, el = hit[0]
    if len(cd.encode()) > int(args[2]): fail("CDATA %d B > %s" % (len(cd.encode()), args[2]))
    m = re.fullmatch(r'(\d+)-(\d+)/(\d+)', a.get("lines", ""))
    if not m: fail("lines=%r is not lo-hi/T" % a.get("lines"))
    lo, hi, tot = map(int, m.groups())
    src = open(os.path.join(os.environ["FIX"], a["p"]), encoding="utf-8").read().split("\n")
    l0 = int(a["l"])
    want = "\n".join(src[l0 - 1: l0 - 1 + (hi - lo + 1)])
    if cd.rstrip("\n") != want.rstrip("\n"): fail("CDATA is not source lines %d..%d of %s (a cut inside a line?)" % (l0, l0 + hi - lo, a["p"]))
    if not (lo == 1 and hi < tot): fail("lines=%s does not describe a head cut" % a.get("lines"))
    print("cut %s lines=%s %d B" % (args[1], a["lines"], len(cd.encode())))
elif cmd == "nextof":                   # nextof FILE NAME : print the <b n=NAME next=> value
    t = load(args[0]); hit = [b for b in bodies(t) if b[0].get("n") == args[1]]
    if not hit or not hit[0][0].get("next"): fail("no <b n=%s next=>" % args[1])
    print(hit[0][0]["next"])
elif cmd == "startsat":                 # startsat FILE PATH LINE : the first <b> CDATA begins with source line LINE of PATH
    t = load(args[0]); bs = bodies(t)
    if not bs: fail("the continuation served no body")
    src = open(os.path.join(os.environ["FIX"], args[1]), encoding="utf-8").read().split("\n")
    want = src[int(args[2]) - 1]
    if not bs[0][1].startswith(want): fail("continuation begins %r, wanted source line %s %r" % (bs[0][1][:60], args[2], want[:60]))
elif cmd == "utf8whole":                # utf8whole FILE NAME TAIL : the <b n=NAME> CDATA is valid UTF-8 and ends with TAIL
    raw = open(args[0], "rb").read()
    m = re.search(rb'<b [^>]*n="' + re.escape(args[1].encode()) + rb'"[^>]*><!\[CDATA\[(.*?)\]\]>', raw, re.S)
    if not m: fail("no <b n=%s> CDATA" % args[1])
    try: cd = m.group(1).decode("utf-8", "strict")
    except UnicodeDecodeError as e: fail("CDATA is not valid UTF-8: %s" % e)
    if args[2] not in cd: fail("the line is not whole: %r missing" % args[2])
    print("%s %d B valid UTF-8" % (args[1], len(m.group(1))))
elif cmd == "sectionsame":              # sectionsame FILE_SLOT FILE_OFF : identical once the slot's own bytes are removed
    a, b = strip_slot(load(args[0])), strip_slot(load(args[1]))
    if a != b:
        i = next((k for k in range(min(len(a), len(b))) if a[k] != b[k]), min(len(a), len(b)))
        fail("sections differ at byte %d: slot %r | off %r" % (i, a[max(0, i - 40): i + 60], b[max(0, i - 40): i + 60]))
elif cmd == "estdelta":                 # estdelta FILE_SLOT FILE_OFF : est_tokens(slot) - est_tokens(off) >= body element bytes / 10
    ts, to = load(args[0]), load(args[1])
    es, eo = root(ts).get("est_tokens", ""), root(to).get("est_tokens", "")
    if not (es.isdigit() and eo.isdigit()): fail("est_tokens missing: %r %r" % (es, eo))
    m = re.search(r'<bodies\b.*?</bodies>', ts, re.S)
    eb = len(m.group(0).encode()) if m else 0
    if eb == 0: fail("no <bodies> element to price (est %s vs %s)" % (es, eo))
    if int(es) - int(eo) < eb // 10: fail("est_tokens %s - %s < %d B / 10" % (es, eo, eb))
    print("est_tokens %s vs %s (+%d for %d B)" % (es, eo, int(es) - int(eo), eb))
elif cmd == "legend":                   # legend FILE STR... : every STR appears inside the XML comments
    lg = legend(load(args[0])); miss = [s for s in args[1:] if s not in lg]
    if miss: fail("legend lacks %s" % miss)
elif cmd == "nodashes":                 # nodashes FILE : no "--" inside any XML comment
    bad = [c[:60] for c in re.findall(r'<!--(.*?)-->', load(args[0]), re.S) if "--" in c]
    if bad: fail("'--' inside an XML comment: %s" % bad)
elif cmd == "rowkeys":                  # rowkeys FILE : print "n|p" per row in rank order (one per line)
    print("\n".join("%s|%s" % (n, p) for r, n, p, a in rows(load(args[0]))))
elif cmd == "prefix":                   # prefix FILE_A FILE_B : A's rows are a rank prefix of B's, A has >= 4 rows
    ra = [(n, p) for r, n, p, a in rows(load(args[0]))]; rb = [(n, p) for r, n, p, a in rows(load(args[1]))]
    if len(ra) < 4: fail("only %d rows (the rank 1..4 floor)" % len(ra))
    if ra != rb[:len(ra)]: fail("rows are not a rank prefix of the default answer (first diff at %d)" % next(i for i in range(len(ra)) if i >= len(rb) or ra[i] != rb[i]))
    print("%d rows, a prefix of %d" % (len(ra), len(rb)))
elif cmd == "nrows":                    # nrows FILE
    print(len(rows(load(args[0]))))
elif cmd == "bodybytes":                # bodybytes FILE : CDATA bytes of every <b>
    print(sum(len(b[1].encode()) for b in bodies(load(args[0]))))
elif cmd == "cdata":                    # cdata FILE NAME : the <b n=NAME> CDATA, verbatim
    hit = [b for b in bodies(load(args[0])) if b[0].get("n") == args[1]]
    if not hit: fail("no <b n=%s>" % args[1])
    sys.stdout.write(hit[0][1])
elif cmd == "bodiesnext":               # bodiesnext FILE REGEX : a <bodies> element (the slot's own disclosure) carries next=~REGEX
    m = re.search(r'<bodies\b([^>]*)>', load(args[0]))
    if not m: fail("no <bodies> element (a skipped slot discloses itself there)")
    a = attrs(m.group(1))
    if not re.search(args[1], a.get("next", "")): fail("<bodies %s> has no next= matching %s" % (m.group(1)[:120], args[1]))
    print("<bodies next=%s>" % a.get("next"))
elif cmd == "over":                     # over FILE : the root carries no over_ceiling="1"
    if root(load(args[0])).get("over_ceiling") == "1": fail("over_ceiling=\"1\" on the root")
elif cmd == "sigsopen":                 # sigsopen FILE : the answer has a <sigs block (premise)
    if "<sigs" not in load(args[0]): fail("no <sigs>")
else:
    fail("unknown check " + cmd)
PY
export FIX
chk(){ python3 "$TMP/chk.py" "$@"; }
run(){ "$BIN" "$FIX" --cache="$TMP/rw.cache" "$@" 2>/dev/null; }       # a fresh cache per gate run: this binary's own parse
runoff(){ RIPWIRE_NO_BODY_SLOT=1 "$BIN" "$FIX" --cache="$TMP/rw.cache" "$@" 2>/dev/null; }
strip_at(){ sed -e 's/ at="[^"]*"//g' -e 's/"at":"[^"]*",\{0,1\}//g'; }
cat > "$TMP/mcptext.py" <<'PY'
import json, sys
for line in sys.stdin:
    line = line.strip()
    if not line: continue
    try: d = json.loads(line)
    except Exception: continue
    for c in (d.get("result") or {}).get("content") or []:
        if c.get("type") == "text": sys.stdout.write(c["text"])
PY
mcpfor(){   # mcpfor TASK [EXTRA_JSON_MEMBERS] > FILE
    python3 -c 'import json,sys; a={"path":sys.argv[1],"task":sys.argv[2]}; a.update(json.loads(sys.argv[3] or "{}")); print(json.dumps({"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"for","arguments":a}}))' "$FIX" "$1" "${2:-}" \
        | "$BIN" --mcp 2>/dev/null | python3 "$TMP/mcptext.py"
}

T_D="How does the JWT middleware verify a token?"
T_PY="How does Stylesheet apply the parsed rules to a widget?"
T_C="How does cmd_find_target resolve the session, window and pane of a command?"
T_SUB="How does tmux resolve which session, window and pane a command targets (the -t target)?"
T_BIG="How does render_screen draw the screen?"
T_HYD="How does hydrate restore the state?"
T_RS="How does the indexer walk the module tree breadth-first to index the crate source paths?"
T_CPP="How does the C++ builder build the plan and run its commands?"
T_GO="How does the packager package the files into an archive?"
T_DTS="How does negotiate pick the content type?"
T_DTS2="How does handshake upgrade the socket?"
T_TEST="How does roundtrip encode a token string?"
T_TWO="How does render produce the view markup?"
T_REF="How does refresh repaint a widget?"
T_STRUCT="How is the screen struct laid out?"
T_WHERE="Where does the JWT middleware verify a token?"
T_SHOW="show documentation of the JWT middleware token check"
T_NOCUE="JWT middleware token verification"

# key|task — every question the gate asks, answered with the slot on and with RIPWIRE_NO_BODY_SLOT=1
QUESTIONS="d|$T_D
py|$T_PY
c|$T_C
sub|$T_SUB
big|$T_BIG
hyd|$T_HYD
rs|$T_RS
cpp|$T_CPP
go|$T_GO
dts|$T_DTS
dts2|$T_DTS2
test|$T_TEST
two|$T_TWO
ref|$T_REF
struct|$T_STRUCT
where|$T_WHERE
show|$T_SHOW
nocue|$T_NOCUE"
# fail fast: a binary that cannot answer the first question answers none of the ~90 below (binoverridecheck's sentinel)
run --for="$T_D" >"$TMP/probe.xml"
if ! grep -q '<sigs' "$TMP/probe.xml"; then no "(0) $BIN answered no <sigs> on the fixture — every arm would be vacuous"; exit 1; fi
while IFS='|' read -r k q; do
    run --for="$q" >"$TMP/$k.xml"
    runoff --for="$q" >"$TMP/$k.off.xml"
    for f in "$k" "$k.off"; do
        if r="$( chk sigsopen "$TMP/$f.xml" )"; then :; else no "(0) $f.xml: $r — the fixture answered nothing, every arm reading it would be vacuous"; fi
    done
done <<<"$QUESTIONS"
run --for="$T_D" --legend=compact >"$TMP/d.c.xml"

# ── D: the body slot in the default regime ─────────────────────────────────────────────────────────────────
if r="$( chk rootattr "$TMP/d.xml" bundle compact )"; then ok "(D1) T-D premise: $r"; else no "(D1) T-D premise: $r"; fi
if r="$( chk bodycount "$TMP/d.xml" 1 )"; then ok "(D1) T-D: exactly one body, root $r"; else no "(D1) T-D: $r"; fi
if r="$( chk body "$TMP/d.xml" jwt src/middleware/jwt/jwt.ts "getCookie" "HTTPException(401" "ctx.set('jwtPayload'" )"; then ok "(D1) T-D: $r holds the three kill strings"; else no "(D1) T-D: $r"; fi
if r="$( chk callsin "$TMP/d.xml" jwt verify )"; then ok "(D1) T-D: the body keeps <calls> (names verify)"; else no "(D1) T-D: $r"; fi
if r="$( chk nobodyof "$TMP/d.xml" verify )"; then ok "(D2) T-D: not verify's body — jwt calls verify (the entry point)"; else no "(D2) T-D: $r"; fi
if r="$( chk estdelta "$TMP/d.xml" "$TMP/d.off.xml" )"; then ok "(D3) T-D: $r"; else no "(D3) T-D: $r"; fi
while IFS='|' read -r k q; do
    if r="$( chk sectionsame "$TMP/$k.xml" "$TMP/$k.off.xml" )"; then ok "(D4) $k: sigs/hops/tail identical slot vs RIPWIRE_NO_BODY_SLOT=1 (not paid in rows)"; else no "(D4) $k: $r"; fi
    if r="$( chk nobody "$TMP/$k.off.xml" )"; then ok "(D4) $k: RIPWIRE_NO_BODY_SLOT=1 serves no body"; else no "(D4) $k: RIPWIRE_NO_BODY_SLOT=1: $r"; fi
done <<<"$QUESTIONS"
if r="$( chk legend "$TMP/d.xml" 'bodies=1' )"; then ok "(D5) full legend explains bodies=1 (the owner body)"; else no "(D5) full legend: $r"; fi
if r="$( chk bodycount "$TMP/d.c.xml" 1 )"; then ok "(D5) compact dialect: the body rides too"; else no "(D5) compact dialect: $r"; fi
if r="$( chk legend "$TMP/d.c.xml" 'b t= n= p= l=' )"; then ok "(D5) compact legend defines b t= n= p= l= when a body rides"; else no "(D5) compact legend: $r"; fi
for k in where show nocue; do
    if r="$( chk rootattr "$TMP/$k.xml" bodies 0 )"; then ok "(D6) $k (no explanatory cue): $r"; else no "(D6) $k: $r"; fi
    if r="$( chk nobody "$TMP/$k.xml" )"; then ok "(D6) $k: no <bodies> element"; else no "(D6) $k: $r"; fi
done

# postures that never take the slot: identical with the slot on and off
run --for="$T_D" --signatures-only >"$TMP/p.sig.xml"
if r="$( chk nobody "$TMP/p.sig.xml" )" && r2="$( chk noroot "$TMP/p.sig.xml" bodies )"; then ok "(D7) --signatures-only: no body, no bodies="; else no "(D7) --signatures-only: ${r:-} ${r2:-}"; fi
run --for="$T_D" --json >"$TMP/p.json"
if python3 -c "import json,sys; d=json.load(open(sys.argv[1])); sys.exit(0 if 'bodies' not in d and len(d.get('sigs',[]))>=1 else 1)" "$TMP/p.json"; then
    ok "(D7) --json: no bodies key, sigs served (the JSON dialect carries no bodies: named floor)"
else
    no "(D7) --json: a bodies key rides the JSON dialect (or no sigs)"
fi
for a in --auto-bodies --detail=1 --format=candidates; do
    run --for="$T_D" "$a" >"$TMP/p.on.xml"; runoff --for="$T_D" "$a" >"$TMP/p.off.xml"
    if [ -s "$TMP/p.on.xml" ] && cmp -s "$TMP/p.on.xml" "$TMP/p.off.xml"; then ok "(D7) $a: byte-identical with and without the slot"; else no "(D7) $a: differs from RIPWIRE_NO_BODY_SLOT=1 (or empty)"; fi
done
if r="$( chk rootattr "$TMP/p.on.xml" bodies '.*' 2>/dev/null )"; then no "(D7) --format=candidates carries bodies="; else ok "(D7) --format=candidates carries no bodies="; fi
run --for=verify >"$TMP/p.ne.xml"; runoff --for=verify >"$TMP/p.ne.off.xml"
if grep -q 'route="name-exact' "$TMP/p.ne.xml" && cmp -s "$TMP/p.ne.xml" "$TMP/p.ne.off.xml"; then ok "(D7) name-exact --for=verify: auto bundle, identical with and without the slot"; else no "(D7) name-exact: not name-exact, or differs from RIPWIRE_NO_BODY_SLOT=1"; fi

# languages, owners, near misses
if r="$( chk rowis "$TMP/py.xml" 1 Stylesheet src/textual/css/stylesheet.py )"; then :; else no "(D8) premise: $r"; fi
if r="$( chk body "$TMP/py.xml" apply src/textual/css/stylesheet.py "_check_rule(" "widget.refresh()" )"; then ok "(D8) Python: $r (the method)"; else no "(D8) Python: $r"; fi
if r="$( chk nobodyof "$TMP/py.xml" Stylesheet )"; then ok "(D8) Python: the class Stylesheet (r1) is never the body"; else no "(D8) Python: $r"; fi
if r="$( chk body "$TMP/c.xml" cmd_find_target src/c/cmd-find.c "cmd_find_get_session(" "cmd_find_valid_state(" )"; then ok "(D9) C: $r"; else no "(D9) C: $r"; fi
if r="$( chk body "$TMP/sub.xml" cmd_find_target src/c/cmd-find.c "cmd_find_get_session(" )"; then ok "(D9b) C subtoken fallback ('target'): $r"; else no "(D9b) C subtoken fallback: $r"; fi
if r="$( chk bodyattr "$TMP/big.xml" render_screen truncated '^1$' )"; then ok "(D10) oversized owner: $r"; else no "(D10) oversized: $r"; fi
if r="$( chk cutatline "$TMP/big.xml" render_screen 6000 )"; then ok "(D10) oversized owner: $r (whole lines, <= 6000 B)"; else no "(D10) oversized: $r"; fi
if nx="$( chk nextof "$TMP/big.xml" render_screen )" && printf '%s' "$nx" | grep -q '^--expand=src/c/render.c:'; then
    ok "(D10) oversized owner: next=$nx"
    hi="$( grep -o '<b [^>]*n="render_screen"[^>]*' "$TMP/big.xml" | grep -o 'lines="[0-9]*-[0-9]*' | sed 's/.*-//' )"
    l0="$( grep -o '<b [^>]*n="render_screen"[^>]*' "$TMP/big.xml" | grep -o ' l="[0-9]*"' | grep -o '[0-9]*' )"
    if [ -n "$hi" ] && [ -n "$l0" ]; then
        run "$nx" >"$TMP/big.next.xml"
        if r="$( chk startsat "$TMP/big.next.xml" src/c/render.c $(( l0 + hi )) )"; then ok "(D10) the next= serves the rest, from source line $(( l0 + hi ))"; else no "(D10) next=: $r"; fi
    else
        no "(D10) could not read lines=/l= off the cut body"
    fi
else
    no "(D10) oversized: no <b n=render_screen next=--expand=src/c/render.c:…> (${nx:-none})"
fi
if r="$( chk over "$TMP/big.xml" )"; then ok "(D10) oversized owner: no over_ceiling on the root (the guard never fires for the body)"; else no "(D10) $r"; fi
if r="$( chk bodyattr "$TMP/hyd.xml" hydrate over_ceiling '^1$' )"; then ok "(D11) one line over the cap: $r (served whole)"; else no "(D11) one line over the cap: $r"; fi
if r="$( chk utf8whole "$TMP/hyd.xml" hydrate 'restored: true' )"; then ok "(D11) one line over the cap: $r, the whole line"; else no "(D11) $r"; fi
if r="$( chk rowis "$TMP/rs.xml" 1 walk src/rs/indexer.rs )"; then :; else no "(L1) premise: $r"; fi
if r="$( chk body "$TMP/rs.xml" index src/rs/indexer.rs "let order = walk(" )"; then ok "(L1) Rust: $r — the entry point (index calls walk) beats walk at r1"; else no "(L1) Rust: $r"; fi
if r="$( chk nobodyof "$TMP/rs.xml" walk )"; then ok "(L1) Rust: not walk's body"; else no "(L1) Rust: $r"; fi
if r="$( chk rowis "$TMP/cpp.xml" 1 Build src/cpp/build.h )"; then :; else no "(L2) premise: $r"; fi
if r="$( chk body "$TMP/cpp.xml" Build src/cpp/build.cc "cannot make progress" )"; then ok "(L2) C++: $r — the out-of-line definition, not the r1 header declaration"; else no "(L2) C++: $r"; fi
if r="$( chk nobodyat "$TMP/cpp.xml" '\.h$' )" && r2="$( chk nobodyof "$TMP/cpp.xml" Builder )"; then ok "(L2) C++: no header declaration, no class body"; else no "(L2) C++: ${r:-} ${r2:-}"; fi
if r="$( chk body "$TMP/go.xml" Package src/go/packager.go "guessing packager from target file extension" )"; then ok "(L3) Go: $r (the method)"; else no "(L3) Go: $r"; fi
if r="$( chk nobodyof "$TMP/go.xml" Packager )"; then ok "(L3) Go: the type Packager is never the body"; else no "(L3) Go: $r"; fi
if r="$( chk rowis "$TMP/dts.xml" 1 negotiate src/types/global.d.ts )"; then :; else no "(N1) premise: $r"; fi
if r="$( chk body "$TMP/dts.xml" negotiate src/negotiate.ts "negotiate fallback" )"; then ok "(N1) TS: $r — the definition, not the r1 .d.ts declaration"; else no "(N1) TS: $r"; fi
for k in dts dts2; do
    if r="$( chk nobodyat "$TMP/$k.xml" '\.d\.ts$' )"; then ok "(N1) $k: no body from a .d.ts declaration"; else no "(N1) $k: $r"; fi
done
if r="$( chk bodycount "$TMP/test.xml" 1 )"; then ok "(N2) the only name owner is a tests/ helper: the subtoken fallback serves one body"; else no "(N2) $r"; fi
if r="$( chk nobodyat "$TMP/test.xml" '(^|/)tests?/' )" && r2="$( chk nobodyof "$TMP/test.xml" roundtrip )"; then ok "(N2) no body from tests/ (roundtrip is a test symbol)"; else no "(N2) ${r:-} ${r2:-}"; fi
if r="$( chk bodycount "$TMP/two.xml" 1 )"; then ok "(N3) two equal owners: one body"; else no "(N3) $r"; fi
if r="$( chk bodyisrow "$TMP/two.xml" render )"; then ok "(N3) two equal owners: $r (the better-ranked)"; else no "(N3) $r"; fi
run --for="$T_TWO" >"$TMP/two2.xml"
if cmp -s "$TMP/two.xml" "$TMP/two2.xml"; then ok "(N3) two equal owners: deterministic x2"; else no "(N3) two equal owners: two runs differ"; fi
if r="$( chk bodycount "$TMP/ref.xml" 1 )"; then ok "(N4) two refresh methods in one file: one body"; else no "(N4) $r"; fi
if r="$( chk bodyisrow "$TMP/ref.xml" refresh )"; then ok "(N4) $r (entity identity: the ranked row's own definition)"; else no "(N4) $r"; fi
if r="$( chk nobodyof "$TMP/struct.xml" screen )"; then ok "(N5) the struct screen (r1) is never the body"; else no "(N5) $r"; fi
while IFS='|' read -r k q; do
    if r="$( chk bodykinds "$TMP/$k.xml" )"; then :; else no "(N5) $k: $r"; fi
done <<<"$QUESTIONS"
ok "(N5) every <b t=> on the fixture answers is fn/method (FAIL lines above otherwise)"

# ── E: explicit targets (reserve-sections-first) ───────────────────────────────────────────────────────────
run --for="$T_D" --token-budget=3000 >"$TMP/e3000.xml"
if r="$( chk body "$TMP/e3000.xml" jwt src/middleware/jwt/jwt.ts "getCookie" )"; then ok "(E1) --token-budget=3000: $r"; else no "(E1) --token-budget=3000: $r"; fi
if r="$( chk prefix "$TMP/e3000.xml" "$TMP/d.xml" )"; then ok "(E1) --token-budget=3000: $r"; else no "(E1) --token-budget=3000: $r"; fi
if r="$( chk over "$TMP/e3000.xml" )"; then ok "(E1) --token-budget=3000: no over_ceiling"; else no "(E1) $r"; fi
run --for="$T_D" --token-budget=400 >"$TMP/e400.xml"
if r="$( chk rootattr "$TMP/e400.xml" bodies 0 )" && r2="$( chk rootattr "$TMP/e400.xml" reason budget )"; then ok "(E2) --token-budget=400: bodies=0 reason=budget"; else no "(E2) --token-budget=400: ${r:-} ${r2:-}"; fi
if r="$( chk bodiesnext "$TMP/e400.xml" '^--expand=src/middleware/jwt/jwt\.ts:' )"; then ok "(E2) --token-budget=400: the skipped slot discloses itself, recoverable: $r"; else no "(E2) --token-budget=400: $r"; fi
if r="$( chk prefix "$TMP/e400.xml" "$TMP/d.xml" )"; then ok "(E2) --token-budget=400: $r"; else no "(E2) --token-budget=400: $r"; fi
prev_rows=0; prev_b=0; mono=1; nflag=""
for n in 300 500 800 1200 2000 3000 5000 8000 1000000; do
    run --for="$T_D" --token-budget=$n >"$TMP/s$n.xml"; runoff --for="$T_D" --token-budget=$n >"$TMP/s$n.off.xml"
    rws="$( chk nrows "$TMP/s$n.xml" )"; rwo="$( chk nrows "$TMP/s$n.off.xml" )"; bb="$( chk bodybytes "$TMP/s$n.xml" )"
    case "$rws$rwo$bb" in *[!0-9]*|'') no "(E3) N=$n: unreadable counts rows=$rws off=$rwo body=$bb"; mono=0; continue;; esac
    if ! chk prefix "$TMP/s$n.xml" "$TMP/d.xml" >/dev/null; then no "(E3) N=$n: rows are not a rank prefix of the default rows"; mono=0; fi
    if ! chk over "$TMP/s$n.xml" >/dev/null; then no "(E3) N=$n: over_ceiling on the root"; mono=0; fi
    [ "$rws" -lt "$prev_rows" ] && { no "(E3) N=$n: rows $rws < $prev_rows at the smaller N"; mono=0; }
    [ "$bb" -lt "$prev_b" ] && { no "(E3) N=$n: body $bb B < $prev_b B at the smaller N"; mono=0; }
    [ "$rws" -gt "$rwo" ] && { no "(E3) N=$n: rows $rws > slot-off $rwo (the body is reserved first, it never adds rows)"; mono=0; }
    prev_rows=$rws; prev_b=$bb; nflag="$nflag $n:$rws/$rwo/${bb}B"
done
if [ "$mono" = 1 ]; then ok "(E3) sweep rows(slot)/rows(off)/body:$nflag — rank prefix, monotone, <= slot-off, no over_ceiling"; fi
cdw=0; cdd=0
chk cdata "$TMP/s1000000.xml" jwt >"$TMP/cd.wide" 2>/dev/null || cdw=1
chk cdata "$TMP/d.xml" jwt >"$TMP/cd.default" 2>/dev/null || cdd=1
if [ "$cdw$cdd" = "00" ] && [ -s "$TMP/cd.wide" ] && cmp -s "$TMP/cd.wide" "$TMP/cd.default"; then ok "(E3) N=1000000: the body equals the default answer's body"; else no "(E3) N=1000000: no body, or it differs from the default answer's"; fi

# ── MC: MCP for parity ─────────────────────────────────────────────────────────────────────────────────────
mcpfor "$T_D" >"$TMP/m.d.xml"; mcpfor "$T_NOCUE" >"$TMP/m.nocue.xml"; mcpfor "$T_D" '{"budget_tokens":400}' >"$TMP/m.d400.xml"
for f in m.d m.nocue m.d400; do
    if r="$( chk sigsopen "$TMP/$f.xml" )"; then :; else no "(MC0) MCP $f answered nothing: $r"; fi
done
if r="$( chk body "$TMP/m.d.xml" jwt src/middleware/jwt/jwt.ts "getCookie" "HTTPException(401" "ctx.set('jwtPayload'" )"; then ok "(MC1) MCP for: $r"; else no "(MC1) MCP for: $r"; fi
cdm=0; chk cdata "$TMP/m.d.xml" jwt >"$TMP/cd.mcp" 2>/dev/null || cdm=1
if [ "$cdm$cdd" = "00" ] && [ -s "$TMP/cd.mcp" ] && cmp -s "$TMP/cd.mcp" "$TMP/cd.default"; then ok "(MC2) MCP for: the body equals the CLI body byte for byte"; else no "(MC2) MCP for: no body, or it differs from the CLI's"; fi
if r="$( chk nobody "$TMP/m.nocue.xml" )"; then ok "(MC3) MCP for, no cue: no body"; else no "(MC3) $r"; fi
if r="$( chk nobodyof "$TMP/m.d400.xml" jwt )" && r2="$( chk bodiesnext "$TMP/m.d400.xml" '^--expand=src/middleware/jwt/jwt\.ts:' )"; then ok "(MC4) MCP budget_tokens=400: no body, the owner recoverable by next=--expand"; else no "(MC4) MCP budget_tokens=400: ${r:-} ${r2:-}"; fi

# ── M: mutation controls — the checkers reject canned pre-change shapes ─────────────────────────────────────
printf '%s' '<ctx task="t" bundle="compact" bodies="0" reason="compact-route" est_tokens="10"><!-- l --><sigs><d l="12" n="jwt" p="src/middleware/jwt/jwt.ts" r="1">x</d></sigs></ctx>' >"$TMP/c1.xml"
if chk body "$TMP/c1.xml" jwt src/middleware/jwt/jwt.ts getCookie >/dev/null 2>&1; then no "(M1) the body checker accepted a canned no-body answer"; else ok "(M1) the body checker rejects a canned no-body answer"; fi
printf '%s' '<ctx task="t" bodies="1"><sigs><d l="4" n="roundtrip" p="tests/test_tokens.py" r="1">x</d></sigs><bodies shown="1" total="1" capped="0"><b t="fn" l="4" p="tests/test_tokens.py" n="roundtrip"><![CDATA[def roundtrip(text):]]></b></bodies></ctx>' >"$TMP/c2.xml"
if chk nobodyat "$TMP/c2.xml" '(^|/)tests?/' >/dev/null 2>&1; then no "(M2) the tests/ checker accepted a canned tests/ body"; else ok "(M2) the tests/ checker rejects a canned tests/ body"; fi
printf '%s' '<ctx task="t" bodies="1"><sigs><d l="7" n="render_screen" p="src/c/render.c" r="1">x</d></sigs><bodies shown="1" total="1" capped="0"><b t="fn" l="7" p="src/c/render.c" n="render_screen"><![CDATA[void]]></b></bodies></ctx>' >"$TMP/c3.xml"
if chk bodyattr "$TMP/c3.xml" render_screen truncated '^1$' >/dev/null 2>&1 || chk nextof "$TMP/c3.xml" render_screen >/dev/null 2>&1; then no "(M3) the cut checkers accepted a silent partial body"; else ok "(M3) the cut checkers reject a partial body with no truncated=/next="; fi
sed 's/<hops /<hops x="1" /' "$TMP/d.off.xml" >"$TMP/c4.xml"
if grep -q '<hops ' "$TMP/d.off.xml"; then
    if chk sectionsame "$TMP/c4.xml" "$TMP/d.off.xml" >/dev/null 2>&1; then no "(M4) the section checker missed a changed <hops>"; else ok "(M4) the section checker sees a changed <hops>"; fi
else
    no "(M4) premise: T-D has no <hops> element to mutate"
fi

# ── X: hygiene ──────────────────────────────────────────────────────────────────────────────────────────────
run --for="$T_D" >"$TMP/d2.xml"
if cmp -s "$TMP/d.xml" "$TMP/d2.xml"; then ok "(X1) T-D deterministic x2"; else no "(X1) T-D differs between two runs"; fi
if command -v xmllint >/dev/null 2>&1; then
    for f in d d.c big hyd py rs cpp go m.d e400; do
        if xmllint --noout "$TMP/$f.xml" 2>/dev/null; then ok "(X2) $f.xml is well-formed XML"; else no "(X2) $f.xml failed xmllint"; fi
    done
else
    skip "(X2) xmllint not installed — well-formedness not checked here"
fi
for f in d d.c big hyd e400; do
    if r="$( chk nodashes "$TMP/$f.xml" )"; then ok "(X3) $f.xml: no '--' inside an XML comment"; else no "(X3) $f.xml: $r"; fi
done

# ── I: optional byte identity against the pre-change binary where the slot must not fire ─────────────────────
if [ -n "${RIPWIRE_BASE_BIN:-}" ] && [ -x "${RIPWIRE_BASE_BIN}" ]; then
    RIPWIRE_NO_BODY_SLOT=1 "$BIN" "$FIX" --no-cache --for="$T_D" 2>/dev/null | strip_at >"$TMP/i.head"
    "$RIPWIRE_BASE_BIN" "$FIX" --no-cache --for="$T_D" 2>/dev/null | strip_at >"$TMP/i.base"
    if [ -s "$TMP/i.base" ] && cmp -s "$TMP/i.base" "$TMP/i.head"; then ok "(I0) RIPWIRE_NO_BODY_SLOT=1 T-D == the base binary byte for byte"; else no "(I0) RIPWIRE_NO_BODY_SLOT=1 T-D differs from the base binary"; fi
    i=0
    while IFS= read -r line; do              # one argv per line, arguments separated by '|' (values may hold spaces)
        i=$(( i + 1 ))
        IFS='|' read -r -a argv <<<"$line"
        "$RIPWIRE_BASE_BIN" "$FIX" --no-cache "${argv[@]}" 2>/dev/null | strip_at >"$TMP/i.base"
        "$BIN"              "$FIX" --no-cache "${argv[@]}" 2>/dev/null | strip_at >"$TMP/i.head"
        if [ ! -s "$TMP/i.base" ]; then no "(I$i) the base binary answered nothing for: $line"; continue; fi
        if cmp -s "$TMP/i.base" "$TMP/i.head"; then ok "(I$i) byte-identical: $line"; else no "(I$i) differs from the base binary: $line"; fi
    done <<'ARGV'
--for=JWT middleware token verification
--for=Where does the JWT middleware verify a token?
--for=How does the JWT middleware verify a token?|--signatures-only
--for=How does the JWT middleware verify a token?|--json
--for=How does the JWT middleware verify a token?|--auto-bodies
--for=How does the JWT middleware verify a token?|--detail=1
--for=verify
--callers=verify
--expand=src/middleware/jwt/jwt.ts:jwt
ARGV
else
    skip "(I) RIPWIRE_BASE_BIN unset or not executable — base-binary byte identity not run here"
fi

exit $fail
