#pragma once
// valuerefs.h — the ONE resolution of reference-as-value rows (RefRole::Value / RefRole::Through, captured by
// src/ingest_valuerefs.h), shared by every surface that discloses them: --callers/--callees and their MCP twins
// (callhierarchy.h), --impact, --safe-delete, --dead-code, --uses and --path.
//
// WHAT A ROW IS, AND IS NOT. A <vr> row says a function is USED AS A VALUE at bind= — stored into a table, field or
// variable, or passed as an argument — and names where it lands (into=). It is NOT a call: the call, if any, happens
// wherever the slot is later invoked. called_by= (callers side) and through= (callees side) name a function that MAY
// call through that slot — a called parameter, or a `tbl[k](…)` / `tbl.k(…)` on the same declaration — a clue an
// agent can follow, never a proven call. No count= / reaches= / callers= / impact_reaches= ever includes a row.
//
// MATCHED BY NAME, with the call graph's visibility: a definition in the reference's own file wins; otherwise a
// C/C++ definition with external linkage (a `static` one is reachable only from its own file), a Go definition in
// the same package directory, or a JS/TS/Python definition the file imports by that very name. A file-scope
// non-function declaration of the same name in the reference's file hides every definition elsewhere. Only
// functions and methods are targets (a class used as a value is out of scope).

