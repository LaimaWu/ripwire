#pragma once
// pathgaps.h — what a reachable="0" path answer's search MET that the call graph holds no edge for (PATH-GAP honesty).
//
// THE DEFECT. --path=A,B (and the MCP path_between twin) answered `reachable="0" hint="no directed call path"` whenever
// the BFS over resolved call edges did not reach B, although that search had walked past calls the graph keeps NO edge
// for: a call the resolver DECLINED (two or more same-language candidates, none local, nothing chose one), a call left
// UNRESOLVED (an in-repo name whose every definition was filtered out), a function handed off as a VALUE (stored or
// passed, then invoked by whoever holds it) and a call made THROUGH a parameter or slot. Any of those can be the hop
// that joins A to B, so "no path" there was a wholeness claim the search could not make.
// A fifth population (train 26b, honesty-small condition 3): a call bound by NAME ALONE (FE-B via="name") has edges only to
// the candidates in reach of the caller, so a same-language, same-kind definition of that name elsewhere (another
// directory) is never searched — the fx/lpy case: run(obj) calls obj.process(), the edge goes to the same-directory
// A.process, and other/B.process, which calls the target, was invisible behind a plain "no directed call path".
//
// THE CONTRACT. pathSearchGaps walks the same directed cone the BFS walked (every symbol reachable from the from=
// definitions over resolved call edges) and counts those five populations inside it. When the count is non-zero the
// answer never says "no directed call path": it says the search was incomplete, gives the counts (searched=, gaps=),
// and names the cone symbols that carry them (<gap> rows, nearest to from= first, capped at kPathGapRows and
// disclosed) with a next= that reads their bodies. When the count is zero the old wording stands, byte for byte.
//
// What it does NOT mean: a <gap> row is not a hop and not evidence that a path exists — it is where the search could
// not see. AMBIGUOUS calls are deliberately NOT a gap: the resolver gives every candidate of a k-way split its own edge
// (graph.h buildGraph's splitPick), and the BFS follows all of them, so ambiguity never hides a path (it can only make
// a FOUND path a guess, which prov="split" / amb= already disclose). Calls to names with no in-repo definition at all
// (Undefined) are not counted either: the graph keeps no per-caller tally of them, and nearly all are library calls.
// Both are floors the legend sentence states. A via="name" call whose every same-language, same-kind definition of the
// name is in the searched cone (its own edge or another cone symbol's) is not a gap either: nothing it could bind to was unsearched.
//
// ONE analysis and ONE emitter for both transports (verbs_navigate.h runPath, mcpverbs.h pathText): the two used to
// carry their own copies of the hint and must not drift on this clause.

#include "graph.h"            // Graph — out-edges (the cone), declinedOut / unresolvedOut (the per-caller gap counts)
#include "model.h"
#include "nextverb.h"         // nextAttrXml — the one pasteable follow-up
#include "sarif.h"            // rootRelativeUri — p= relative to root=, as every other row
#include "serialize.h"        // escapeXml
#include "valuerefindex.h"    // ValueRefIndex::targetsOf — what a value reference may hand off
#include "filter.h"           // pathTierIndexOver — the near-sink rows' tier key within a depth
#include "arch.h"             // wildcardMatch — the ONE `*` match, which a glob endpoint reads
#include "closures.h"         // ClosureIndex — a near-sink row written inside another function's body (why=closure)
#include "infra/sortutil.h"   // svLess: the explicit byte order every string_view sort here takes (portablebuildcheck #6)

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

inline constexpr std::size_t kPathGapRows = 3;   // <gap> rows printed; gap_syms= counts them all, gap_syms_capped="1" says so

// The five gap populations, in the order gaps= lists them. A declarative table (CONTRIBUTING §3): the spelling lives once.
enum class PathGapKind : std::uint8_t { Declined, Unresolved, Value, Through, Name };
inline constexpr std::size_t                                    kPathGapKindCount = 5;
inline constexpr std::array<std::string_view, kPathGapKindCount> kPathGapKindNames = { "declined", "unresolved", "value", "through", "name" };
using PathGapCounts = std::array<std::size_t, kPathGapKindCount>;

struct PathGapSym
{
    NodeId        id    = kNoNode;
    std::uint32_t depth = 0;       // hops from the nearest from= definition
    std::size_t   calls = 0;       // the sum of counts — the row order's second key
    PathGapCounts counts{};
};

struct PathSearchGaps
{
    std::size_t             searched = 0;   // symbols the directed search reached, the from= definitions included
    std::size_t             calls    = 0;   // every gap in the cone: the sum of totals
    PathGapCounts           totals{};
    std::vector<PathGapSym> syms;           // cone symbols carrying >= 1 gap: nearest first, then more gaps, then id
    std::vector<NodeId>     cone;           // every searched symbol, BFS order (searched == cone.size()): the near-sink rows' met=

    bool any() const noexcept { return calls > 0; }
};

