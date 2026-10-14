#!/usr/bin/env bash
# attribmultidefcheck.sh — per-row DEFINITION ATTRIBUTION on answers whose selector names several definitions.
#
#   test/attribmultidefcheck.sh [BIN]        # BIN defaults to $RIPWIRE_BIN, then build/ripwire
#
# The defect class (round 4, O1 false claims ninja-07 / nfpm-06 / ninja-26; kysely-06 / mdbook-06 complete only after a
# follow-on): a selector that matches two or more definitions (defs="2"+) answers with the UNION of their callers,
# callees or dependants, and every row reads as belonging to one definition. fix27-impact-multidef hedged the --impact
# file rollup only; this gate covers every verb that folds per-selector rows.
#
# The contract (docs: the lane design, reports/attrib-multidef.md):
#   * DEFINITION GROUP: a bodied definition plus the bodyless declarations the parser pairs with it (same language family,
#     scope, name and parameter count, one bodied partner); an unpaired declaration is its own group. An answer is
#     MULTI-GROUP when its selector matched two or more groups.
#   * A multi-group answer lists the groups as <def ix= t= [sc=] p= [bodyless="1"]/> children of its root (CLI JSON
#     "def_table":[{"ix","t","sc","p"}], MCP "def_table":[{"ix","kind","scope","file","line"}]); every row carries
#     of_def="TOKENS" (JSON/MCP "of_def":[TOKENS]): space-separated K (a <def ix=>) with a mark — K alone = a proven edge
#     ties the row to group K, K~n = by name alone, K~s = only as one arm of a call the resolver split between
#     same-named definitions. callers: row -> group; callees: group -> row; impact: a chain of calls from the row to the
#     group (a by-name hop makes it ~n, else a split hop ~s); safe-delete <c> rows as callers; the impact <rf> rollup
#     row: the union of its symbols' tokens, the strongest mark per K.
#   * A ONE-group answer (a single definition, or a definition with its own declarations) carries no table and no of_def:
#     byte-identical. Its rows that are one arm of a split call carry prov="split" (the map's per-edge vocabulary) when
#     the selector matched 2+ definitions (the ninja-26 shape: a declaration + its definition).
#   * --path: an endpoint with several definitions that no path bound (reachable="0") carries no from_p=/to_p=.
#
# Corpus test/attribmultideffix (in-tree, outside test/fixture):
#   cpp/  ninja-07 shape StringPiece::empty vs EvalString::empty (UsesPiece/UsesEval/UsesBoth, RunPiece/RunEval at depth 2
#         in their own files), ninja-26 shape ManifestParser::ParseDefault (decl + def) calling the CanonicalizePath
#         overload pair (a split call), near misses in fmt.{h,cc}: Fmt::put overloads in ONE class (split), outer::run vs
#         outer::inner::run (nested namespace), Box<T>::get vs the Box<int> specialisation; ReadPath decl + def.
#   go/   nfpm-06 shape: three Get (files.Get fn, modtime Get fn, (*Info).Get method), cross_kind fn:2,method:1.
#   py/ts/java/  two same-named methods of different classes IN ONE FILE (entity identity: owner, not file + name);
#         py/ also draw_mixed: a proven Widget.render call and a by-name thing.render call in one row.
#   rs/   mdbook-06 shape: Book::iter vs Shelf::iter on param-typed receivers (by name + split today: hedged tokens).
#   objc/ two -area methods, typed message receivers (split today: hedged tokens).
#
# Arm classes in the output: [P] positive — the defect shape, RED on a binary without per-row attribution; [N] near miss —
# a shape the new code could plausibly mis-handle (a one-group answer that must stay plain, an overload or a namesake that
# must not be over-attributed); it is red on such a binary only where it needs a hedge that binary cannot print; [C] control
# — holds on every binary.
set -u
ROOT="$( cd "$( dirname "$0" )/.." && pwd )"
BIN="${1:-${RIPWIRE_BIN:-$ROOT/build/ripwire}}"
[ "${BIN#/}" = "$BIN" ] && BIN="$ROOT/$BIN"
FIX="$ROOT/test/attribmultideffix"
DIR="$( mktemp -d )"; trap 'rm -rf "$DIR"' EXIT
fail=0
# every verdict is an if/else: a failed write of a PASS line must never print FAIL for an arm that passed (gateexitcheck G2)
ok(){ printf '  PASS  %s\n' "$*" || { fail=1; printf '  FAIL  could not write the PASS line for: %s\n' "$*"; }; return 0; }
no(){ printf '  FAIL  %s\n' "$*"; fail=1; }
# say RC PASSMSG FAILMSG — the verdict of the test just run ($?), as an if/else (gateexitcheck G2)
say(){ if [ "$1" = 0 ]; then ok "$2"; else no "$3"; fi; }

