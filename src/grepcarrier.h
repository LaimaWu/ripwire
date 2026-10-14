#pragma once

// grepcarrier.h — what a grep hit's CARRIER holds and who reads it (L4, train 28).
//
// WHY. A literal often sits in a variable or constant rather than a function: `DRIVER = get_environ( "TEXTUAL_DRIVER" )`,
// or a C initialiser `cmd_split_window_entry = { .name = "split-window", … .exec = cmd_split_window_exec }`. The grep
// <enc> row then names that variable with callers="0" — a call count on a thing nobody calls — and the answer stops one
// hop short of what the reader wanted: the place the value is READ (app.py's `constants.DRIVER`) or the FUNCTION the
// initialiser stores (`.exec = cmd_split_window_exec`). Both facts are already in the index; this header reads them.
//
// THE RULE (labels from the index, never from text):
//   * a carrier is an enclosing definition of kind Var (a variable, constant or initialiser) — the parser's kind;
//   * reads= / reads_at= are the uses verb's role="read" sites for the carrier's NAME (matched by name, the uses verb's own
//     unit), counted only where that verb can see reads at all: an ingest that captured value uses and a language whose
//     reads reach the use-site table (countfloor.h valueUsesArmedFor), never a C-family header, whose readers are `.c`
//     files the uses verb never reads (the countFloorNext rule). Anywhere else the attributes are ABSENT: unknown, never 0;
//   * vals= are the function values the carrier itself stores: value references (valuerefindex.h) made inside the
//     carrier's definition whose name resolves to an indexed function, in source order, as slot=fn@line: the slot key as
//     the value index normalised it, and the line of the ONE definition the name resolves to (@path:line when that
//     definition is in another file) — the hop the reader wants next. A name resolving to several definitions keeps no
//     @: which one the slot holds is not something the index decided.
// The CLI (verbs_grep.h) and the MCP grep twin (mcpverbs.h grepEnclosingJson) both call grepCarriers, so the two
// surfaces cannot disagree about a row.

#include "countfloor.h"      // valueUsesArmedFor — where reads reach the use-site table
#include "filter.h"          // pathTierIndexOver, compareTierThenPath — the uses verb's base row order
#include "model.h"
#include "nextverb.h"        // nextFlag — the root next= spelling
#include "sarif.h"           // rootRelativeUri — the root-relative spelling every grep path takes
#include "search.h"          // GrepEncRow — the rows this decorates
#include "valuerefindex.h"   // ValueRefIndex — madeIn / targetsOf, the stored function values
#include "infra/Diagnostics.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

inline constexpr std::size_t kGrepCarrierShown = 3;   // reads_at= sites and vals= entries printed per row (reads=/vals_total= count all)

struct GrepCarrierSite
{
    std::string   path;       // the READS ingest's own file spelling (the CLI's may be a second, rich ingest of the same root)
    std::uint32_t line = 0;
};

struct GrepCarrierVal
{
    std::string   slot;       // the normalised key without its leading '.', "" for a plain assignment
    std::string   fn;         // the function name as written
    std::string   path;       // the definition's file when it is not the carrier's own, else ""
    std::uint32_t line = 0;   // the definition's line; 0 = the name resolves to several definitions (no @)
};

struct GrepCarrier
{
    bool                         isCarrier  = false;   // every definition on the row is a Var
    bool                         readsKnown = false;   // reads= is a count the uses verb could make (see the rule above)
    std::uint32_t                reads      = 0;       // role="read" sites spelled like the carrier's name
    std::vector<GrepCarrierSite> at;                   // the first kGrepCarrierShown of them, in the uses verb's base order
    std::vector<GrepCarrierVal>  vals;                 // the first kGrepCarrierShown stored function values
    std::uint32_t                valsTotal  = 0;       // every stored function value on the row
};

// Does the uses verb see reads of a definition of `s`'s language at that path? (countfloor.h countFloorNext's rule.)
inline bool grepCarrierReadsVisible( const IngestResult& ing, const Symbol& s ) noexcept
{
    const std::string_view path = s.fileId < ing.files.size() ? std::string_view( ing.files[ s.fileId ] ) : std::string_view();
    return valueUsesArmedFor( s.lang ) && !path.ends_with( ".h" );
}

inline bool isGrepCarrierRow( const IngestResult& ing, const GrepEncRow& row ) noexcept
{
    return !row.ids.empty() && std::ranges::all_of( row.ids, [ & ]( NodeId id ) { return id < ing.symbols.size() && ing.symbols[id].kind == SymKind::Var; } );
}

// Do any rows want reads= — a carrier whose language/path the uses verb reads? The CLI asks this before paying for a
// rich ingest, so an answer with no such row costs nothing extra.
inline bool grepCarriersWantReads( const IngestResult& ing, std::span<const GrepEncRow> rows ) noexcept
{
    return std::ranges::any_of( rows, [ & ]( const GrepEncRow& row )
    {
        return isGrepCarrierRow( ing, row ) && grepCarrierReadsVisible( ing, ing.symbols[ row.ids.front() ] );
    } );
}

