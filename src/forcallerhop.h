#pragma once
// forcallerhop.h — THE CALLER HOP of a name-exact --for answer (lane mf1-nx): when the route names ONE function, its callers.
//
// WHY. A --for question whose subject is a bare function name routes name-exact and serves that function: its signature row,
// its body and the body's callee signatures (<calls>). A change-propagation question ("if X gains a parameter, which functions
// must change?") needs the other direction, and the answer stopped at the definition: on the MF1-DEV rows the name-exact
// change-propagation answers held 3 of 16 required items, while the callers verb on the same symbol reached them. Measured
// offline before this code (reports/mf1-nx.md Stage 0): appending the callers verb's rows lifted 4 of the 7 name-exact
// function rows by >= 1 required item under Rule C, and none of the 163 non-sealed round-5 rows routes name-exact at all.
//
// WHAT. <callers of= shown= total= capped= d2= d2_shown= next=> after the file tail and before the bodies (a prefix-budgeted
// reader meets the callers before the body CDATA): the function's 1-hop callers — the callers verb's own rows
// (callhierarchy.h oneHopCallerRows: every definition's in-edges, tier/path order, ranked before the cap), each <s t= n= sc=
// p=file:line> with its call sites (sites_at=, the callers verb's CALLSITE-AT pass) and via="name" when the edge was matched by
// name alone — then the callers of the SHOWN callers as d="2" rows (the same helper one hop further, by rank, at most
// kForCallerHopDepth2Rows). shown=/total= count the first hop, d2_shown=/d2= the second; capped="1" when either was cut, and
// next= is the whole caller radius (--impact) either way. Counts are floors: resolved call edges only, as the callers verb says.
//
// WHEN. The name-exact route, the auto <bodies> bundle, and its anchor-restricted body candidates are ONE function: every
// candidate a function or method with the same name and scope (a C++ declaration and its out-of-line definition are one
// function). The DEFAULT regime and the XML dialect only, as the body fold: an explicit --token-budget, --json,
// --signatures-only, --detail=N and every other route carry none (byte-identical to before). MCP for is not changed.
// RIPWIRE_NO_CALLER_HOP (any value) turns it off — the A/B handle, the RIPWIRE_NO_BODY_FOLD precedent.

#include "callhierarchy.h"   // oneHopCallerRows — the callers verb's rows; rowCallSites / callSitesAttrXml via editcheck.h
#include "defattrib.h"       // attributeOneHopAnswer — via="name" on a row, as the callers verb decides it
#include "graph.h"           // viaNameAttr
#include "model.h"
#include "nextverb.h"        // nextFlag / nextAttrXml
#include "receiverevidence.h"   // ReceiverEvidence::callableKind — the one function-or-method test
#include "sarif.h"           // rootRelativeUri
#include "serialize.h"       // escapeXml
#include "valuerefs.h"       // VrRender

#include <cstddef>
#include <cstdlib>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

// The first hop's shown rows: a runaway guard over a fan-in, not a size target — every row it leaves out is counted (total=,
// capped="1") and next= serves them. 16 rows of ~60-110 B: a change-propagation answer lists every caller in the common case.
inline constexpr std::size_t kForCallerHopRows = 16;
// The second hop's shown rows (callers of the shown callers, by rank): measured on Stage 0, the rows that carried a required item
// (the entry point two hops up) were within the first four.
inline constexpr std::size_t kForCallerHopDepth2Rows = 4;

// The A/B switch: the hop rides unless RIPWIRE_NO_CALLER_HOP is set. The caller adds the posture (name-exact route, XML, default regime).
inline bool forCallerHopOn()
{
    return std::getenv( "RIPWIRE_NO_CALLER_HOP" ) == nullptr;
}

// The function the hop is about: the body candidates when they are ONE function — non-empty, every one a function or method
// with the first one's name AND scope (the entity, not a namesake in another class) — else empty. The candidates are the
// anchor-restricted auto-body head (verbs_for.h computeAutoBodyCandidateIds), so a name-exact answer bodying two different
// symbols, or a class, a variable or a doc section, carries no hop.
inline std::vector<NodeId> forCallerHopTargets( const IngestResult& ing, std::span<const NodeId> bodyCandidates )
{
    if( bodyCandidates.empty() )
    {
        return {};
    }
    const Symbol& first = ing.symbols[ bodyCandidates.front() ];
    for( const NodeId id : bodyCandidates )
    {
        const Symbol& s = ing.symbols[id];
        if( !ReceiverEvidence::callableKind( s.kind ) || s.name != first.name || s.scope != first.scope )
        {
            return {};
        }
    }
    return std::vector<NodeId>( bodyCandidates.begin(), bodyCandidates.end() );
}

// The hop's legend clauses, present-only beside the element: the full dialect's and the compact dialect's. No "--": they ride
// inside an XML comment.
inline constexpr std::string_view kForCallerHopLegend =
    "; callers of=: the route named one function, so its callers follow its definition: s t= n= sc= p=file:line rows are its "
    "1-hop callers (sites_at= their calls of it as file:line, sites_total= when that list was cut; via=name: matched by name "
    "alone, the receiver unproven), shown= of total=; then d=2 rows, callers of the shown callers by rank, d2_shown= of d2=; "
    "capped=1 when either hop was cut; counts are floors (resolved calls only); next= lists the whole caller radius";
