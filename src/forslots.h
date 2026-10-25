#pragma once
// forslots.h — INITIALIZER SLOTS on the shown table rows of a compact-route --for answer (lane peer-s, card FOR-SLOTS).
//
// WHY. A compact --for answer (bodies="0") names a struct/table initializer as one <d> row: its declaration line
// (`const struct window_mode window_copy_mode =`), never the functions it holds. A question whose answer is one of those
// functions (tmux's copy mode: `.command = window_copy_command`) then had the table on screen and the function nowhere, though
// the index already binds it (`--callers=window_copy_command` prints the `<vr … into="window_copy_mode.command">` row).
// Mechanism learned from codegraph's handleExplore (MIT; a constant initializer is a seedable node whose span shows the
// `.field = fn` lines) — no code taken.
//
// WHAT. On a SHOWN row whose symbol is neither a callable nor a class/struct/interface (a variable, a constant: an initializer)
// and that holds at least one function value by the index, the row gains
//   slots="k=fn,…"   up to kForSlotCap pairs, in source order: k = the slot as the --callers/--callees `into=` spells it, less the
//                    row's own name (`.command` → `command`; a positional C entry stays `[0]`; a Python key stays `["run"]`),
//                    fn = the function the index binds there (one pair per distinct target of a by-name lookup), and
//   slots_more="N"   N more pairs it did not list (absent when none was cut).
// A pair is a Value reference (RefRole::Value, src/valuerefindex.h) made INSIDE the row's symbol and resolved to a function:
// binding, never a call, never "relevant". A key or a name a comma or `=` would split is counted in slots_more=, never printed.
//
// NOT ON: a row whose text the answer already carries (a body the fold or the body slot serves holds it, also an enclosing body:
// the `.f = fn` lines are on screen). Same posture as mem= (formembers.h): the compact route, XML, the DEFAULT regime.
// RIPWIRE_NO_INIT_SLOTS (any value) turns it off: the A/B handle.
//
// COST. One pass over the reference table finds the candidate rows' Value references; the value index (the by-name resolution of
// the stored functions) is built only when one is found, so an answer with no initializer row pays one scan.