namespace grepcarrier
{

// The value-reference sites that replace a read row in the uses verb (valuerefindex.h UsesValueFilter): a Value
// reference spelled like `names[k]` that resolves to a function. Built only when such a reference exists, so the
// common case (a variable name no function shares) never builds the reads ingest's value index.
inline std::vector<std::uint64_t> valueSitesOf( const IngestResult& rd, std::span<const std::string_view> names )
{
    std::vector<std::uint32_t> candidates;
    for( std::uint32_t i = 0; i < rd.references.size(); ++i )
    {
        const Reference& r = rd.references[i];
        if( r.role == RefRole::Value && !r.fieldName.starts_with( '@' ) && std::ranges::binary_search( names, std::string_view( r.calleeName ) ) )
        {
            candidates.push_back( i );
        }
    }
    std::vector<std::uint64_t> sites;
    if( candidates.empty() )
    {
        return sites;
    }
    const ValueRefIndex vri( rd );
    for( const std::uint32_t i : candidates )
    {
        if( !vri.targetsOf( i ).empty() )
        {
            sites.push_back( ( std::uint64_t( rd.references[i].fileId ) << 32 ) | rd.references[i].startByte );
        }
    }
    std::ranges::sort( sites );
    return sites;
}

}   // namespace grepcarrier

// One carrier verdict per row. `ing` is the ingest the rows came from; `vri` its value index (vals=); `reads` the
// ingest whose role="read" references count (nullptr = none captured: reads= stays unknown on every row). The reads
// pass is ONE walk over `reads->references` for every carrier name on the page, never a per-row rescan.
inline std::vector<GrepCarrier> grepCarriers( const IngestResult& ing, const ValueRefIndex& vri, std::span<const GrepEncRow> rows,
                                              const IngestResult* reads )
{
    std::vector<GrepCarrier> out( rows.size() );
    std::vector<NodeId>      carrierIds;     // every carrier definition on the page, for ONE madeIn pass
    std::vector<std::string_view> readNames; // the names whose reads are counted
    for( std::size_t r = 0; r < rows.size(); ++r )
    {
        if( !isGrepCarrierRow( ing, rows[r] ) )
        {
            continue;
        }
        out[r].isCarrier = true;
        carrierIds.insert( carrierIds.end(), rows[r].ids.begin(), rows[r].ids.end() );
        const Symbol& s = ing.symbols[ rows[r].ids.front() ];
        if( reads != nullptr && grepCarrierReadsVisible( ing, s ) )
        {
            out[r].readsKnown = true;
            readNames.push_back( s.name );
        }
    }
    if( carrierIds.empty() )
    {
        return out;
    }

    // vals=: the stored function values, attributed to the row whose definition made them (source order: madeIn is
    // ascending reference order, which is file then byte order within one ingest).
    for( const std::uint32_t v : vri.madeIn( carrierIds, RefRole::Value ) )
    {
        const Reference& ref = ing.references[v];
        if( ref.fieldName.starts_with( '@' ) || vri.targetsOf( v ).empty() )
        {
            continue;   // a decorator row is a fact about a definition; an unresolved name is not a function value
        }
        for( std::size_t r = 0; r < rows.size(); ++r )
        {
            if( !out[r].isCarrier || std::ranges::find( rows[r].ids, ref.fromSymbol ) == rows[r].ids.end() )
            {
                continue;
            }
            ++out[r].valsTotal;
            if( out[r].vals.size() < kGrepCarrierShown )
            {
                const std::string_view key = std::string_view( ref.composeRel ).starts_with( '.' ) ? std::string_view( ref.composeRel ).substr( 1 )
                                                                                                    : std::string_view( ref.composeRel );
                GrepCarrierVal val{ std::string( key ), ref.calleeName, {}, 0 };
                if( const std::vector<NodeId> targets = vri.targetsOf( v ); targets.size() == 1 && targets[0] < ing.symbols.size() )
                {
                    const Symbol& def = ing.symbols[ targets[0] ];
                    val.line = def.line;
                    if( def.fileId != ref.fileId && def.fileId < ing.files.size() )
                    {
                        val.path = ing.files[ def.fileId ];
                    }
                }
                out[r].vals.push_back( std::move( val ) );
            }
            break;   // one row per name (grepEnclosingRows groups by chain): a definition sits on one row
        }
    }

    if( readNames.empty() )
    {
        return out;
    }
    EXPECTS( reads != nullptr, "a name is queued for reads only when a reads ingest was handed in" );
    std::ranges::sort( readNames );
    readNames.erase( std::unique( readNames.begin(), readNames.end() ), readNames.end() );
    const std::vector<std::uint64_t> valueSites = grepcarrier::valueSitesOf( *reads, readNames );

    // ONE pass: the uses verb's read rows (collectUseSites' filters) for every queued name.
    struct ReadHit { std::uint32_t name; std::uint32_t fileId; std::uint32_t line; };
    std::vector<ReadHit> hits;
    for( const Reference& r : reads->references )
    {
        if( r.role != RefRole::Read || r.isCompose || r.isDocLink || r.lang == Lang::Markdown )
        {
            continue;
        }
        const auto it = std::ranges::lower_bound( readNames, std::string_view( r.calleeName ) );
        if( it == readNames.end() || *it != r.calleeName )
        {
            continue;
        }
        if( std::ranges::binary_search( valueSites, ( std::uint64_t( r.fileId ) << 32 ) | r.startByte ) )
        {
            continue;   // the uses verb serves this site as role="value", not a read
        }
        hits.push_back( { std::uint32_t( it - readNames.begin() ), r.fileId, r.line } );
    }
    const std::vector<std::uint8_t> tierOfFile = pathTierIndexOver( *reads, hits, []( const ReadHit& h ) { return h.fileId; } );
    std::ranges::sort( hits, [ & ]( const ReadHit& a, const ReadHit& b )
    {
        if( a.name != b.name )
        {
            return a.name < b.name;
        }
        if( const int c = compareTierThenPath( *reads, tierOfFile, a.fileId, b.fileId ); c != 0 )
        {
            return c < 0;
        }
        return a.line < b.line;
    } );
    for( std::size_t r = 0; r < rows.size(); ++r )
    {
        if( !out[r].readsKnown )
        {
            continue;
        }
        const std::string_view name = ing.symbols[ rows[r].ids.front() ].name;
        const std::uint32_t    k    = std::uint32_t( std::ranges::lower_bound( readNames, name ) - readNames.begin() );
        ASSUME( k < readNames.size() && readNames[k] == name, "every readsKnown row queued its name" );
        const auto [ first, last ] = std::ranges::equal_range( hits, k, {}, &ReadHit::name );   // hits are sorted by name first
        out[r].reads = std::uint32_t( last - first );
        for( auto h = first; h != last && out[r].at.size() < kGrepCarrierShown; ++h )
        {
            out[r].at.push_back( { reads->files[ h->fileId ], h->line } );
        }
    }
    return out;
}