#include "valuerefindex.h"      // ValueRefIndex, the rows, the --uses filter (the resolution core)
#include "graphlegend.h"       // countFieldOrEmpty — the absent-at-zero count spelling
#include "model.h"
#include "sarif.h"
#include "serialize.h"   // escapeXml / jsonStr

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rw
{

// ── rendering ────────────────────────────────────────────────────────────────────────────────────────────────
struct VrRender
{
    bool             singleRoot = true;
    std::string_view rootPrefix;
};

inline std::string vrPath( const IngestResult& ing, std::uint32_t fileId, const VrRender& rr )
{
    return std::string( rr.singleRoot ? sarif::rootRelativeUri( ing.files[fileId], rr.rootPrefix ) : std::string_view( ing.files[fileId] ) );
}

// The default display window of the <vrs> rows (the runaway guard): value_refs= and total= stay whole, the rows
// beyond it are counted (capped="1") and the next= verb (--uses=SYM) pages every site. It lives beside the two
// renderers that apply it and emit that disclosure (valueRefsXml, valueRefsJson).
inline constexpr std::size_t kValueRefRowCap = 64;

// The <vrs total= shown= capped= [next=]> window and its rows, or "" when there is no row (byte identity).
inline std::string valueRefsXml( const IngestResult& ing, const ValueRefRows& rows, bool callersSide, const VrRender& rr,
                                 std::string_view nextVerb, std::size_t cap = kValueRefRowCap )
{
    if( rows.rows.empty() )
    {
        return {};
    }
    EXPECTS( cap > 0, "a window of zero rows would cut every row while saying shown=0 — the cap is a runaway guard, never a hide" );
    const std::size_t total = rows.rows.size();
    const std::size_t shown = std::min( total, cap );
    std::vector<char> esc;
    const auto        ex = [ & ]( std::string_view s ) { return std::string( escapeXml( s, esc ) ); };
    std::string out = "<vrs total=\"" + std::to_string( total ) + "\" shown=\"" + std::to_string( shown ) + "\" capped=\"" + ( shown < total ? "1" : "0" ) + "\"";
    if( shown < total && !nextVerb.empty() )
    {
        out += " next=\"" + ex( nextVerb ) + "\"";
    }
    out += ">";
    for( std::size_t i = 0; i < shown; ++i )
    {
        const ValueRefRow& row = rows.rows[i];
        const Reference&   r   = ing.references[row.ref];
        out += "<vr";
        if( callersSide )
        {
            if( row.in != kNoNode )
            {
                out += " in_id=\"" + ex( ing.symbols[row.in].name ) + "\"";
            }
        }
        else
        {
            const Symbol& t = ing.symbols[row.to];
            out += " to=\"" + ex( t.name ) + "\" def=\"" + ex( vrPath( ing, t.fileId, rr ) ) + ":" + std::to_string( t.line ) + "\"";
        }
        out += " bind=\"" + ex( vrPath( ing, r.fileId, rr ) ) + ":" + std::to_string( r.line ) + "\"";
        out += " into=\"" + ex( r.fieldName ) + "\"";
        if( callersSide && !row.calledBy.empty() )
        {
            out += " called_by=\"" + ex( row.calledBy ) + "\"";
        }
        if( !callersSide && !row.through.empty() )
        {
            out += " through=\"" + ex( row.through ) + "\"";
        }
        if( !callersSide && row.sites > 1 )
        {
            out += " sites=\"" + std::to_string( row.sites ) + "\"";
        }
        out += "/>";
    }
    out += "</vrs>";
    return out;
}

// The JSON twin: `,"KEY":{"total":N,"shown":M,"capped":0|1[,"next":"…"],"rows":[{…}]}`, or "" when there is no row.
inline std::string valueRefsJson( const IngestResult& ing, const ValueRefRows& rows, bool callersSide, const VrRender& rr,
                                  std::string_view key, std::string_view nextVerb, std::size_t cap = kValueRefRowCap )
{
    if( rows.rows.empty() )
    {
        return {};
    }
    const std::size_t total = rows.rows.size();
    const std::size_t shown = std::min( total, cap );
    std::string out = ",\"" + std::string( key ) + "\":{\"total\":" + std::to_string( total ) + ",\"shown\":" + std::to_string( shown )
                    + ",\"capped\":" + ( shown < total ? "1" : "0" );
    if( shown < total && !nextVerb.empty() )
    {
        out += ",\"next\":\"" + jsonStr( nextVerb ) + "\"";
    }
    out += ",\"rows\":[";
    for( std::size_t i = 0; i < shown; ++i )
    {
        const ValueRefRow& row = rows.rows[i];
        const Reference&   r   = ing.references[row.ref];
        out += i == 0 ? "{" : ",{";
        bool first = true;
        const auto kv = [ & ]( std::string_view k, std::string_view v )
        {
            out += first ? "\"" : ",\"";
            first = false;
            out.append( k );
            out += "\":\"" + jsonStr( v ) + "\"";
        };
        if( callersSide )
        {
            if( row.in != kNoNode )
            {
                kv( "in_id", ing.symbols[row.in].name );
            }
        }
        else
        {
            const Symbol& t = ing.symbols[row.to];
            kv( "to", t.name );
            kv( "def", vrPath( ing, t.fileId, rr ) + ":" + std::to_string( t.line ) );
        }
        kv( "bind", vrPath( ing, r.fileId, rr ) + ":" + std::to_string( r.line ) );
        kv( "into", r.fieldName );
        if( callersSide && !row.calledBy.empty() )
        {
            kv( "called_by", row.calledBy );
        }
        if( !callersSide && !row.through.empty() )
        {
            kv( "through", row.through );
        }
        if( !callersSide && row.sites > 1 )
        {
            kv( "sites", std::to_string( row.sites ) );
        }
        out += "}";
    }
    out += "]}";
    return out;
}

// The depth cut over the answered definitions (ValueRefRows::depthCut, ingest_valuerefs.h kVrMaxDepth): files of their
// language family whose value walk stopped at the cap — value_refs_depth_capped="N" — and the first cut, file:line —
// value_refs_depth_at=. "" when there is none, so an answer over an uncut tree is byte-identical.
inline std::string valueRefsDepthAttrXml( const IngestResult& ing, const ValueRefIndex::DepthCuts& cut, const VrRender& rr )
{
    if( cut.files == 0 )
    {
        return {};
    }
    EXPECTS( cut.first < ing.references.size(), "a depth cut counted over N > 0 files names its first record" );
    const Reference&  r = ing.references[cut.first];
    std::vector<char> esc;
    return " value_refs_depth_capped=\"" + std::to_string( cut.files ) + "\" value_refs_depth_at=\""
         + std::string( escapeXml( vrPath( ing, r.fileId, rr ) + ":" + std::to_string( r.line ), esc ) ) + "\"";
}
inline std::string valueRefsDepthKeyJson( const IngestResult& ing, const ValueRefIndex::DepthCuts& cut, const VrRender& rr )
{
    if( cut.files == 0 )
    {
        return {};
    }
    EXPECTS( cut.first < ing.references.size(), "a depth cut counted over N > 0 files names its first record" );
    const Reference& r = ing.references[cut.first];
    return ",\"value_refs_depth_capped\":" + std::to_string( cut.files ) + ",\"value_refs_depth_at\":\""
         + jsonStr( vrPath( ing, r.fileId, rr ) + ":" + std::to_string( r.line ) ) + "\"";
}

// ── IX-3 name-template dispatch rows ────────────────────────────────────────────────────────────────────
// The callers-side window (--callers / --safe-delete / --impact of a method a template can name): one <pd> per site,
// in reference order, cut at kPatternRowCap (the per-method reach measured at median 1, max 2 on the census trees).
inline constexpr std::size_t kPatternRowCap = 8;
inline std::string patternSiteAttrs( const IngestResult& ing, const Reference& r, const VrRender& rr, std::vector<char>& esc )
{
    const auto ex = [ & ]( std::string_view s ) { return std::string( escapeXml( s, esc ) ); };
    std::string out;
    if( r.fromSymbol < ing.symbols.size() )
    {
        out += " in=\"" + ex( ing.symbols[ r.fromSymbol ].name ) + "\"";
    }
    out += " p=\"" + ex( vrPath( ing, r.fileId, rr ) ) + ":" + std::to_string( r.line ) + "\" pattern=\"" + ex( r.calleeName ) + "\"";
    return out;
}
inline std::string patternDispatchXml( const IngestResult& ing, std::span<const std::uint32_t> sites, const VrRender& rr )
{
    if( sites.empty() )
    {
        return {};
    }
    const std::size_t shown = std::min( sites.size(), kPatternRowCap );
    std::vector<char> esc;
    std::string out = "<pds total=\"" + std::to_string( sites.size() ) + "\" shown=\"" + std::to_string( shown ) + "\">";
    for( std::size_t i = 0; i < shown; ++i )
    {
        out += "<pd" + patternSiteAttrs( ing, ing.references[ sites[i] ], rr, esc ) + "/>";
    }
    return out + "</pds>";
}
// The dispatcher side (--callees of the function that holds the template): one <pd> per site with cands= (every member
// it can name), the first kPatternRowCap of them as <t> rows, and next= (the callers verb on one of them) when cut.
inline std::string patternCalleesXml( const IngestResult& ing, const ValueRefIndex& vri, std::span<const std::uint32_t> sites, const VrRender& rr )
{
    std::string       out;
    std::vector<char> esc;
    const auto        ex = [ & ]( std::string_view s ) { return std::string( escapeXml( s, esc ) ); };
    for( const std::uint32_t i : sites )
    {
        const Reference&          r     = ing.references[i];
        const std::vector<NodeId> cands = vri.patternTargets( i );
        const std::size_t         shown = std::min( cands.size(), kPatternRowCap );
        out += "<pd l=\"" + std::to_string( r.line ) + "\" pattern=\"" + ex( r.calleeName ) + "\" cands=\"" + std::to_string( cands.size() )
             + "\" shown=\"" + std::to_string( shown ) + "\"";
        if( shown < cands.size() )
        {
            out += " next=\"" + ex( "--grep=" + ( r.fieldName.empty() ? r.composeRel : r.fieldName ) ) + "\"";
        }
        out += ">";
        for( std::size_t k = 0; k < shown; ++k )
        {
            const Symbol& t = ing.symbols[ cands[k] ];
            out += "<t n=\"" + ex( t.scope.empty() ? t.name : t.scope + "." + t.name ) + "\" p=\"" + ex( vrPath( ing, t.fileId, rr ) ) + ":"
                 + std::to_string( t.line ) + "\"/>";
        }
        out += "</pd>";
    }
    return out;
}
inline std::string patternDispatchAttrXml( std::size_t n, std::size_t tooWide )
{
    return countFieldOrEmpty( "pattern_dispatch", n, /*json=*/false ) + countFieldOrEmpty( "pattern_too_wide", tooWide, /*json=*/false );
}
// The legend clause, exactly when an answer carries pattern_dispatch= or pattern_too_wide=.
inline const char* patternDispatchLegend( bool any, bool callersSide ) noexcept
{
    if( any && !callersSide )
    {
        return "pattern_dispatch=N (absent when 0): N lookups in this body build a NAME from a template (Python getattr/hasattr/"
               "__dict__.get with an f-string or +, a JS/TS o['p' + x] / o[`p${x}`]); each <pd l= pattern= cands=K shown=> lists the "
               "class members whose names match it (a self/this/cls receiver: the class family only), first 8 as <t n= p=>, next= the "
               "literal scan for the rest. MAY be invoked; matched by name, not a proven call: never in count=. ";
    }
    return any ? "pattern_dispatch=N (absent when 0): N lookups build a NAME from a template (Python getattr/hasattr/__dict__.get with an "
                 "f-string or +, a JS/TS o['p' + x] / o[`p${x}`]) whose literal prefix/suffix this method's name matches; a self/this/cls "
                 "receiver keeps only the class family (ancestors and subclasses). MAY be invoked through that name; matched by name, "
                 "not a proven call: never in callers=/impact_reaches=/count=, and the dead-code list is unchanged. <pds>/<pd in= p= "
                 "pattern=> list them. pattern_too_wide=K: K templates whose literal side is under 3 identifier characters "
                 "(f\"_{m}\") could also name it; no row. "
               : "";
}

inline std::string valueRefsCountAttrXml( std::size_t n ) { return countFieldOrEmpty( "value_refs", n, /*json=*/false ); }
inline std::string valueRefsCountKeyJson( std::size_t n ) { return countFieldOrEmpty( "value_refs", n, /*json=*/true ); }
// The legend clause for the depth attributes, exactly when an answer about these rows carries them.
inline const char* valueRefsDepthLegendFor( const ValueRefRows& rows ) noexcept
{
    return valueRefsDepthLegend( rows.depthCut.files > 0 );
}

}   // namespace rw