// NAME gaps: per cone symbol, the callee names it reaches only by name (an edge whose Graph::outNameOnly bit is set) for
// which the tree holds a same-language, same-kind DEFINITION outside the cone — a namesake the search never walked. A
// namesake inside the cone was searched (another cone symbol reaches it), and every edge target of a cone symbol is in
// the cone, so "outside the cone" also means the call has no edge to it. Counted once per such name per caller. Only the
// names behind the cone's name-only edges are looked up.
inline void countUnlistedNamesakes( const IngestResult& ing, const Graph& g, std::span<const NodeId> cone, std::vector<PathGapCounts>& per )
{
    EXPECTS( g.outNameOnly.size() == g.outTargets.size(), "buildGraph allocates the hedge bits one per flattened edge" );
    struct Key
    {
        Lang             lang;
        SymKind          kind;
        std::string_view name;
    };
    // explicit byte order on the name (never string_view's operator< / <=>: libstdc++'s compare wraps n1 - n2 under
    // -fsanitize=integer, portablebuildcheck #6/#6b); == needs no order
    const auto keyLess = []( const Key& a, const Key& b )
    {
        if( a.lang != b.lang )
        {
            return a.lang < b.lang;
        }
        if( a.kind != b.kind )
        {
            return a.kind < b.kind;
        }
        return sortutil::svLess( a.name, b.name );
    };
    const auto keyEq = []( const Key& a, const Key& b ) { return a.lang == b.lang && a.kind == b.kind && a.name == b.name; };
    const auto keyOf    = [ & ]( NodeId t ) { const Symbol& s = ing.symbols[t]; return Key{ s.lang, s.kind, s.name }; };
    const auto hasHedge = [ & ]( NodeId u ) { return std::any_of( g.outNameOnly.begin() + g.outOff[u], g.outNameOnly.begin() + g.outOff[u + 1], []( std::uint8_t b ) { return b != 0; } ); };
    const auto isSym    = [ & ]( std::uint32_t k ) { return g.outTargets[k] < ing.symbols.size(); };
    std::vector<std::string_view> names;
    for( const NodeId u : cone )
    {
        for( std::uint32_t k = g.outOff[u]; k < g.outOff[u + 1]; ++k )
        {
            if( g.outNameOnly[k] != 0 && isSym( k ) )
            {
                names.push_back( ing.symbols[ g.outTargets[k] ].name );
            }
        }
    }
    if( names.empty() )
    {
        return;
    }
    std::ranges::sort( names, sortutil::svLess );
    names.erase( std::ranges::unique( names ).begin(), names.end() );
    std::vector<char> searched( ing.symbols.size(), 0 );   // the cone, by symbol id
    for( const NodeId u : cone )
    {
        if( u < searched.size() )
        {
            searched[u] = 1;
        }
    }
    std::vector<Key> unsearched;   // the tree's definitions of exactly those names that the search never reached
    for( std::size_t i = 0; i < ing.symbols.size(); ++i )
    {
        const Symbol& s = ing.symbols[i];
        if( searched[i] == 0 && isDefinitionNotDeclaration( s ) && std::ranges::binary_search( names, std::string_view( s.name ), sortutil::svLess ) )
        {
            unsearched.push_back( Key{ s.lang, s.kind, s.name } );
        }
    }
    if( unsearched.empty() )
    {
        return;
    }
    std::ranges::sort( unsearched, keyLess );
    for( const NodeId u : cone )
    {
        if( !hasHedge( u ) )
        {
            continue;
        }
        std::vector<Key> hedged;   // the targets u reached by name alone
        for( std::uint32_t k = g.outOff[u]; k < g.outOff[u + 1]; ++k )
        {
            if( isSym( k ) && g.outNameOnly[k] != 0 )
            {
                hedged.push_back( keyOf( g.outTargets[k] ) );
            }
        }
        std::ranges::sort( hedged, keyLess );
        hedged.erase( std::ranges::unique( hedged, keyEq ).begin(), hedged.end() );
        for( const Key& key : hedged )
        {
            per[u][ std::size_t( PathGapKind::Name ) ] += std::ranges::binary_search( unsearched, key, keyLess ) ? 1u : 0u;
        }
    }
}

// The directed cone of `srcs` (the same out-edges shortestPathAny walks, every one of them — no early stop at a target,
// because this runs only when there was none) and the gap populations inside it. Deterministic: BFS in id-ascending
// out-edge order, a reference pass in index order, and a total order on the rows.
inline PathSearchGaps pathSearchGaps( const IngestResult& ing, const Graph& g, std::span<const NodeId> srcs, const ValueRefIndex& values )
{
    constexpr std::uint32_t kUnseen = std::numeric_limits<std::uint32_t>::max();
    const std::size_t       N       = g.wOutDeg.size();
    EXPECTS( g.outOff.size() == N + 1, "the out-edge CSR has one offset per node plus the end" );
    std::vector<std::uint32_t> depth( N, kUnseen );
    std::vector<NodeId>        queue;
    queue.reserve( 64 );
    for( const NodeId s : srcs )
    {
        if( s < N && depth[s] == kUnseen )
        {
            depth[s] = 0;
            queue.push_back( s );
        }
    }
    for( std::size_t head = 0; head < queue.size(); ++head )
    {
        const NodeId u = queue[head];
        for( std::uint32_t k = g.outOff[u]; k < g.outOff[u + 1]; ++k )
        {
            const NodeId v = g.outTargets[k];
            if( v < N && depth[v] == kUnseen )
            {
                depth[v] = depth[u] + 1;
                queue.push_back( v );
            }
        }
    }

    PathSearchGaps out;
    out.searched = queue.size();
    std::vector<PathGapCounts> per( N );   // only cone entries are ever written
    const auto inCone = [ & ]( NodeId n ) noexcept { return n < N && depth[n] != kUnseen; };
    for( const NodeId u : queue )
    {
        per[u][ std::size_t( PathGapKind::Declined ) ]   = u < g.declinedOut.size() ? g.declinedOut[u] : 0;
        per[u][ std::size_t( PathGapKind::Unresolved ) ] = u < g.unresolvedOut.size() ? g.unresolvedOut[u] : 0;
    }
    for( std::uint32_t i = 0; i < ing.references.size(); ++i )
    {
        const Reference& r = ing.references[i];
        if( !inCone( r.fromSymbol ) )
        {
            continue;
        }
        if( r.role == RefRole::Through )
        {
            ++per[ r.fromSymbol ][ std::size_t( PathGapKind::Through ) ];
        }
        else if( r.role == RefRole::Value )
        {
            // A function handed off as a value is a gap only when the search did not reach it by a call anyway.
            const std::vector<NodeId> targets = values.targetsOf( i );
            if( std::ranges::any_of( targets, [ & ]( NodeId t ) { return t < N && !inCone( t ); } ) )
            {
                ++per[ r.fromSymbol ][ std::size_t( PathGapKind::Value ) ];
            }
        }
    }
    if( !g.outNameOnly.empty() )   // FE-B's hedge bits (absent on a graph built without them: no name population)
    {
        countUnlistedNamesakes( ing, g, queue, per );
    }
    for( const NodeId u : queue )
    {
        std::size_t here = 0;
        for( std::size_t k = 0; k < kPathGapKindCount; ++k )
        {
            here          += per[u][k];
            out.totals[k] += per[u][k];
        }
        if( here > 0 )
        {
            out.calls += here;
            out.syms.push_back( PathGapSym{ u, depth[u], here, per[u] } );
        }
    }
    std::ranges::sort( out.syms, []( const PathGapSym& a, const PathGapSym& b )
    {
        if( a.depth != b.depth )
        {
            return a.depth < b.depth;
        }
        if( a.calls != b.calls )
        {
            return a.calls > b.calls;
        }
        return a.id < b.id;
    } );
    out.cone = std::move( queue );
    ENSURES( out.any() == !out.syms.empty(), "a non-zero total is carried by at least one cone symbol, and only then" );
    ENSURES( out.cone.size() == out.searched, "the cone is exactly the searched symbols" );
    return out;
}

// "kind:N,kind:N" over a per-kind count table — zero entries left out, table order. One spelling for gaps= and why=.
template<std::size_t K>
inline std::string kindCountsValue( const std::array<std::size_t, K>& c, const std::array<std::string_view, K>& names )
{
    std::string v;
    for( std::size_t k = 0; k < K; ++k )
    {
        if( c[k] > 0 )
        {
            v += ( v.empty() ? "" : "," ) + std::string( names[k] ) + ":" + std::to_string( c[k] );
        }
    }
    return v;
}