#include "forfold.h"            // fold_detail::overlapsServed — the fold's own "the answer already carries this text" reading
#include "formembers.h"         // MemberNamesServed — the bodies the answer serves
#include "model.h"
#include "valuerefindex.h"      // ValueRefIndex::targetsOf — the functions a value reference may hold

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rw
{

// How many slot pairs one row lists; the rest is counted in slots_more= (the --callees <vs targets=> cap, kSlotTargetCap).
inline constexpr std::ptrdiff_t kForSlotCap = 16;
static_assert( kForSlotCap > 0, "a table row lists at least one slot pair" );

// The A/B switch: slots ride unless RIPWIRE_NO_INIT_SLOTS is set. The caller adds the posture (compact route, XML, default).
inline bool forInitSlotsOn()
{
    return std::getenv( "RIPWIRE_NO_INIT_SLOTS" ) == nullptr;
}

// The legend clauses, present-only beside the attribute (a row carries slots=). Terse; no "--" (an XML comment); one clause each.
inline constexpr std::string_view kForCompactInitSlotsLegend =
    "; d slots= an initializer's slot=function bindings by the index (source order, not calls), slots_more= pairs cut";
inline constexpr std::string_view kForInitSlotsLegend =
    "; slots= on a variable or constant d row: up to 16 slot=function pairs the initializer binds by the index (the slot as "
    "into= spells it less the row name, source order), a binding not a call, slots_more= the pairs not listed";
static_assert( kForCompactInitSlotsLegend.find( ';', 1 ) == std::string_view::npos && kForInitSlotsLegend.find( ';', 1 ) == std::string_view::npos,
               "one clause each: a ';' inside would split it (the legend's clause separator)" );
static_assert( kForSlotCap == 16, "kForInitSlotsLegend states the cap: move the sentence with the constant" );

namespace slot_detail
{
// The slot key as into= spells it, less the row's own name: `o.run` → `run`, `po[0]` → `[0]`, `T["k"]` → `["k"]`.
inline std::string_view slotKey( std::string_view fieldName, std::string_view owner ) noexcept
{
    if( !owner.empty() && fieldName.size() > owner.size() && fieldName.starts_with( owner ) )
    {
        std::string_view rest = fieldName.substr( owner.size() );
        if( rest.front() == '.' )
        {
            rest.remove_prefix( 1 );
            return rest;
        }
        if( rest.front() == '[' )
        {
            return rest;
        }
    }
    return fieldName;
}
}   // namespace slot_detail

// The slot pairs of one row.
struct InitSlotPairs
{
    std::vector<std::string> pairs;   // "k=fn", source order, at most kForSlotCap
    std::size_t              more = 0;
};

// The pairs the Value references `refs` (all made inside `id`, reference order) bind, by the value index.
inline InitSlotPairs initSlotPairs( const IngestResult& ing, const ValueRefIndex& vri, NodeId id, const std::vector<std::uint32_t>& refs )
{
    InitSlotPairs out;
    std::vector<std::string> seen;
    for( const std::uint32_t v : refs )
    {
        const Reference&       r   = ing.references[v];
        const std::string_view key = slot_detail::slotKey( r.fieldName, ing.symbols[id].name );
        std::vector<NodeId>    tgt = vri.targetsOf( v );
        std::ranges::sort( tgt, [ & ]( NodeId a, NodeId b ) { return ing.symbols[a].name != ing.symbols[b].name ? ing.symbols[a].name < ing.symbols[b].name : a < b; } );
        for( const NodeId t : tgt )
        {
            const std::string_view fn = ing.symbols[t].name;
            std::string pair = std::string( key ) + "=" + std::string( fn );
            if( std::ranges::find( seen, pair ) != seen.end() )
            {
                continue;   // one binding spelled twice (a by-name lookup with two same-named defs prints the name once)
            }
            seen.push_back( pair );
            const bool splits = key.empty() || fn.empty() || key.find_first_of( ",=" ) != std::string_view::npos || fn.find_first_of( ",=" ) != std::string_view::npos;
            if( !splits && std::ssize( out.pairs ) < kForSlotCap )
            {
                out.pairs.push_back( std::move( pair ) );
                continue;
            }
            ++out.more;   // cut by the count, or a pair the list's comma or '=' would split: counted, never silent
        }
    }
    return out;
}

// THE SPLICE: walk the rendered <sigs> rows (each opens `<d l=`, one per `shownIds` entry, in that order) and append
// ` slots="…"[ slots_more="N"]` to each initializer row's head. Returns the bytes added (0: `sigs` untouched). A row count that
// disagrees with `shownIds` splices nothing (the rows are not the ones the ids name).
inline std::size_t spliceInitSlots( std::string& sigs, const IngestResult& ing, const std::vector<NodeId>& shownIds, const MemberNamesServed& served )
{
    // the candidate rows: an initializer kind the answer does not already carry the text of
    std::vector<std::pair<NodeId, std::size_t>> cand;   // (symbol, row index)
    for( std::size_t i = 0; i < shownIds.size(); ++i )
    {
        const NodeId id = shownIds[i];
        // an initializer may sit on a variable, a field or an untyped binding — never a callable, an owner (mem= names those) or a section
        const SymKind k = id < ing.symbols.size() ? ing.symbols[id].kind : SymKind::Section;
        if( ( k == SymKind::Var || k == SymKind::Field || k == SymKind::Other ) && !fold_detail::overlapsServed( ing, id, served.bodies, /*orHolds=*/false ) )
        {
            cand.emplace_back( id, i );
        }
    }
    if( cand.empty() )
    {
        return 0;
    }
    // one scan: the Value references made inside a candidate (the index is built only when there is one)
    std::vector<std::vector<std::uint32_t>> refsOf( cand.size() );
    std::vector<std::pair<NodeId, std::size_t>> byId;   // (symbol, candidate index), sorted: one binary search per reference
    for( std::size_t c = 0; c < cand.size(); ++c )
    {
        byId.emplace_back( cand[c].first, c );
    }
    std::ranges::sort( byId );
    bool any = false;
    for( std::uint32_t i = 0; i < ing.references.size(); ++i )
    {
        const Reference& r = ing.references[i];
        if( r.role != RefRole::Value || r.fromSymbol == kNoNode || r.calleeName.empty() )
        {
            continue;   // a depth-cut record or a slot fact names nothing (name = "")
        }
        for( auto it = std::ranges::lower_bound( byId, std::pair<NodeId, std::size_t>{ r.fromSymbol, 0 } ); it != byId.end() && it->first == r.fromSymbol; ++it )
        {
            refsOf[it->second].push_back( i );
            any = true;
        }
    }
    if( !any )
    {
        return 0;
    }
    constexpr std::string_view kOpen = "<d l=\"";
    std::vector<std::size_t> heads;
    for( std::size_t at = sigs.find( kOpen ); at != std::string::npos; at = sigs.find( kOpen, at + kOpen.size() ) )
    {
        heads.push_back( at );
    }
    if( heads.size() != shownIds.size() )
    {
        DISCLOSE( Diagnostics::answerUnchanged, "no row carries slots=: the answer is the one without initializer slots",
                  "spliceInitSlots: the <sigs> rows and the shown ids disagree" );
        return 0;
    }
    const ValueRefIndex vri( ing );
    std::vector<std::pair<std::size_t, std::string>> inserts;   // (offset of the head's '>', attribute run)
    std::vector<char> esc;
    for( std::size_t c = 0; c < cand.size(); ++c )
    {
        if( refsOf[c].empty() )
        {
            continue;
        }
        const InitSlotPairs sp = initSlotPairs( ing, vri, cand[c].first, refsOf[c] );
        if( sp.pairs.empty() )
        {
            continue;   // nothing resolved to a function (a slots_more= alone would claim pairs it cannot show)
        }
        std::string run = " slots=\"";
        for( std::size_t k = 0; k < sp.pairs.size(); ++k )
        {
            run += k == 0 ? "" : ",";
            run += escapeXml( sp.pairs[k], esc );
        }
        run += "\"";
        if( sp.more > 0 )
        {
            run += " slots_more=\"" + std::to_string( sp.more ) + "\"";
        }
        const std::size_t i     = cand[c].second;
        const std::size_t close = sigs.find( '>', heads[i] );
        ASSUME( close != std::string::npos && ( i + 1 == heads.size() || close < heads[i + 1] ), "a <d> head closes before the next row opens" );
        inserts.emplace_back( close, std::move( run ) );
    }
    std::ranges::sort( inserts, {}, &std::pair<std::size_t, std::string>::first );
    std::size_t added = 0;
    for( auto it = inserts.rbegin(); it != inserts.rend(); ++it )   // back to front: earlier offsets stay valid
    {
        sigs.insert( it->first, it->second );
        added += it->second.size();
    }
    return added;
}

}   // namespace rw