inline constexpr std::string_view kForCompactCallerHopLegend =
    "; callers of=: the one function's 1-hop callers, s t= n= sc= p=file:line (sites_at= its call sites, sites_total= if cut; "
    "via=name by name only), shown= of total=; d=2 rows: their callers, d2_shown= of d2=; capped=1 if cut; floors; next= the radius";

struct CallerHopRender
{
    std::string xml;   // <callers …>…</callers>; empty when there is no target or the render failed
};

// The rows' shared rendering inputs: the index, the single-root run's own root (empty: p= keeps ingest's spelling) and the escape buffer.
struct CallerHopRowCtx
{
    const IngestResult& ing;
    std::string_view    rootArg;
    std::vector<char>&  esc;
};

// One <s> row: kind, name, scope (when it has one), the root-relative definition site, then the row's extra attributes.
inline void appendCallerHopRow( std::string& x, const CallerHopRowCtx& c, NodeId id, std::string_view extra )
{
    const IngestResult&    ing = c.ing;
    std::vector<char>&     esc = c.esc;
    const Symbol&          s   = ing.symbols[id];
    const std::string_view rp  = c.rootArg.empty() ? std::string_view( ing.files[ s.fileId ] ) : sarif::rootRelativeUri( ing.files[ s.fileId ], c.rootArg );
    x += "<s t=\"";
    x += symTag( s.kind );
    x += "\" n=\"";
    x += escapeXml( s.name, esc );
    if( !s.scope.empty() )
    {
        x += "\" sc=\"";
        x += escapeXml( s.scope, esc );
    }
    x += "\" p=\"";
    x += escapeXml( rp, esc );
    x += ':';
    x += std::to_string( s.line );
    x += '"';
    x += extra;
    x += "/>";
}

// The second hop: the callers of the shown first-hop rows that are neither a target nor a first-hop row, in oneHopCallerRows'
// order (tier, path, rank before the cap). Every first-hop row is excluded, shown or not: it is already counted in total=.
inline std::vector<NodeId> callerHopSecondRows( const IngestResult& ing, const Graph& g, std::span<const NodeId> targets,
                                                std::span<const NodeId> firstAll, std::span<const NodeId> firstShown )
{
    std::vector<char> known( ing.symbols.size(), 0 );
    for( const NodeId id : targets )
    {
        known[id] = 1;
    }
    for( const NodeId id : firstAll )
    {
        known[id] = 1;
    }
    std::vector<NodeId> out;
    for( const NodeId id : oneHopCallerRows( ing, g, firstShown ) )
    {
        if( !known[id] )
        {
            out.push_back( id );
        }
    }
    return out;
}

// The element. `rootArg` is the single-root run's own root (empty under multi-root: p= keeps ingest's spelling), as every --for row.
inline CallerHopRender renderForCallerHop( const IngestResult& ing, const Graph& g, std::span<const NodeId> targets, std::string_view rootArg )
{
    CallerHopRender out;
    if( targets.empty() )
    {
        return out;
    }
    const std::vector<NodeId>     first      = oneHopCallerRows( ing, g, targets );
    const std::size_t             firstShown = std::min( first.size(), kForCallerHopRows );
    const std::span<const NodeId> shown      = std::span<const NodeId>( first ).first( firstShown );
    const std::vector<NodeId>     second     = callerHopSecondRows( ing, g, targets, first, shown );
    const std::size_t             secondShown = std::min( second.size(), kForCallerHopDepth2Rows );

    // the callers verb's per-row readings over the shown rows: the call sites (CALLSITE-AT) and the by-name hedge (FE-B)
    const RowCallSites       sites = rowCallSites( ing, targets, shown, VrRender{ !rootArg.empty(), rootArg }, g.keptForwards );
    const OneHopAttribution  attr  = attributeOneHopAnswer( ing, g, targets, shown, /*towardMatches=*/true );

    const Symbol&     t   = ing.symbols[ targets.front() ];
    const std::string sel = t.scope.empty() ? t.name : t.scope + "::" + t.name;   // the callers verb's own qualified selector
    std::vector<char>     esc;
    const CallerHopRowCtx rc{ ing, rootArg, esc };
    std::string&          x = out.xml;
    x.reserve( 160 + ( firstShown + secondShown ) * 112 );
    x += "<callers of=\"";
    x += escapeXml( sel, esc );
    x += "\" shown=\"" + std::to_string( firstShown ) + "\" total=\"" + std::to_string( first.size() ) + "\" capped=\"";
    x += ( firstShown < first.size() || secondShown < second.size() ) ? "1" : "0";
    x += "\"";
    if( !second.empty() )
    {
        x += " d2=\"" + std::to_string( second.size() ) + "\" d2_shown=\"" + std::to_string( secondShown ) + "\"";
    }
    x += nextAttrXml( nextFlag( "--impact=", sel ) );
    x += '>';
    for( std::size_t i = 0; i < firstShown; ++i )
    {
        const std::string extra = std::string( viaNameAttr( attr.nameOnly( i ) ) ) + callSitesAttrXml( sites.tokens[i], sites.totals[i] );
        appendCallerHopRow( x, rc, shown[i], extra );
    }
    for( std::size_t i = 0; i < secondShown; ++i )
    {
        appendCallerHopRow( x, rc, second[i], " d=\"2\"" );
    }
    x += "</callers>";
    return out;
}

}   // namespace rw