// gaps="declined:D,unresolved:U,value:V,through:T,name:M" — zero entries left out, table order.
inline std::string pathGapCountsValue( const PathGapCounts& c )
{
    return kindCountsValue( c, kPathGapKindNames );
}

// The legend sentence, carried only by an answer that prints the clause (absent otherwise: byte-identical).
inline constexpr std::string_view kPathGapsLegend =
    "searched=N gaps= gap_syms=: reachable=0 but the search is INCOMPLETE — the N symbols reachable from from= over resolved call edges hold calls the graph "
    "keeps no edge for (gaps= counts them: declined = several candidates and nothing chose one, unresolved = an in-repo name every definition of which was "
    "filtered out, value = a function stored or passed as a value and not otherwise reached, through = a call through a parameter or slot, name = a call "
    "bound by name alone (via=name) whose name the tree also defines, same language and kind, where the call has no edge: that namesake was never "
    "searched), so this is NOT proof that no path exists. <gap t= n= p= gaps=> rows are the symbols that carry them, nearest to from= first, "
    "gap_syms_capped=1 = more than shown; a row is where the search could not see, never a hop. Not counted: ambiguous calls (every candidate has an "
    "edge the search followed) and calls to names defined nowhere in the tree. next= reads the rows' bodies. ";
inline std::string_view pathGapsLegend( bool on ) noexcept
{
    return on ? kPathGapsLegend : std::string_view();
}

struct PathGapsXml
{
    std::string rootAttrs;   // searched= gaps= gap_syms= [gap_syms_capped=] hint= next=, or the plain no-path hint alone
    std::string rows;        // the <gap> rows, the path element's only children when reachable="0" ("" with the plain hint)
};

// The transport's own spellings of the follow-ups, so one function writes both dialects: `connect` is the undirected verb
// ("--connect=A,B" on the CLI, "the connect verb on A,B" on MCP), `usesImpact` the non-call one ("--uses/--impact" /
// "uses/impact"), `tail` the CLI's several-defs clause (plain text, "" on MCP) — exactly as the no-gap hint had them.
struct PathHintSpelling
{
    std::string      connect;
    std::string_view usesImpact;
    std::string_view tail;
};

// The reachable="0" clause: the gap clause when the search met a gap, else the plain hint, byte-identical to before.
// `rootPrefix` empty ⇒ p= stays as indexed (multi-root), exactly as the hop rows spell it.
inline PathGapsXml pathUnreachedXml( const IngestResult& ing, const PathSearchGaps& gaps, bool singleRoot, std::string_view rootPrefix,
                                     const PathHintSpelling& say )
{
    std::vector<char> esc;
    const auto ex = [ & ]( std::string_view s ) -> std::string { return std::string( escapeXml( s, esc ) ); };
    PathGapsXml out;
    if( !gaps.any() )
    {
        out.rootAttrs = " hint=\"no directed call path — try " + ex( say.connect ) + " (undirected: finds a shared caller), or "
                      + std::string( say.usesImpact ) + " for non-call references" + std::string( say.tail ) + "\"";
        return out;
    }
    const std::size_t shown = std::min( gaps.syms.size(), kPathGapRows );
    std::string nextSelectors;   // the comma-joined file:name list; nextFlag quotes it when a shell would split or expand it
    for( std::size_t i = 0; i < shown; ++i )
    {
        const Symbol&          s  = ing.symbols[ gaps.syms[i].id ];
        const std::string_view rp = singleRoot ? sarif::rootRelativeUri( ing.files[ s.fileId ], rootPrefix ) : std::string_view( ing.files[ s.fileId ] );
        out.rows += "<gap t=\"" + std::string( symTag( s.kind ) ) + "\" n=\"" + ex( s.name ) + "\" p=\"" + ex( rp ) + ":" + std::to_string( s.line )
                  + "\" gaps=\"" + pathGapCountsValue( gaps.syms[i].counts ) + "\"/>";
        // file:name, the selector spelling --expand resolves to this one definition (the same p= the row prints)
        nextSelectors += ( i == 0 ? "" : "," ) + std::string( rp ) + ":" + s.name;
    }
    out.rootAttrs = " searched=\"" + std::to_string( gaps.searched ) + "\" gaps=\"" + pathGapCountsValue( gaps.totals ) + "\" gap_syms=\""
                  + std::to_string( gaps.syms.size() ) + "\"" + ( gaps.syms.size() > shown ? " gap_syms_capped=\"1\"" : "" )
                  + " hint=\"no path through resolved call edges, but the search is incomplete: it met calls or hand-offs it could not follow in full (gaps=) — read the gap rows, or try "
                  + ex( say.connect ) + " (undirected: finds a shared caller)" + std::string( say.tail ) + "\"" + nextAttrXml( nextFlag( "--expand=", nextSelectors ) );
    return out;
}

// ── SINK-SIDE FRONTIER (lane ix2-slice, commit 1): the reachable="0" answer's <near_sink> block ──────────────────────────
// THE GAP. The <gap> rows above are FORWARD-only: where the search from from= went blind. A reader closing the gap by hand
// also walks BACK from to=: who calls it, who calls those, and how the last of them is entered when nothing proven calls it.
// htop's `--path=ScreenManager_run,LinuxMachine_scanCPUTime` met a declined call in checkRecalculation, but nothing named
// the platform build Machine_scan (linux/LinuxMachine.c), the sink's only caller — the hop the declined call could have meant.
// (Mechanism learned from a peer's failure diagnosis, which reports the backward frontier beside the forward one; no code copied.)
//
// THE CONTRACT. pathNearSink walks the sink's PROVEN callers back over resolved call edges to depth kNearSinkDepth, and
// classifies, for every symbol it meets (to= itself included), how that symbol may ALSO be entered without an edge:
//   declined  a declined call (several candidates, none chosen) whose candidate list holds it (the callers verb's <dc> rows);
//   value     it is stored or passed as a value, and no call through a parameter or slot is matched to that hand-off;
//   through   it is passed as a value that a call through a parameter or slot is matched to (the invoker is named);
//   name      a call bound by name alone (via=name) went to a same-language, same-kind namesake and has no edge to it;
//   closure   it is a named function written inside another function's body (src/closures.h): that one runs it or hands it
//             on (koa's `const handleResponse = () => respond( ctx )` inside Application.handleRequest);
//   external  none of those and no proven caller either: no indexed way in was found (an entry point, a caller outside
//             the tree, a framework hook, a hand-off the index does not record, such as koa's local `.then( handleResponse )`, or dead).
// Each row names ONE such site (in= its enclosing definition, site= its file:line): a site whose enclosing definition the
// forward search reached comes first (met="1": the search met that hand-off and could not follow it), then by kind, then by
// path and line. The rows: every proven caller at depth 1..kNearSinkDepth, plus a to= definition that carries a reason; the
// ones carrying a reason first (met before not), then depth, then id; kNearSinkRows shown, rows= counts them all.
//
// What it does NOT mean: a row is not a hop of a path and not evidence that one exists; why= says how the row MAY be entered
// that the graph holds no edge for (the classification is a floor: unresolved calls carry no per-name record, and calls to
// names defined nowhere are not counted). met="1" is the closest thing to a bridge this answer offers, and it is a hedge.
// A reachable="1" answer never carries the block (byte-identical); the hint and the forward clause above are unchanged.
inline constexpr std::uint32_t kNearSinkDepth = 2;   // proven caller hops back from to= (1 = a direct caller)
inline constexpr std::size_t   kNearSinkRows  = 4;   // <ns> rows printed; rows= counts them all, rows_capped="1" says so