echo "attribmultidefcheck: BIN=$BIN"
[ -x "$BIN" ] || { echo "  FAIL  no ripwire binary at $BIN"; exit 1; }
command -v python3 >/dev/null 2>&1 || { echo "  FAIL  python3 is required (the answers are parsed, not grepped)"; exit 1; }
[ -d "$FIX/cpp" ] || { echo "  FAIL  corpus $FIX missing"; exit 1; }

# ── the answer reader: one canonical line per row, so every arm compares strings ─────────────────────────────────────
# amd.py FILE FORM ROOTTAG ROWKEY
#   FORM xml : ROOTTAG = the root element (callers|callees|impact|safe-delete), ROWKEY = the row element (s|c|rf)
#   FORM json: ROOTTAG unused, ROWKEY = the row array key (callers|callees|calledBy|calls|reach…); FILE may be an MCP
#              JSON-RPC line (its content[0].text is read)
# prints: ROOT ok defs=N groups=G         (G = <def> rows; -1 when the root is absent)
#         DEF  k p bodyless sc
#         ROW  name<TAB>canon<TAB>flags   canon = the of_def tokens mapped to their group's p (sorted), "-" when the row has
#                                         no of_def, "!K" for a token whose K names no <def>; flags = via=name / prov=split
cat >"$DIR/amd.py" <<'PY'
import json, re, sys
path, form, roottag, rowkey = sys.argv[1:5]
raw = open(path, encoding='utf-8', errors='replace').read()
def attrs(tag):
    return dict(re.findall(r'\s([A-Za-z_]+)="([^"]*)"', tag))
def canon(tokens, table):
    out = []
    for t in tokens:
        k, _, mark = t.partition('~')
        p = table.get(k)
        out.append(('!' + k) if p is None else (p + ('~' + mark if mark else '')))
    return ' '.join(sorted(out))
if form == 'mcp':
    try:
        msg = json.loads(raw.strip().splitlines()[-1])
        raw = msg['result']['content'][0]['text']
    except Exception as e:
        print('ROOT missing mcp-unreadable %s' % e); sys.exit(0)
    form = 'json' if raw.lstrip().startswith('{') else 'xml'
if form == 'xml':
    m = re.search(r'<%s\s[^>]*>' % re.escape(roottag), raw)
    if not m:
        print('ROOT missing'); sys.exit(0)
    root = attrs(m.group(0))
    body = raw[m.end():]
    end = body.find('</%s>' % roottag)
    body = body if end < 0 else body[:end]
    table = {}
    defs = re.findall(r'<def\s[^>]*?/?>', body)
    for d in defs:
        a = attrs(d)
        table[a.get('ix', '?')] = a.get('p', '?')
        print('DEF\t%s\t%s\t%s\t%s' % (a.get('ix', '?'), a.get('p', '?'), a.get('bodyless', '0'), a.get('sc', '')))
    print('ROOT\tok\tdefs=%s\tgroups=%d\tto_p=%s\tfrom_p=%s' % (root.get('defs', root.get('to_defs', '?')), len(defs),
          root.get('to_p', '-'), root.get('from_p', '-')))
    for r in re.findall(r'<%s\s[^>]*?/?>' % re.escape(rowkey), body):
        a = attrs(r)
        name = a.get('n', a.get('p', '?'))
        tok = a.get('of_def')
        flags = ' '.join(x for x in (('via=name' if a.get('via') == 'name' else ''), ('prov=split' if a.get('prov') == 'split' else '')) if x)
        print('ROW\t%s\t%s\t%s' % (name, '-' if tok is None else canon(tok.split(), table), flags))