// The root next= on an uncut answer whose TOP hit sits in a carrier with reads= — the uses verb on that carrier's name
// (the spelling floor_next= uses, countfloor.h countFloorNext), because where the value is read is the hop the at verb
// on the hit's own line cannot give. "" otherwise: the at verb on the top hit stays the follow-up. The CLI grep root
// and the MCP grep twin both call it; each caller keeps its cut-answer rule (the next page) ahead of this one.
inline std::string grepCarrierNext( const IngestResult& ing, std::span<const GrepHit> hits, std::span<const GrepEncRow> rows,
                                    std::span<const GrepCarrier> carriers )
{
    if( hits.empty() || hits[0].enclosingId == kNoNode )
    {
        return {};
    }
    for( std::size_t r = 0; r < rows.size() && r < carriers.size(); ++r )
    {
        if( std::ranges::find( rows[r].ids, hits[0].enclosingId ) != rows[r].ids.end() )
        {
            return carriers[r].readsKnown && carriers[r].reads > 0 ? nextFlag( "--uses=", ing.symbols[ hits[0].enclosingId ].name ) : std::string();
        }
    }
    return {};
}

// The two list attributes as plain text items (the emitters escape them for their own dialect): reads_at= as
// path:line, vals= as slot=fn@line / slot=fn@path:line / slot=fn — paths root-relative exactly when the hit rows' are.
struct GrepCarrierText
{
    std::vector<std::string> readsAt;
    std::vector<std::string> vals;
};

inline GrepCarrierText grepCarrierText( const GrepCarrier& c, bool singleRoot, std::string_view rootPrefix )
{
    const auto rel = [ & ]( const std::string& p ) { return std::string( singleRoot ? sarif::rootRelativeUri( p, rootPrefix ) : std::string_view( p ) ); };
    GrepCarrierText t;
    if( c.readsKnown && c.reads > 0 )
    {
        for( const GrepCarrierSite& site : c.at )
        {
            t.readsAt.push_back( rel( site.path ) + ":" + std::to_string( site.line ) );
        }
    }
    for( const GrepCarrierVal& v : c.vals )
    {
        std::string item = v.slot.empty() ? v.fn : v.slot + "=" + v.fn;
        if( v.line != 0 )
        {
            item += "@" + ( v.path.empty() ? std::string() : rel( v.path ) + ":" ) + std::to_string( v.line );
        }
        t.vals.push_back( std::move( item ) );
    }
    return t;
}

}   // namespace rw