// The five un-edged reach kinds, in the order why= lists them (external is their absence, with no proven caller). Closure
// (commit 2, src/closures.h): the row is a named function written inside in='s body — its site is its own definition.
enum class NearSinkKind : std::uint8_t { Declined, Value, Through, Name, Closure };
inline constexpr std::size_t                                     kNearSinkKindCount = 5;
inline constexpr std::array<std::string_view, kNearSinkKindCount> kNearSinkKindNames = { "declined", "value", "through", "name", "closure" };
using NearSinkCounts = std::array<std::size_t, kNearSinkKindCount>;

struct NearSinkSite
{
    bool          valid  = false;      // a site was chosen
    std::uint32_t fileId = 0;          // the site's file and line: a reference's, or (closure) the row's own definition
    std::uint32_t line   = 0;
    std::uint32_t order  = 0;          // the last tie-break: the reference index, or (closure) the row id
    NodeId        in     = kNoNode;    // its enclosing definition (kNoNode: file scope)
    NearSinkKind  kind   = NearSinkKind::Declined;
    bool          met    = false;      // the forward search reached `in`
};

struct NearSinkRow
{
    NodeId         id      = kNoNode;
    std::uint32_t  depth   = 0;   // proven call hops back from the nearest to= definition (0 = to= itself)
    std::size_t    callers = 0;   // its proven callers (in-edges), any depth
    NearSinkCounts counts{};
    NearSinkSite   site;          // the one site the row names (none when counts are all 0)

    bool reasoned() const noexcept { return site.valid || callers == 0; }   // a why= (external included)
};

struct PathNearSink
{
    std::size_t              callers = 0;   // the sink's proven callers within kNearSinkDepth hops (to= excluded)
    std::vector<NearSinkRow> rows;          // served order, uncut (the emitter shows kNearSinkRows)
};

// Is site a preferred over site b for one row? met first, then kind (table order), then path, line, order.
inline bool nearSinkSiteBetter( const IngestResult& ing, const NearSinkSite& a, const NearSinkSite& b )
{
    if( !b.valid || !a.valid )
    {
        return a.valid && !b.valid;
    }
    if( a.met != b.met )
    {
        return a.met;
    }
    if( a.kind != b.kind )
    {
        return a.kind < b.kind;
    }
    if( a.fileId != b.fileId )
    {
        return ing.files[ a.fileId ] < ing.files[ b.fileId ];
    }
    return a.line != b.line ? a.line < b.line : a.order < b.order;
}