else:
    try:
        j = json.loads(raw)
    except Exception as e:
        print('ROOT missing json-unreadable %s' % e); sys.exit(0)
    tab = j.get('def_table') or []
    table = {}
    for d in tab:
        p = d.get('p') or ('%s:%s' % (d.get('file', '?'), d.get('line', '?')))
        table[str(d.get('ix'))] = p
        print('DEF\t%s\t%s\t%s\t%s' % (d.get('ix'), p, d.get('bodyless', 0), d.get('sc', d.get('scope', ''))))
    print('ROOT\tok\tdefs=%s\tgroups=%d\tto_p=-\tfrom_p=-' % (j.get('defs', '?'), len(tab)))
    for r in j.get(rowkey) or []:
        name = r.get('n', r.get('name', '?'))
        tok = r.get('of_def')
        flags = ' '.join(x for x in (('via=name' if r.get('via') == 'name' else ''), ('prov=split' if r.get('prov') == 'split' else '')) if x)
        print('ROW\t%s\t%s\t%s' % (name, '-' if tok is None else canon([str(t) for t in tok], table), flags))
PY

# ans TAG LANG ROOTTAG ROWKEY FORM -- ARGS… : run BIN on the corpus, read the answer into $DIR/TAG.rows; FAIL (and return 1)
# when the run failed or printed no root, so no absence arm ever reads a crashed or empty run as "not attributed".
ans(){
    local tag="$1" lang="$2" roottag="$3" rowkey="$4" form="$5"; shift 6
    if ! ( cd "$FIX/$lang" && perl -e 'alarm 120; exec @ARGV' "$BIN" . --no-cache "$@" ) >"$DIR/$tag.out" 2>"$DIR/$tag.err"; then
        no "$tag: '$*' on $lang exited non-zero: $( head -2 "$DIR/$tag.err" )"; : >"$DIR/$tag.rows"; return 1
    fi
    python3 "$DIR/amd.py" "$DIR/$tag.out" "$form" "$roottag" "$rowkey" >"$DIR/$tag.rows"
    if ! grep -q '^ROOT	ok' "$DIR/$tag.rows"; then no "$tag: '$*' on $lang printed no <$roottag> root"; return 1; fi
    return 0
}
# mcp TAG LANG ROWKEY TOOL ARGSJSON : the same, through the MCP server (initialize + one tools/call)
mcp(){
    local tag="$1" lang="$2" rowkey="$3" tool="$4" args="$5"
    printf '{"jsonrpc":"2.0","id":0,"method":"initialize","params":{}}\n{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"%s","arguments":{"path":"%s",%s}}}\n' \
        "$tool" "$FIX/$lang" "$args" | perl -e 'alarm 120; exec @ARGV' "$BIN" --mcp 2>"$DIR/$tag.err" | tail -1 >"$DIR/$tag.out"
    python3 "$DIR/amd.py" "$DIR/$tag.out" mcp "$( [ "$tool" = impact ] && echo impact || echo path )" "$rowkey" >"$DIR/$tag.rows"
    if ! grep -q '^ROOT	ok' "$DIR/$tag.rows"; then no "$tag: MCP $tool on $lang gave no answer: $( head -c 200 "$DIR/$tag.out" )"; return 1; fi
    return 0
}
row(){ awk -F'\t' -v n="$2" '$1=="ROW" && $2==n { print $3; exit }' "$DIR/$1.rows"; }
rowflags(){ awk -F'\t' -v n="$2" '$1=="ROW" && $2==n { print $4; exit }' "$DIR/$1.rows"; }
groups(){ awk -F'\t' '$1=="ROOT" { sub("groups=","",$4); print $4; exit }' "$DIR/$1.rows"; }
rootf(){ awk -F'\t' -v f="$2" '$1=="ROOT" { for(i=3;i<=NF;i++) if(index($i,f"=")==1){ print substr($i,length(f)+2); exit } }' "$DIR/$1.rows"; }
nrows(){ grep -c '^ROW' "$DIR/$1.rows"; }