// The backward frontier of `dstDefs` (see the block comment above). `forwardCone` = PathSearchGaps::cone, the symbols the
// forward search reached. Deterministic: BFS in in-edge order from the to= definitions in the order given, one pass over the
// declined records, the value index in reference order, one pass over the name-only edges, and a total order on the rows.
inline PathNearSink pathNearSink( const IngestResult& ing, const Graph& g, std::span<const NodeId> dstDefs, std::span<const NodeId> forwardCone,
                                  const ValueRefIndex& values )
{
    constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();
    const std::size_t       S     = ing.symbols.size();
    PathNearSink            out;
    std::vector<std::uint32_t> rowOf( S, kNone );   // symbol → its index in out.rows
    std::vector<char>          met( S, 0 );
    for( const NodeId u : forwardCone )
    {
        if( u < S )
        {
            met[u] = 1;
        }
    }
    const auto* ro = g.inEdges.rowOffsets();
    const auto* ci = g.inEdges.colIndices();
    EXPECTS( g.inEdges.rows() >= S, "the in-edge CSR has a row per symbol" );
    for( const NodeId t : dstDefs )
    {
        if( t < S && rowOf[t] == kNone )
        {
            rowOf[t] = std::uint32_t( out.rows.size() );
            out.rows.push_back( NearSinkRow{ t, 0, 0, {}, {} } );
        }
    }
    for( std::size_t head = 0; head < out.rows.size(); ++head )   // BFS: out.rows grows behind `head`, in depth order
    {
        const NodeId        u = out.rows[head].id;
        const std::uint32_t d = out.rows[head].depth;
        for( std::uint32_t k = ro[u]; k < ro[u + 1]; ++k )
        {
            const NodeId c = ci[k];
            if( c >= S || c == u )
            {
                continue;   // a non-symbol node (a doc file) is no caller
            }
            ++out.rows[head].callers;
            if( d < kNearSinkDepth && rowOf[c] == kNone )
            {
                rowOf[c] = std::uint32_t( out.rows.size() );
                out.rows.push_back( NearSinkRow{ c, d + 1, 0, {}, {} } );
            }
        }
    }
    out.callers = out.rows.size() - std::size_t( std::ranges::count_if( out.rows, []( const NearSinkRow& r ) { return r.depth == 0; } ) );
    const auto offerAt = [ & ]( std::uint32_t row, NearSinkKind kind, std::uint32_t fileId, std::uint32_t line, std::uint32_t order, NodeId in )
    {
        NearSinkRow& r = out.rows[row];
        ++r.counts[ std::size_t( kind ) ];
        const NearSinkSite cand{ true, fileId, line, order, in, kind, in < S && met[in] != 0 };
        if( nearSinkSiteBetter( ing, cand, r.site ) )
        {
            r.site = cand;
        }
    };
    const auto offer = [ & ]( std::uint32_t row, NearSinkKind kind, std::uint32_t ref, NodeId in )
    {
        offerAt( row, kind, ing.references[ref].fileId, ing.references[ref].line, ref, in );
    };

    // CLOSURE: a row written inside another function's body is entered from there (or wherever that one hands it on).
    {
        const ClosureIndex closures( ing );
        for( std::uint32_t row = 0; closures.any() && row < out.rows.size(); ++row )
        {
            const NodeId u = out.rows[row].id;
            if( const NodeId parent = closures.parentOf( u ); parent != kNoNode )
            {
                offerAt( row, NearSinkKind::Closure, ing.symbols[u].fileId, ing.symbols[u].line, u, parent );
            }
        }
    }

    // DECLINED: one pass over the stored candidate lists (which rows each list holds), then one over the records.
    std::vector<std::vector<std::uint32_t>> rowsOfList( g.declinedListCallCount.size() );
    for( std::size_t list = 0; list < rowsOfList.size(); ++list )
    {
        for( std::uint32_t slot = g.declinedListOff[list]; slot < g.declinedListOff[list + 1]; ++slot )
        {
            if( const NodeId cand = g.declinedListCand[slot]; cand < S && rowOf[cand] != kNone )
            {
                rowsOfList[list].push_back( rowOf[cand] );
            }
        }
    }
    for( std::uint32_t site = 0; site < g.declinedSiteList.size(); ++site )
    {
        const std::uint32_t list = g.declinedSiteList[site];
        if( list == kNoDeclinedList || list >= rowsOfList.size() )
        {
            continue;
        }
        for( const std::uint32_t row : rowsOfList[list] )
        {
            if( g.declinedSiteCaller[site] != out.rows[row].id )   // a declined self-call is no way in
            {
                offer( row, NearSinkKind::Declined, g.declinedSiteRef[site], g.declinedSiteCaller[site] );
            }
        }
    }

    // VALUE / THROUGH: each value reference to a row; one matched to a call through a parameter or slot is a THROUGH site
    // there (the invoker), else a VALUE site where it is handed off. A reference made inside the row itself is no way in.
    for( std::uint32_t row = 0; row < out.rows.size(); ++row )
    {
        const NodeId u = out.rows[row].id;
        for( const std::uint32_t v : values.valueRefsTo( std::span<const NodeId>( &u, 1 ) ) )
        {
            const Reference& vr = ing.references[v];
            if( vr.fromSymbol == u )
            {
                continue;
            }
            const std::vector<std::uint32_t> throughs = values.throughsFor( v );
            if( throughs.empty() )
            {
                offer( row, NearSinkKind::Value, v, vr.fromSymbol );
            }
            for( const std::uint32_t t : throughs )
            {
                offer( row, NearSinkKind::Through, t, ing.references[t].fromSymbol );
            }
        }
    }

    // NAME: a caller whose call reached a same-language, same-kind namesake of a row by name alone, with no edge to the row.
    // Counted once per (row, caller); the site is that caller's call of the name, found in one reference pass below.
    if( !g.outNameOnly.empty() )
    {
        EXPECTS( g.outNameOnly.size() == g.outTargets.size(), "buildGraph allocates the hedge bits one per flattened edge" );
        struct Key
        {
            Lang             lang;
            SymKind          kind;
            std::string_view name;
            std::uint32_t    row;
        };
        const auto keyLess = []( const Key& a, const Key& b )
        {
            if( a.lang != b.lang )
            {
                return a.lang < b.lang;
            }
            if( a.kind != b.kind )
            {
                return a.kind < b.kind;
            }
            return sortutil::svLess( a.name, b.name );
        };
        std::vector<Key> keys;
        keys.reserve( out.rows.size() );
        for( std::uint32_t row = 0; row < out.rows.size(); ++row )
        {
            const Symbol& s = ing.symbols[ out.rows[row].id ];
            keys.push_back( Key{ s.lang, s.kind, s.name, row } );
        }
        std::ranges::stable_sort( keys, keyLess );
        std::vector<std::pair<NodeId, std::uint32_t>> nameHits;   // (caller, row)
        const std::size_t N = g.outOff.empty() ? 0 : g.outOff.size() - 1;
        for( NodeId c = 0; c < N && c < S; ++c )
        {
            for( std::uint32_t k = g.outOff[c]; k < g.outOff[c + 1]; ++k )
            {
                const NodeId w = g.outTargets[k];
                if( g.outNameOnly[k] == 0 || w >= S )
                {
                    continue;
                }
                const Symbol& ws = ing.symbols[w];
                const Key     probe{ ws.lang, ws.kind, ws.name, 0 };
                for( auto it = std::ranges::lower_bound( keys, probe, keyLess ); it != keys.end() && !keyLess( probe, *it ); ++it )
                {
                    const NodeId u = out.rows[ it->row ].id;
                    const auto   from = g.outTargets.begin() + g.outOff[c];
                    const auto   to   = g.outTargets.begin() + g.outOff[c + 1];
                    if( u != w && u != c && !std::binary_search( from, to, u ) )   // outTargets ascend within a source
                    {
                        nameHits.emplace_back( c, it->row );
                    }
                }
            }
        }
        std::ranges::sort( nameHits );
        nameHits.erase( std::ranges::unique( nameHits ).begin(), nameHits.end() );
        if( !nameHits.empty() )
        {
            // the call site: the caller's first call reference of the row's name (reference order)
            std::vector<std::pair<NodeId, std::uint32_t>> firstCall;   // (caller, ref), sorted by caller then ref
            std::vector<char> isCaller( S, 0 );
            for( const auto& [ c, row ] : nameHits )
            {
                isCaller[c] = 1;
            }
            for( std::uint32_t i = 0; i < ing.references.size(); ++i )
            {
                const Reference& r = ing.references[i];
                if( r.role == RefRole::Call && r.fromSymbol < S && isCaller[ r.fromSymbol ] != 0 )
                {
                    firstCall.emplace_back( r.fromSymbol, i );
                }
            }
            std::ranges::sort( firstCall );
            for( const auto& [ c, row ] : nameHits )
            {
                const std::string_view name = ing.symbols[ out.rows[row].id ].name;
                std::uint32_t          ref  = UINT32_MAX;
                for( auto it = std::ranges::lower_bound( firstCall, std::pair<NodeId, std::uint32_t>( c, 0u ) ); it != firstCall.end() && it->first == c; ++it )
                {
                    if( ing.references[ it->second ].calleeName == name )
                    {
                        ref = it->second;
                        break;
                    }
                }
                if( ref != UINT32_MAX )   // a hedged edge is always some call reference's; none found ⇒ no row claim
                {
                    offer( row, NearSinkKind::Name, ref, c );
                }
            }
        }
    }

    // ix2-slice commit 4: within a depth, the path TIER (source, then test/bench, then docs) before id — a glob to= (on_*) makes
    // hundreds of depth-0 rows, and the docs examples must not lead them
    const std::vector<std::uint8_t> tierOfFile = pathTierIndexOver( ing, out.rows, [ & ]( const NearSinkRow& r ) { return ing.symbols[ r.id ].fileId; } );
    std::ranges::sort( out.rows, [ & ]( const NearSinkRow& a, const NearSinkRow& b )
    {
        if( a.reasoned() != b.reasoned() )
        {
            return a.reasoned();
        }
        if( a.site.met != b.site.met )
        {
            return a.site.met;
        }
        if( a.depth != b.depth )
        {
            return a.depth < b.depth;
        }
        const std::uint8_t ta = tierOfFile[ ing.symbols[ a.id ].fileId ];
        const std::uint8_t tb = tierOfFile[ ing.symbols[ b.id ].fileId ];
        return ta != tb ? ta < tb : a.id < b.id;
    } );
    // a to= definition with proven callers and no other way in is to_p= itself, not a frontier row; a to= DECLARATION (a
    // prototype: no body) is no frontier either — it is entered through the definition it declares, whose callers it shares
    std::erase_if( out.rows, [ & ]( const NearSinkRow& r ) { return r.depth == 0 && ( !r.reasoned() || !isDefinitionNotDeclaration( ing.symbols[ r.id ] ) ); } );
    ENSURES( out.rows.size() >= out.callers, "every proven caller within the depth is a row" );
    return out;
}

inline constexpr std::string_view kNearSinkLegend =
    "<near_sink callers= rows=>: the SINK-SIDE frontier — to='s proven callers walked back over resolved call edges up to depth= hops "
    "(callers= counts them), each <ns d= t= n= p=> row with callers= its own proven callers (absent at 0) and why= how it may ALSO be "
    "entered with no edge: declined = a declined call lists it as a candidate, value = it is stored or passed as a value, through = a "
    "call through a parameter or slot is matched to that value, name = a by-name call went to a namesake and has no edge to it, "
    "closure = it is a named function written inside in='s body (site= its own definition: in= runs it or hands it on), "
    "external = none of those and no proven caller: no indexed way in was found (an entry point, an out-of-tree or framework caller, "
    "a hand-off the index does not record, or dead); in= site= name one such site (its enclosing definition, file:line), met=1 when the forward search reached in= (it met that "
    "hand-off and could not follow it). A row is where the backward search could not see, never a hop and not proof a path exists; "
    "rows= counts them, rows_capped=1 = more than shown; next= on the block lists the first row's callers and declined sites. ";
inline std::string_view nearSinkLegend( bool on ) noexcept
{
    return on ? kNearSinkLegend : std::string_view();
}

// The <near_sink> block (empty string when there is no row). `rootPrefix` empty ⇒ p= stays as indexed (multi-root).
inline std::string pathNearSinkXml( const IngestResult& ing, const PathNearSink& ns, bool singleRoot, std::string_view rootPrefix )
{
    if( ns.rows.empty() )
    {
        return {};
    }
    std::vector<char> esc;
    const auto ex  = [ & ]( std::string_view s ) -> std::string { return std::string( escapeXml( s, esc ) ); };
    const auto rel = [ & ]( std::uint32_t fileId ) -> std::string_view
    { return singleRoot ? sarif::rootRelativeUri( ing.files[ fileId ], rootPrefix ) : std::string_view( ing.files[ fileId ] ); };
    const std::size_t shown = std::min( ns.rows.size(), kNearSinkRows );
    std::string       rows;
    for( std::size_t i = 0; i < shown; ++i )
    {
        const NearSinkRow& r   = ns.rows[i];
        const Symbol&      s   = ing.symbols[ r.id ];
        // why="declined:D,value:V,through:T,name:M" (table order), "external" with no proven caller and no other way in,
        // nothing with proven callers only
        const std::string  counted = kindCountsValue( r.counts, kNearSinkKindNames );
        const std::string  why     = counted.empty() && r.callers == 0 ? std::string( "external" ) : counted;
        rows += "<ns d=\"" + std::to_string( r.depth ) + "\" t=\"" + std::string( symTag( s.kind ) ) + "\" n=\"" + ex( s.name ) + "\" p=\""
              + ex( rel( s.fileId ) ) + ":" + std::to_string( s.line ) + "\"" + ( r.callers > 0 ? " callers=\"" + std::to_string( r.callers ) + "\"" : std::string() )
              + ( why.empty() ? std::string() : " why=\"" + why + "\"" );
        if( r.site.valid )
        {
            rows += ( r.site.in < ing.symbols.size() ? " in=\"" + ex( ing.symbols[ r.site.in ].name ) + "\"" : std::string() ) + " site=\""
                  + ex( rel( r.site.fileId ) ) + ":" + std::to_string( r.site.line ) + "\"" + ( r.site.met ? " met=\"1\"" : "" );
        }
        rows += "/>";
    }
    const Symbol& first = ing.symbols[ ns.rows.front().id ];
    return "<near_sink depth=\"" + std::to_string( kNearSinkDepth ) + "\" callers=\"" + std::to_string( ns.callers ) + "\" rows=\""
         + std::to_string( ns.rows.size() ) + "\"" + ( ns.rows.size() > shown ? " rows_capped=\"1\"" : "" )
         + nextAttrXml( nextFlag( "--callers=", std::string( rel( first.fileId ) ) + ":" + first.name ) ) + ">" + rows + "</near_sink>";
}

// ── GLOB ENDPOINT (lane ix2-slice, commit 4: IX-2 P4) ────────────────────────────────────────────────────────────────
// `--path=post_message,on_*` answered rc=1 "endpoint not found": the reader asking how a message reaches its handlers names
// the handler FAMILY, not one of its 279 members. An endpoint that names no definition and holds a `*` (and no file:
// qualifier) now stands for every function or method DEFINITION whose name fits it (`*` = any run of characters, possibly
// empty), so the one multi-target search answers it; from_glob="1"/to_glob="1" on the root says which endpoint was a
// pattern, and from_defs=/to_defs= count its matches. A name that does resolve is never read as a pattern.
// The definitions a path endpoint names: resolveAllByNameQualified, else — for a bare `*` pattern — every function or method
// definition whose name fits it, in id order. `*glob` says which happened.
inline std::vector<NodeId> resolvePathEndpoint( const IngestResult& ing, std::string_view spec, std::size_t* unprovenDefs, bool* glob )
{
    *glob                   = false;
    std::vector<NodeId> out = resolveAllByNameQualified( ing, spec, unprovenDefs );
    if( !out.empty() || spec.find( '*' ) == std::string_view::npos || spec.find( ':' ) != std::string_view::npos )
    {
        return out;
    }
    for( NodeId id = 0; id < ing.symbols.size(); ++id )
    {
        const Symbol& s = ing.symbols[id];
        if( ( s.kind == SymKind::Function || s.kind == SymKind::Method ) && isDefinitionNotDeclaration( s ) && wildcardMatch( s.name, spec ) )
        {
            out.push_back( id );
        }
    }
    *glob = !out.empty();
    return out;
}

inline constexpr std::string_view kPathGlobLegend =
    "from_glob=1 / to_glob=1: that endpoint held a * and named no definition, so it stands for every function or method definition "
    "whose name fits the pattern (* = any run of characters); from_defs=/to_defs= count them. ";
inline std::string_view pathGlobLegend( bool on ) noexcept
{
    return on ? kPathGlobLegend : std::string_view();
}
inline std::string pathGlobAttrXml( bool fromGlob, bool toGlob )
{
    return std::string( fromGlob ? " from_glob=\"1\"" : "" ) + ( toGlob ? " to_glob=\"1\"" : "" );
}