# expect CLASS TAG NAME WANT : the row's canonical attribution equals WANT exactly
expect(){
    local cls="$1" tag="$2" n="$3" want="$4" got
    got="$( row "$tag" "$n" )"
    if [ -z "$got" ]; then no "[$cls] $tag: no row n=\"$n\" (rows: $( grep '^ROW' "$DIR/$tag.rows" | cut -f2 | tr '\n' ' ' ))"
    elif [ "$got" = "$want" ]; then ok "[$cls] $tag: $n -> $want"
    else no "[$cls] $tag: $n attributed '$got', want '$want'"; fi
}
# hedged CLASS TAG NAME ALLOWED : the row has of_def, every token is marked (~n/~s) and names a group in ALLOWED
hedged(){
    local cls="$1" tag="$2" n="$3" allowed="$4" got t bad=""
    got="$( row "$tag" "$n" )"
    if [ -z "$got" ] || [ "$got" = "-" ]; then no "[$cls] $tag: $n carries no of_def ('$got'); a multi-definition row must name or hedge its definition"; return; fi
    for t in $got; do
        case "$t" in *~n|*~s) ;; *) bad="$bad $t(unmarked)" ;; esac
        case " $allowed " in *" ${t%%~*} "*) ;; *) bad="$bad $t(not in: $allowed)" ;; esac
    done
    [ -z "$bad" ]; say $? "[$cls] $tag: $n hedged ($got)" \
        "[$cls] $tag: $n tokens '$got' —$bad"
}
# plain CLASS TAG : one-group answer — no <def> table and no of_def on any row (a read of a successful run)
plain(){
    local cls="$1" tag="$2"
    if [ "$( groups "$tag" )" = 0 ] && ! awk -F'\t' '$1=="ROW" && $3!="-" { f=1 } END { exit !f }' "$DIR/$tag.rows" \
       && [ "$( nrows "$tag" )" -gt 0 ]; then ok "[$cls] $tag: one definition group — no <def> table, no of_def ($( nrows "$tag" ) rows)"
    else no "[$cls] $tag: a one-group answer grew attribution: groups=$( groups "$tag" ), rows: $( grep '^ROW' "$DIR/$tag.rows" | cut -f2,3 | tr '\t\n' '= ' )"; fi
}
# invariant CLASS TAG : every row has of_def, every K resolves to a <def>, and a row with no unmarked token is hedged
invariant(){
    local cls="$1" tag="$2" bad
    bad="$( awk -F'\t' '$1=="ROW" { if ($3=="-") print $2": no of_def"; else if ($3 ~ /!/) print $2": token names no <def> ("$3")" }' "$DIR/$tag.rows" )"
    if [ "$( nrows "$tag" )" -gt 0 ] && [ -z "$bad" ]; then ok "[$cls] $tag: invariant — all $( nrows "$tag" ) rows attributed or hedged, every K in the table"
    else no "[$cls] $tag: invariant broken: $( printf '%s' "$bad" | tr '\n' ';' | cut -c1-300 )"; fi
}

SP=string_piece.h:6; EV=eval_env.h:5

# ── (A) ninja-07 shape, C++ callers: the union names its definition per row ───────────────────────────────────────────
echo "(A) C++ --callers=empty (StringPiece::empty vs EvalString::empty)"
if ans a_cpp cpp callers s xml -- --callers=empty; then
    [ "$( groups a_cpp )" = 2 ]; say $? "[P] a_cpp: two <def> groups listed" \
        "[P] a_cpp: <def> rows=$( groups a_cpp ), want 2"
    expect P a_cpp UsesPiece "$SP"
    expect P a_cpp UsesEval  "$EV"
    expect P a_cpp UsesBoth  "$EV $SP"
    expect P a_cpp ParseDefault "$EV"
    invariant P a_cpp
    sed 's/-->.*//' "$DIR/a_cpp.out" | grep -q 'of_def='; say $? "[P] a_cpp: the legend defines of_def= (present-only clause)" \
        "[P] a_cpp: of_def= is not defined in the answer's legend"
fi
if ans a_cppj cpp - callers json -- --callers=empty --json; then
    expect P a_cppj UsesPiece "$SP"; expect P a_cppj UsesBoth "$EV $SP"