// ── HEDGED PATH BRIDGE (lane ix2-slice, commit 4: IX-2 P1) ─────────────────────────────────────────────────────────
// THE GAP. The gap rows say where the forward search went blind and <near_sink> where the backward one did, but neither
// joins them: on htop the reader still has to see that checkRecalculation's declined `Machine_scan()` could mean the Linux
// build, whose callee is the sink. A human crosses exactly those hops.
//
// THE CONTRACT. Only on a reachable="0" answer, a second search may CROSS hops the graph holds no edge for — at most
// kBridgeCrossings of them, each fanning out to at most kBridgeFanOut candidates (a wider one is not followed):
//   declined  from a symbol to each candidate of a declined call it makes (the call's stored candidate list);
//   value     from a symbol to each function it stores or passes as a value (ValueRefIndex::targetsOf);
//   closure   from a function to each named function written in its body (src/closures.h);
//   namesake  from a symbol whose call was bound by name alone to one definition, to the name's other same-language,
//             same-kind definitions (the namesakes the proven search never walked).
// Layer by layer — every symbol reachable with k crossings over resolved edges before any with k+1 — so the chain printed
// has the FEWEST crossings, then is the first found in deterministic (id-ascending) order. <bridge proven="0" crossings=
// hops=> lists it as <s> rows like a path's, the crossing into a row named by via= (and cands= its fan-out).
//
// What it does NOT mean: a bridge is NOT a path — reachable= stays 0, hops= stays 0, and no count includes it. It is the
// closest chain the index can show, every crossing a hedge. Absent (with no clause) when no chain exists within the bounds.
inline constexpr std::uint32_t kBridgeCrossings = 2;
inline constexpr std::size_t   kBridgeFanOut    = 16;
inline constexpr std::size_t   kBridgeMaxHops   = 8;   // a longer chain is no answer a reader would follow: none is printed

enum class BridgeVia : std::uint8_t { Edge, Declined, Value, Closure, Namesake };
inline constexpr std::array<std::string_view, 5> kBridgeViaNames = { "", "declined", "value", "closure", "namesake" };

struct PathBridge
{
    std::vector<NodeId>        hops;       // from= definition … to= definition
    std::vector<BridgeVia>     via;        // parallel: how the hop INTO each row was taken (Edge: a resolved call edge)
    std::vector<std::uint32_t> fan;        // parallel: a crossing's candidate count (0 on an edge)
    std::size_t                crossings = 0;

    bool found() const noexcept { return !hops.empty(); }
};

inline PathBridge pathBridge( const IngestResult& ing, const Graph& g, std::span<const NodeId> srcDefs, std::span<const NodeId> dstDefs,
                              const ValueRefIndex& values )
{
    constexpr std::uint32_t kUnseen = std::numeric_limits<std::uint32_t>::max();
    const std::size_t       S       = ing.symbols.size();
    PathBridge              out;
    if( S == 0 || g.outOff.size() < S + 1 )
    {
        return out;   // no graph over these symbols (an empty tree): no chain to show
    }
    // the crossings out of each symbol, as (from, index) pairs sorted by from: declined records, value references, closures
    std::vector<std::pair<NodeId, std::uint32_t>> declinedBy, valueBy, closureOf;
    for( std::uint32_t site = 0; site < g.declinedSiteList.size(); ++site )
    {
        declinedBy.emplace_back( g.declinedSiteCaller[site], site );
    }
    for( std::uint32_t i = 0; i < ing.references.size(); ++i )
    {
        if( ing.references[i].role == RefRole::Value && ing.references[i].fromSymbol < S )
        {
            valueBy.emplace_back( ing.references[i].fromSymbol, i );
        }
    }
    const ClosureIndex closures( ing );
    for( NodeId id = 0; closures.any() && id < S; ++id )
    {
        if( const NodeId p = closures.parentOf( id ); p != kNoNode )
        {
            closureOf.emplace_back( p, id );
        }
    }
    std::ranges::sort( declinedBy );
    std::ranges::sort( valueBy );
    std::ranges::sort( closureOf );
    // namesakes: the definitions by (lang, kind, name), id within
    std::vector<NodeId> defsByKey;
    for( NodeId id = 0; id < S; ++id )
    {
        if( isDefinitionNotDeclaration( ing.symbols[id] ) )
        {
            defsByKey.push_back( id );
        }
    }
    const auto keyLess = [ & ]( NodeId a, NodeId b )
    {
        const Symbol& x = ing.symbols[a];
        const Symbol& y = ing.symbols[b];
        if( x.lang != y.lang )
        {
            return x.lang < y.lang;
        }
        if( x.kind != y.kind )
        {
            return x.kind < y.kind;
        }
        return x.name != y.name ? sortutil::svLess( x.name, y.name ) : a < b;
    };
    std::ranges::sort( defsByKey, keyLess );
    const auto sameKey = [ & ]( NodeId a, NodeId b )
    { const Symbol& x = ing.symbols[a]; const Symbol& y = ing.symbols[b]; return x.lang == y.lang && x.kind == y.kind && x.name == y.name; };

    std::vector<std::uint32_t> layer( S, kUnseen );
    std::vector<NodeId>        parent( S, kNoNode );
    std::vector<BridgeVia>     viaOf( S, BridgeVia::Edge );
    std::vector<std::uint32_t> fanOf( S, 0 );
    std::vector<char>          isDst( S, 0 );
    // a chain may not route THROUGH a de-prioritized symbol — a test, bench, demo or generated file, or a minified bundle
    // (filter.h's rank tiers) — only start or end at one: the runtime path a reader asks for runs through the code itself
    const std::vector<float>   tierMul = rankTierSymbolMultipliers( ing );
    const auto                 routable = [ & ]( NodeId v ) { return isDst[v] != 0 || tierMul[v] >= 1.f; };
    for( const NodeId t : dstDefs )
    {
        if( t < S )
        {
            isDst[t] = 1;
        }
    }
    std::vector<NodeId> frontier;
    for( const NodeId s : srcDefs )
    {
        if( s < S && layer[s] == kUnseen )
        {
            layer[s] = 0;
            frontier.push_back( s );
        }
    }
    NodeId hit = kNoNode;
    for( std::uint32_t k = 0; k <= kBridgeCrossings && hit == kNoNode && !frontier.empty(); ++k )
    {
        // every symbol reachable from this layer's seeds over resolved call edges joins the layer
        std::vector<NodeId> queue = std::move( frontier );
        frontier.clear();
        for( std::size_t head = 0; head < queue.size() && hit == kNoNode; ++head )
        {
            const NodeId u = queue[head];
            if( isDst[u] != 0 )
            {
                hit = u;
                break;
            }
            for( std::uint32_t e = g.outOff[u]; e < g.outOff[u + 1]; ++e )
            {
                if( const NodeId v = g.outTargets[e]; v < S && layer[v] == kUnseen && routable( v ) )
                {
                    layer[v]  = k;
                    parent[v] = u;
                    queue.push_back( v );
                }
            }
        }
        if( hit != kNoNode || k == kBridgeCrossings )
        {
            break;
        }
        // one crossing out of this layer seeds the next
        const auto cross = [ & ]( NodeId from, NodeId to, BridgeVia via, std::size_t fan )
        {
            if( to < S && layer[to] == kUnseen && routable( to ) )
            {
                layer[to]  = k + 1;
                parent[to] = from;
                viaOf[to]  = via;
                fanOf[to]  = std::uint32_t( fan );
                frontier.push_back( to );
            }
        };
        for( const NodeId u : queue )
        {
            const std::pair<NodeId, std::uint32_t> lo( u, 0u ), hi( u, std::numeric_limits<std::uint32_t>::max() );
            for( auto it = std::ranges::lower_bound( declinedBy, lo ); it != declinedBy.end() && *it <= hi; ++it )
            {
                const std::uint32_t list = g.declinedSiteList[ it->second ];
                if( list == kNoDeclinedList || list + 1 >= g.declinedListOff.size() )
                {
                    continue;   // a call with no stored candidate list names nothing to cross to
                }
                const std::size_t fan = g.declinedListOff[list + 1] - g.declinedListOff[list];
                for( std::uint32_t slot = g.declinedListOff[list]; fan <= kBridgeFanOut && slot < g.declinedListOff[list + 1]; ++slot )
                {
                    cross( u, g.declinedListCand[slot], BridgeVia::Declined, fan );
                }
            }
            for( auto it = std::ranges::lower_bound( valueBy, lo ); it != valueBy.end() && *it <= hi; ++it )
            {
                const std::vector<NodeId> targets = values.targetsOf( it->second );
                for( std::size_t t = 0; targets.size() <= kBridgeFanOut && t < targets.size(); ++t )
                {
                    cross( u, targets[t], BridgeVia::Value, targets.size() );
                }
            }
            for( auto it = std::ranges::lower_bound( closureOf, lo ); it != closureOf.end() && *it <= hi; ++it )
            {
                cross( u, it->second, BridgeVia::Closure, 1 );
            }
            for( std::uint32_t e = g.outOff[u]; e < g.outOff[u + 1] && e < g.outNameOnly.size(); ++e )
            {
                const NodeId w = g.outTargets[e];
                if( g.outNameOnly[e] == 0 || w >= S )
                {
                    continue;
                }
                auto first = std::ranges::lower_bound( defsByKey, w, [ & ]( NodeId a, NodeId b ) { return sameKey( a, b ) ? false : keyLess( a, b ); } );
                auto last  = first;
                while( last != defsByKey.end() && sameKey( *last, w ) )
                {
                    ++last;
                }
                const std::size_t fan = std::size_t( last - first ) - ( std::find( first, last, w ) != last ? 1u : 0u );
                for( auto it = first; fan <= kBridgeFanOut && it != last; ++it )
                {
                    if( *it != w )
                    {
                        cross( u, *it, BridgeVia::Namesake, fan );
                    }
                }
            }
        }
        std::ranges::sort( frontier );   // the next layer's seeds in id order: the walk below them is order-independent of how they were met
    }
    if( hit == kNoNode )
    {
        return out;
    }
    for( NodeId n = hit; n != kNoNode; n = parent[n] )
    {
        out.hops.push_back( n );
        out.via.push_back( viaOf[n] );
        out.fan.push_back( fanOf[n] );
        out.crossings += viaOf[n] != BridgeVia::Edge ? 1u : 0u;
    }
    std::ranges::reverse( out.hops );
    std::ranges::reverse( out.via );
    std::ranges::reverse( out.fan );
    if( out.hops.size() > kBridgeMaxHops + 1 )
    {
        return PathBridge{};   // past the hop bound: the legend says a bridge is shown only within it
    }
    ENSURES( out.crossings >= 1 && out.crossings <= kBridgeCrossings, "only reached when the proven search was not (reachable=0), within the bound" );
    return out;
}

inline constexpr std::string_view kPathBridgeLegend =
    "<bridge proven=\"0\" crossings= hops=>: a HEDGED chain from= to to= of at most 8 hops that crosses at most 2 the graph holds no "
    "edge for, each fanning out to at most 16 candidates, and runs through no test, bench, demo, generated or minified symbol; its <s> rows read like a path's, and via= on a row names the crossing INTO it: declined = "
    "one of cands= candidates of a declined call in the previous row, value = a function the previous row stores or passes as a value, "
    "closure = a function written in the previous row's body, namesake = another same-language definition of a name the previous row "
    "calls by name alone (via=name rows are proven edges bound by name). Fewest crossings first. It is NOT a path: reachable= and "
    "hops= on the root stay 0, no count includes it, and every crossing is a hedge. ";
inline std::string_view pathBridgeLegend( bool on ) noexcept
{
    return on ? kPathBridgeLegend : std::string_view();
}

inline std::string pathBridgeXml( const IngestResult& ing, const Graph& g, const PathBridge& br, bool singleRoot, std::string_view rootPrefix )
{
    if( !br.found() )
    {
        return {};
    }
    std::vector<char> esc;
    const auto        ex = [ & ]( std::string_view s ) -> std::string { return std::string( escapeXml( s, esc ) ); };
    std::string       out = "<bridge proven=\"0\" crossings=\"" + std::to_string( br.crossings ) + "\" hops=\"" + std::to_string( br.hops.size() - 1 ) + "\">";
    for( std::size_t i = 0; i < br.hops.size(); ++i )
    {
        const Symbol&          s  = ing.symbols[ br.hops[i] ];
        const std::string_view rp = singleRoot ? sarif::rootRelativeUri( ing.files[ s.fileId ], rootPrefix ) : std::string_view( ing.files[ s.fileId ] );
        out += "<s t=\"" + std::string( symTag( s.kind ) ) + "\" n=\"" + ex( s.name ) + "\" p=\"" + ex( rp ) + ":" + std::to_string( s.line ) + "\"";
        if( br.via[i] != BridgeVia::Edge )
        {
            out += " via=\"" + std::string( kBridgeViaNames[ std::size_t( br.via[i] ) ] ) + "\" cands=\"" + std::to_string( br.fan[i] ) + "\"";
        }
        else if( i > 0 )
        {
            out += viaNameAttr( edgeNameOnly( g, br.hops[i - 1], br.hops[i] ) );   // FE-B: a proven hop bound by name says so
        }
        out += "/>";
    }
    return out + "</bridge>";
}

}   // namespace rw