fi
if ( cd "$FIX/cpp" && "$BIN" . --no-cache --callers=empty --format=columnar ) >"$DIR/a_col.out" 2>/dev/null && grep -q '<callers ' "$DIR/a_col.out"; then
    { grep -q '<def ' "$DIR/a_col.out" && grep -qE 'fields="[^"]*of_def' "$DIR/a_col.out"; }; say $? "[P] a_col: columnar carries the <def> table and an of_def column" \
        "[P] a_col: columnar lacks <def> rows or the of_def column: $( grep -oE 'fields="[^"]*"' "$DIR/a_col.out" )"
else no "a_col: columnar --callers=empty failed"; fi
if ans a_sd cpp safe-delete c xml -- --safe-delete=empty; then
    expect P a_sd UsesPiece "$SP"; expect P a_sd UsesEval "$EV"; invariant P a_sd
fi
if mcp a_mcp cpp calledBy find_referencing_symbols '"symbol":"empty"'; then
    expect P a_mcp UsesPiece "$SP"; expect P a_mcp UsesBoth "$EV $SP"; invariant P a_mcp
fi
if mcp a_mcps cpp calledBy find_symbol '"symbol":"empty"'; then
    expect P a_mcps UsesEval "$EV"
fi

# ── (B) ninja-26 shape, C++ callees: a declaration + its definition = ONE group; the split overload arms are hedged ──
echo "(B) C++ --callees=ParseDefault (decl + def; CanonicalizePath overload split)"
if ans b_cpp cpp callees s xml -- --callees=ParseDefault; then
    plain N b_cpp
    for n in CanonicalizePath; do
        n_split="$( awk -F'\t' -v n="$n" '$1=="ROW" && $2==n && $4 ~ /prov=split/' "$DIR/b_cpp.rows" | wc -l | tr -d ' ' )"
        [ "$n_split" = 2 ]; say $? "[P] b_cpp: both CanonicalizePath arms carry prov=\"split\"" \
            "[P] b_cpp: $n_split of 2 CanonicalizePath rows carry prov=\"split\""
    done
    for n in empty ReadPath; do
        [ -z "$( rowflags b_cpp "$n" )" ]; say $? "[N] b_cpp: proven callee $n carries no hedge" \
            "[N] b_cpp: proven callee $n hedged '$( rowflags b_cpp "$n" )'"
    done
fi
if ans b_cpp2 cpp callers s xml -- --callers=CanonicalizePath; then
    hedged P b_cpp2 ParseDefault "util.cc:2 util.cc:6 util.h:3 util.h:4"
    hedged P b_cpp2 CanonicalizePath "util.cc:2 util.cc:6 util.h:3 util.h:4"
fi
if ans b_rp cpp callers s xml -- --callers=ReadPath; then plain N b_rp; fi
if ans b_up cpp callers s xml -- --callers=UsesPiece; then plain N b_up; fi
if ans b_q cpp callers s xml -- --callers=string_piece.h:empty; then plain N b_q; fi

# ── (C) near misses in one C++ file pair ──────────────────────────────────────────────────────────────────────────────
echo "(C) C++ near misses: overloads in one class, nested namespace, template specialisation"
if ans c_put cpp callers s xml -- --callers=put; then
    hedged N c_put PutInt "fmt.cc:2 fmt.cc:3 fmt.h:3 fmt.h:4"
    hedged N c_put PutStr "fmt.cc:2 fmt.cc:3 fmt.h:3 fmt.h:4"
fi
if ans c_run cpp callers s xml -- --callers=run; then
    expect N c_run RunOuter fmt.cc:4
    expect N c_run RunInner fmt.cc:5
    sc4="$( awk -F'\t' '$1=="DEF" && $3=="fmt.cc:4" { print $5 }' "$DIR/c_run.rows" )"
    sc5="$( awk -F'\t' '$1=="DEF" && $3=="fmt.cc:5" { print $5 }' "$DIR/c_run.rows" )"
    { [ -n "$sc4" ] && [ -n "$sc5" ] && [ "$sc4" != "$sc5" ]; }; say $? "[N] c_run: the two run groups carry distinct sc= ($sc4 / $sc5)" \
        "[N] c_run: nested-namespace groups not told apart by sc= ('$sc4' / '$sc5')"
fi
if ans c_get cpp callers s xml -- --callers=get; then
    # GetInt(const Box<int>&) calls the SPECIALISATION's get (fmt.h:18). The resolver binds the primary (fmt.h:14) today:
    # a proven token naming the primary alone is a false attribution; the specialisation, or a hedge, is correct.
    g="$( row c_get GetInt )"
    case "$g" in
        "fmt.h:18") ok "[N] c_get: GetInt -> the Box<int> specialisation" ;;
        ""|"-"|"fmt.h:14") no "[N] c_get: GetInt attributed '$g' (the primary template, or nothing): must name fmt.h:18 or hedge" ;;
        *) case "$g" in *~n|*~s) ok "[N] c_get: GetInt hedged ($g)" ;; *) no "[N] c_get: GetInt attributed '$g'" ;; esac ;;
    esac
    g="$( row c_get GetDouble )"
    case "$g" in
        "fmt.h:14") ok "[N] c_get: GetDouble -> the primary template" ;;
        *~n|*~s) ok "[N] c_get: GetDouble hedged ($g)" ;;
        *) no "[N] c_get: GetDouble attributed '$g' (want fmt.h:14 or a hedge)" ;;
    esac
fi
if ( cd "$FIX/cpp" && "$BIN" . --no-cache --for=empty ) >"$DIR/c_for.out" 2>/dev/null; then
    n_d="$( grep -oE '<d [^>]*n="empty"[^>]*>' "$DIR/c_for.out" | grep -c 'sc="' )"
    [ "$n_d" -ge 2 ]; say $? "[C] c_for: --for=empty serves each definition as its own row with sc= ($n_d)" \
        "[C] c_for: --for=empty rows with sc=: $n_d"
else no "c_for: --for=empty failed"; fi

# ── (D) impact: per-row attribution through the transitive reach, and the file rollup ───────────────────────────────
echo "(D) C++ --impact=empty"
if ans d_imp cpp impact s xml -- --impact=empty; then
    expect P d_imp UsesPiece "$SP"; expect P d_imp UsesEval "$EV"; expect P d_imp UsesBoth "$EV $SP"
    expect P d_imp RunPiece "$SP";  expect P d_imp RunEval "$EV"
    invariant P d_imp
fi
if ans d_rf cpp impact rf xml -- --impact=empty --limit=2; then
    expect P d_rf parser.cc "$EV $SP"
    expect P d_rf eval_user.cc "$EV"
fi
if ans d_impj cpp - reach json -- --impact=empty --json; then
    # the row array key of the JSON dialect is read off the answer, never assumed
    key="$( python3 -c 'import json,sys; j=json.load(open(sys.argv[1])); print(next((k for k,v in j.items() if isinstance(v,list) and v and isinstance(v[0],dict) and "n" in v[0]), "-"))' "$DIR/d_impj.out" 2>/dev/null )"
    python3 "$DIR/amd.py" "$DIR/d_impj.out" json - "$key" >"$DIR/d_impj.rows"
    expect P d_impj RunPiece "$SP"
fi
if mcp d_mcp cpp s impact '"symbol":"empty"'; then expect P d_mcp RunEval "$EV"; fi
if ans d_one cpp impact s xml -- --impact=string_piece.h:empty; then plain N d_one; fi

# ── (E) --path endpoints ──────────────────────────────────────────────────────────────────────────────────────────────
echo "(E) C++ --path endpoints"
if ans e_un cpp path s xml -- --path=Clear,empty; then
    [ "$( rootf e_un to_p )" = "-" ]; say $? "[P] e_un: unreached multi-definition to= carries no to_p" \
        "[P] e_un: reachable=0 with to_defs=2 still claims to_p=\"$( rootf e_un to_p )\""
    [ "$( rootf e_un from_p )" = "eval_env.h:4" ]; say $? "[N] e_un: the single-definition from= keeps from_p" \
        "[N] e_un: from_p='$( rootf e_un from_p )'"
fi
if ans e_re cpp path s xml -- --path=RunPiece,empty; then
    [ "$( rootf e_re to_p )" = "$SP" ]; say $? "[C] e_re: a reached multi-definition to= names the bound one ($SP)" \
        "[C] e_re: to_p='$( rootf e_re to_p )'"
fi
if ans e_sp cpp path s xml -- --path=PutInt,put; then
    # the hop into put is one arm of a split call (Fmt::put(int) vs Fmt::put(const char*)): the row that ends the path says so
    case "$( rowflags e_sp put )" in *prov=split*) ok "[P] e_sp: the split hop into a multi-definition to= carries prov=\"split\"" ;;
        *) no "[P] e_sp: the hop into put is a split arm but carries '$( rowflags e_sp put )'" ;; esac
fi
if mcp e_mcp cpp s path_between '"from":"Clear","to":"empty"'; then
    [ "$( rootf e_mcp to_p )" = "-" ]; say $? "[P] e_mcp: MCP path_between drops the unbound to_p too" \
        "[P] e_mcp: MCP to_p='$( rootf e_mcp to_p )'"
fi

# ── (F) the other languages: nfpm-06 (Go), entity identity in one file (Python, TypeScript, Java), hedged (Rust, ObjC) ──
echo "(F) Go / Python / TypeScript / Java / Rust / ObjC"
if ans f_go go callers s xml -- --callers=Get; then
    expect P f_go Package  "deb/deb.go:7 files/files.go:4"
    expect P f_go Describe "deb/deb.go:7"
    expect P f_go ListOnly "files/files.go:4"
    expect P f_go Stamp    "modtime.go:4"
    [ "$( groups f_go )" = 3 ]; say $? "[P] f_go: three <def> groups" \
        "[P] f_go: <def> rows=$( groups f_go ), want 3"
fi
if mcp f_gomcp go calledBy find_referencing_symbols '"symbol":"Get"'; then expect P f_gomcp Describe "deb/deb.go:7"; fi
if ans f_py py callers s xml -- --callers=render; then
    expect P f_py draw_widget widgets.py:2
    expect P f_py draw_canvas widgets.py:6
    expect P f_py draw_both   "widgets.py:2 widgets.py:6"
    expect P f_py draw_mixed  "widgets.py:2 widgets.py:6~n"
fi
if ans f_ts ts callers s xml -- --callers=area; then
    expect P f_ts circleArea shapes.ts:2; expect P f_ts squareArea shapes.ts:5; expect P f_ts bothAreas "shapes.ts:2 shapes.ts:5"
fi
if ans f_java java callers s xml -- --callers=lookup; then
    expect P f_java fromCache Store.java:2; expect P f_java fromRegistry Store.java:5; expect P f_java fromBoth "Store.java:2 Store.java:5"
fi
if ans f_rs rs callers s xml -- --callers=iter; then
    hedged P f_rs count_items "src/lib.rs:3 src/lib.rs:7"
    hedged P f_rs count_books "src/lib.rs:3 src/lib.rs:7"
    case "$( rowflags f_rs count_items )" in *via=name*) ok "[N] f_rs: the existing via=\"name\" row hedge stays" ;; *) no "[N] f_rs: via=\"name\" lost on count_items" ;; esac
fi
if ans f_objc objc callers s xml -- --callers=area; then
    hedged P f_objc circleArea "Shapes.m:6 Shapes.m:12"
    hedged P f_objc squareArea "Shapes.m:6 Shapes.m:12"
fi
if ans f_py1 py callers s xml -- --callers=draw_widget; then
    # a single definition with no caller: the root, no rows — read the table count only
    [ "$( groups f_py1 )" = 0 ]; say $? "[N] f_py1: single definition, no <def> table" \
        "[N] f_py1: <def> rows on a single definition"
fi

# ── (G) well-formedness and determinism ───────────────────────────────────────────────────────────────────────────────
echo "(G) well-formedness, determinism"
if command -v xmllint >/dev/null 2>&1; then
    for t in a_cpp a_col a_sd b_cpp d_imp d_rf e_un f_go; do
        [ -s "$DIR/$t.out" ] || continue
        xmllint --noout "$DIR/$t.out" 2>/dev/null; say $? "[C] $t: well-formed XML" \
            "[C] $t: malformed XML"
    done
    ( cd "$FIX/cpp" && "$BIN" . --no-cache --callers=empty --legend=full ) >"$DIR/g_full.out" 2>/dev/null && xmllint --noout "$DIR/g_full.out" 2>/dev/null; say $? "[C] g_full: --legend=full well-formed" \
        "[C] g_full: --legend=full malformed or failed"
else
    printf '  SKIP  xml well-formedness (no xmllint)\n'
fi
( cd "$FIX/cpp" && "$BIN" . --no-cache --callers=empty ) >"$DIR/g_det.out" 2>/dev/null
cmp -s "$DIR/g_det.out" "$DIR/a_cpp.out"; say $? "[C] determinism: --callers=empty byte-identical run to run" \
    "[C] --callers=empty differs run to run"

[ "$fail" = 0 ] && echo "ALL PASS" || echo "FAILURES ABOVE"
exit $fail
