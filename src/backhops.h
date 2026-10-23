#pragma once
// backhops.h — HOP1: ONE backward hop provider for a sink, read by three answers (lane hop1).
//
// THE GAP. `--callers=X` already names, one hop at a time, who MAY call X without a graph edge: a <vr via="slot"> row's
// called_by= (a call through a struct field X was stored in, IX-1) and a <dc> row (a declined call whose stored candidate
// list holds X). The answers that join hops — --path's hedged bridge, --path's root next= and --impact's blast radius —
// ignored those rows: tmux `--path=cmdq_fire_command,window_copy_cmd_cancel` printed no chain and `--impact` said
// reaches="0" beside its own slot row, while three `--callers` calls walk the exact chain.
//
// THE CONTRACT. BackwardHops::backwardHops( sink ) returns the sink's backward hops of two kinds, each KNOWN, never guessed:
//   slot / slot-name  the functions the value-ref slot join names as calling through a struct field the sink is stored in
//                     (valuerefindex.h slotCallers, the --callers rows' own rule: same-type tier alone when it has a hit,
//                     else the field-name tier; one per calling function); width = how many functions that row names;
//   declined          the callers of declined calls whose stored candidate list holds the sink (graph.h declinedSitesNaming's
//                     list membership) — only a call with a stored list, not a self-call, and not one that reads as the
//                     builtin method its name spells (DeclinedCrossRule); width = the list's candidate count.
// A consumer applies its own width bound (kBackHopWidth: a wider set is a hedge it does not follow, never a hop).
//
// What it does NOT mean: a hop is NOT a call edge and NOT proven. Every consumer prints it as a hedge (proven="0", via=
// named, the width beside it) and never counts it in reachable= / hops= / reaches=.

#include "graph.h"              // Graph — the declined records and their stored candidate lists
#include "model.h"
#include "nextverb.h"           // nextAttrXml / nextFlag — the ring's continuation
#include "receiverevidence.h"   // ReceiverEvidence::chainOf / classLike — the receiver chain and the class kinds, one test each
#include "serialize.h"          // escapeXml / jsonStr
#include "valuerefindex.h"      // ValueRefIndex, slotCallers — the slot join and its tier rule
#include "valuerefs.h"          // VrRender / vrPath — p= spelled as every other row
#include "infra/Diagnostics.h"  // EXPECTS
#include "infra/sortutil.h"     // svLess

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rw
{

// The widest backward set a consumer follows as a hop: the same 16 the bridge allows a declined fan (pathgaps.h
// kBridgeFanOut reads this). A wider slot or declined set is a hedge a consumer does not follow.
inline constexpr std::size_t kBackHopWidth = 16;

// A member call spelled like a builtin type's method (`ids.join( "," )`, `keys.push( k )`, `d.get( k )`, Ruby `x.to_s`) — the
// builtin-method gate's per-language tables (graph.h BuiltinMethodGate::tableFor) — is far more often the builtin than an in-tree
// candidate the resolver declined between: the acceptance hand-check rejected every such declined site as a way in or a crossing.
inline bool builtinSpelledCall( const Reference& call ) noexcept
{
    const std::span<const std::string_view> table = BuiltinMethodGate::tableFor( call.lang );
    return call.qualifier.empty() && std::binary_search( table.begin(), table.end(), std::string_view( call.calleeName ), sortutil::svLess );
}

// #11 (HOP1 Stage 0, hono `this.router.match( method, path )`): builtinSpelledCall reads the NAME alone, so a declined call on a
// receiver the source TYPES as an in-tree class was dropped as "probably String.prototype.match". DeclinedCrossRule keeps the
// name rule and lifts it for exactly one shape, all of whose facts the index holds:
//   (1) the record is a Ladder decline (graph.h DeclinedSiteKind::Ladder): the resolver's own builtin-method gate ran on this call
//       and ADMITTED in-tree candidates — it did not decline it as the builtin (that is DeclinedSiteKind::Builtin);
//   (2) its stored candidate list is complete: kept, two or more candidates, at most kBackHopWidth;
//   (3) the receiver is ONE field `f` of an object whose class the index knows: the enclosing class's own instance — `this.f`
//       (JS/TS) or `self.f` (Python, Ruby), the language's keyword only (checklist 37: a local named `self` in JS is an ordinary
//       name) — or a parameter or local of the calling function whose class the source states (`app: Hono<E>`, a
//       LocalBindKind::RecvType record, unqualified, one spelling), standing for EVERY in-tree class definition of that name;
//   (4) EVERY such class states f's class (a LocalBindKind::MemberType record, `router!: Router<…>`, `this.f = new Foo()`; one
//       spelling per class, two decide nothing) as the name of an in-tree class-like definition.
// What it does NOT mean: the call is not proven to reach any candidate; it is only no longer read as the builtin. A `.match` on a
// receiver with no stated class (`path.match( re )`), on a field typed `string`, on a field one same-named class types outside
// the tree or not at all, or on a field only a base class states (bases are not walked) stays skipped.
class DeclinedCrossRule
{
public:
    DeclinedCrossRule( const IngestResult& ing, const Graph& g ) : m_ing( ing ), m_g( g ) {}

    // Whether declined record `site` reads as the builtin method its name spells: no crossing or way in goes through it.
    bool builtinSpelled( std::uint32_t site ) const
    {
        EXPECTS( site < m_g.declinedSiteRef.size(), "a declined record index from the graph's own record list" );
        const Reference& r = m_ing.references[ m_g.declinedSiteRef[ site ] ];
        return builtinSpelledCall( r ) && !receiverTypedInTree( site, r );
    }

private:
    struct FieldType
    {
        NodeId           owner = kNoNode;   // the class definition that states the field (checklist 16: the entity, not its name —
                                            // hono's hono-base.ts and hono.ts both define a class `Hono`, each typing `router` its own
                                            // way); in m_recvTypes, the function stating its parameter's or local's class
        std::string_view field;
        std::string_view type;    // "" = spelled two ways: decides nothing
    };
    struct ClassName
    {
        std::string_view name;
        NodeId           id = kNoNode;
    };
    struct ClassSpan
    {
        std::uint32_t fileId = 0;
        std::uint32_t begin  = 0;
        std::uint32_t end    = 0;
        NodeId        id     = kNoNode;
    };

    // The class definition that owns definition `d`: `d` itself when class-like, else the innermost class-like definition of its
    // file whose span holds it (ReceiverEvidence's containment rule); kNoNode when none (a free function, a C++ out-of-line member).
    NodeId ownerOf( NodeId d ) const
    {
        const Symbol& s = m_ing.symbols[ d ];
        if( ReceiverEvidence::classLike( s.kind ) )
        {
            return d;
        }
        const auto       lo   = std::ranges::lower_bound( m_classes, s.fileId, {}, &ClassSpan::fileId );
        const ClassSpan* best = nullptr;
        for( auto it = lo; it != m_classes.end() && it->fileId == s.fileId; ++it )
        {
            if( it->id != d && it->begin <= s.sigStartByte && s.sigStartByte < it->end && ( best == nullptr || it->end - it->begin < best->end - best->begin ) )
            {
                best = &*it;
            }
        }
        return best != nullptr ? best->id : kNoNode;
    }

    static bool selfKeyword( Lang lang, std::string_view root ) noexcept
    {
        switch( lang )
        {
            case Lang::JavaScript: case Lang::TypeScript: return root == "this";
            case Lang::Python:     case Lang::Ruby:       return root == "self";
            default:                                      return false;   // builtinSpelledCall's tables hold these four only
        }
    }

    bool receiverTypedInTree( std::uint32_t site, const Reference& r ) const
    {
        if( m_g.declinedSiteKind[ site ] != DeclinedSiteKind::Ladder )
        {
            return false;   // (1) the gate declined it as the builtin, or another rule declined it
        }
        const std::uint32_t list = m_g.declinedSiteList[ site ];
        if( list == kNoDeclinedList || list + 1 >= m_g.declinedListOff.size() )
        {
            return false;   // (2) no stored list: nothing says the set is complete
        }
        const std::size_t fan = m_g.declinedListOff[ list + 1 ] - m_g.declinedListOff[ list ];
        if( fan < 2 || fan > kBackHopWidth )
        {
            return false;   // (2) a single or too-wide set
        }
        const ReceiverEvidence::Chain ch     = ReceiverEvidence::chainOf( r );
        const NodeId                  caller = m_g.declinedSiteCaller[ site ];
        if( !ch.valid || ch.root.empty() || ch.path.empty() || ch.path.find( '.' ) != std::string_view::npos || caller >= m_ing.symbols.size() )
        {
            return false;   // (3) not one field of a named receiver
        }
        buildOnce();
        std::vector<NodeId> owners;   // (3) the class definitions the receiver object may be an instance of
        if( selfKeyword( r.lang, ch.root ) )
        {
            if( const NodeId own = ownerOf( caller ); own != kNoNode )
            {
                owners.push_back( own );
            }
        }
        else if( const std::string_view cls = statedClassOf( caller, ch.root ); !cls.empty() )
        {
            for( auto it = std::ranges::lower_bound( m_classIds, cls, sortutil::svLess, &ClassName::name ); it != m_classIds.end() && it->name == cls; ++it )
            {
                owners.push_back( it->id );
            }
        }
        return !owners.empty() && std::ranges::all_of( owners, [ & ]( NodeId owner )
        {
            // (4) this class states one in-tree class for the field; stating none (a primitive, a base's field), two spellings, or
            // a class the tree does not define leaves the builtin reading standing
            const auto it = std::ranges::lower_bound( m_fields, std::pair( owner, ch.path ), fieldKeyLess, [ ]( const FieldType& f ) { return std::pair( f.owner, f.field ); } );
            return it != m_fields.end() && it->owner == owner && it->field == ch.path && !it->type.empty()
                && std::ranges::binary_search( m_classNames, it->type, sortutil::svLess );
        } );
    }

    // The class the source states for parameter or local `var` of function `fn` (one unqualified spelling), "" when none or two.
    std::string_view statedClassOf( NodeId fn, std::string_view var ) const
    {
        const auto it = std::ranges::lower_bound( m_recvTypes, std::pair( fn, var ), fieldKeyLess, [ ]( const FieldType& f ) { return std::pair( f.owner, f.field ); } );
        return it != m_recvTypes.end() && it->owner == fn && it->field == var ? it->type : std::string_view();
    }

    static bool fieldKeyLess( const std::pair<NodeId, std::string_view>& a, const std::pair<NodeId, std::string_view>& b ) noexcept
    {
        return a.first != b.first ? a.first < b.first : sortutil::svLess( a.second, b.second );
    }

    // Built on the first receiver shape that needs it: a tree with no such call pays nothing.
    void buildOnce() const
    {
        if( m_built )
        {
            return;
        }
        m_built = true;
        for( NodeId id = 0; id < m_ing.symbols.size(); ++id )
        {
            if( const Symbol& s = m_ing.symbols[ id ]; ReceiverEvidence::classLike( s.kind ) && isDefinitionNotDeclaration( s ) )
            {
                m_classNames.push_back( s.name );
                m_classIds.push_back( ClassName{ s.name, id } );
                m_classes.push_back( ClassSpan{ s.fileId, s.sigStartByte, s.endByte, id } );
            }
        }
        std::ranges::stable_sort( m_classes, {}, &ClassSpan::fileId );
        std::ranges::stable_sort( m_classIds, sortutil::svLess, &ClassName::name );
        std::ranges::sort( m_classNames, sortutil::svLess );
        m_classNames.erase( std::ranges::unique( m_classNames ).begin(), m_classNames.end() );
        for( const Binding& b : m_ing.bindings )
        {
            if( ( b.kind != LocalBindKind::MemberType && b.kind != LocalBindKind::RecvType ) || b.var.empty() || b.typeName.empty() || !b.importedName.empty()
                || b.fromSymbol >= m_ing.symbols.size() )
            {
                continue;   // a qualified spelling (`ext.Router`) names no class of this tree for certain: left out
            }
            if( b.kind == LocalBindKind::RecvType )
            {
                m_recvTypes.push_back( FieldType{ b.fromSymbol, b.var, b.typeName } );   // keyed by the function that states it
            }
            else if( const NodeId owner = ownerOf( b.fromSymbol ); owner != kNoNode )
            {
                m_fields.push_back( FieldType{ owner, b.var, b.typeName } );
            }
        }
        oneSpellingPerKey( m_fields );
        oneSpellingPerKey( m_recvTypes );
    }

    // Sorted by key, one row per (owner, name): two different spellings decide nothing (type "").
    static void oneSpellingPerKey( std::vector<FieldType>& v )
    {
        std::ranges::sort( v, [ ]( const FieldType& a, const FieldType& b )
        {
            if( a.owner != b.owner || a.field != b.field )
            {
                return fieldKeyLess( { a.owner, a.field }, { b.owner, b.field } );
            }
            return sortutil::svLess( a.type, b.type );
        } );
        std::vector<FieldType> merged;
        for( const FieldType& f : v )
        {
            if( !merged.empty() && merged.back().owner == f.owner && merged.back().field == f.field )
            {
                if( merged.back().type != f.type )
                {
                    merged.back().type = {};
                }
                continue;
            }
            merged.push_back( f );
        }
        v = std::move( merged );
    }

    const IngestResult&                    m_ing;
    const Graph&                           m_g;
    mutable bool                           m_built = false;
    mutable std::vector<FieldType>         m_fields;       // (owner, field) → the one stated class, sorted
    mutable std::vector<std::string_view>  m_classNames;   // every in-tree class-like definition name, sorted, distinct
    mutable std::vector<ClassSpan>         m_classes;      // every class-like definition's span, by file (ownerOf)
    mutable std::vector<ClassName>         m_classIds;     // every class-like definition by name (a stated class's definitions)
    mutable std::vector<FieldType>         m_recvTypes;    // (function, parameter or local) → the one stated class, sorted
};

// ── the provider ─────────────────────────────────────────────────────────────────────────────────────────────────────
enum class BackHopVia : std::uint8_t { Slot, SlotName, Declined };

struct BackHop
{
    NodeId        from  = kNoNode;   // the function that may call the sink without a graph edge
    BackHopVia    via   = BackHopVia::Slot;
    std::uint32_t width = 0;         // slot: the functions that row names; declined: the stored candidates
    std::uint32_t ref   = 0;         // the call's reference: the slot call through the field, or the declined call
};

class BackwardHops
{
public:
    BackwardHops( const IngestResult& ing, const Graph& g, const ValueRefIndex& values ) : m_ing( ing ), m_g( g ), m_values( values ), m_rule( ing, g ) {}

    const DeclinedCrossRule& declinedRule() const noexcept { return m_rule; }

    // The sink's backward hops: slot rows first (the stronger tier first per calling function), then declined, each kind by
    // caller id then reference — one row per (from, kind), the narrowest width kept. Memoised per sink.
    std::span<const BackHop> backwardHops( NodeId sink ) const
    {
        if( sink >= m_ing.symbols.size() )
        {
            return {};
        }
        if( const auto it = m_memo.find( sink ); it != m_memo.end() )
        {
            return it->second;
        }
        std::vector<BackHop> out;
        slotHops( sink, out );
        declinedHops( sink, out );
        return m_memo.try_emplace( sink, std::move( out ) ).first->second;
    }

private:
    void slotHops( NodeId sink, std::vector<BackHop>& out ) const
    {
        std::vector<BackHop> slot;
        for( const std::uint32_t r : m_values.valueRefsTo( std::span<const NodeId>( &sink, 1 ) ) )
        {
            if( !m_values.calledBy( r ).empty() )
            {
                continue;   // a container/parameter join answers that row first (valueRefCallerRows' own rule): no slot callers there
            }
            bool                             typed    = false;
            std::uint32_t                    nameAlso = 0;
            const std::vector<std::uint32_t> calls    = slotCallers( m_ing, m_values.slots().callsFor( r ), typed, nameAlso );
            for( const std::uint32_t c : calls )
            {
                const NodeId f = m_ing.references[ c ].fromSymbol;
                if( f != sink && f < m_ing.symbols.size() )   // a call made inside the sink itself is no way in
                {
                    slot.push_back( BackHop{ f, typed ? BackHopVia::Slot : BackHopVia::SlotName, std::uint32_t( calls.size() ), c } );
                }
            }
        }
        keepOnePerCaller( slot );
        out.insert( out.end(), slot.begin(), slot.end() );
    }

    void declinedHops( NodeId sink, std::vector<BackHop>& out ) const
    {
        std::vector<BackHop> dc;
        for( const std::uint32_t site : declinedSitesNaming( m_g, std::span<const NodeId>( &sink, 1 ) ) )
        {
            const std::uint32_t list = m_g.declinedSiteList[ site ];
            const NodeId        from = m_g.declinedSiteCaller[ site ];
            if( list == kNoDeclinedList || list + 1 >= m_g.declinedListOff.size() || from == sink || from >= m_ing.symbols.size() || m_rule.builtinSpelled( site ) )
            {
                continue;   // no stored list, a declined self-call, or a call that reads as the builtin it spells
            }
            dc.push_back( BackHop{ from, BackHopVia::Declined, m_g.declinedListOff[ list + 1 ] - m_g.declinedListOff[ list ], m_g.declinedSiteRef[ site ] } );
        }
        keepOnePerCaller( dc );
        out.insert( out.end(), dc.begin(), dc.end() );
    }

    // One row per calling function: the stronger kind (slot before slot-name), then the narrowest width, then the first call.
    static void keepOnePerCaller( std::vector<BackHop>& v )
    {
        std::ranges::sort( v, [ ]( const BackHop& a, const BackHop& b )
        {
            if( a.from != b.from ) { return a.from < b.from; }
            if( a.via != b.via ) { return a.via < b.via; }
            if( a.width != b.width ) { return a.width < b.width; }
            return a.ref < b.ref;
        } );
        v.erase( std::ranges::unique( v, [ ]( const BackHop& a, const BackHop& b ) { return a.from == b.from; } ).begin(), v.end() );
    }

    const IngestResult&                          m_ing;
    const Graph&                                 m_g;
    const ValueRefIndex&                         m_values;
    DeclinedCrossRule                            m_rule;
    mutable HashMap<NodeId, std::vector<BackHop>> m_memo;
};

inline bool isSlotHop( BackHopVia v ) noexcept
{
    return v == BackHopVia::Slot || v == BackHopVia::SlotName;
}
inline std::string_view backHopViaName( BackHopVia v ) noexcept
{
    return v == BackHopVia::Slot ? "slot" : v == BackHopVia::SlotName ? "slot-name" : "declined";
}

// Whether any of `sinks` has a backward hop a consumer follows (width within kBackHopWidth): --path's root next= swap.
inline bool anyFollowedBackHop( const BackwardHops& hops, std::span<const NodeId> sinks )
{
    return std::ranges::any_of( sinks, [ & ]( NodeId s )
    {
        return std::ranges::any_of( hops.backwardHops( s ), [ ]( const BackHop& h ) { return h.width <= kBackHopWidth; } );
    } );
}

// ── consumer 2: the --impact hedged slot ring ─────────────────────────────────────────────────────────────────────────
// THE GAP. --impact's reaches= walks resolved edges only, so a function stored in a struct field answered reaches="0" beside
// its own <vr via="slot" called_by=> row (the IX-1 census: 226 such answers on tmux + htop).
//
// THE CONTRACT. Ring d=1 is the slot callers the answer's own <vr> rows name (the provider's slot hops of each answered
// definition); ring d=2 is the slot callers of each d=1 function (one more slot hop, the provider again). Two slot hops in all,
// no declined hop (declined_calls= / <dc> already speak for those), no resolved edge walked from a ring member.
// reaches_hedged=N counts the distinct ring functions (d=1 and d=2) that reaches= does not already hold and that are not an
// answered definition; the d=2 rows (<rh>) are listed after the <vrs> rows, kImpactRingRowCap shown, next= the callers of the
// first unshown row's d=1 function. Absent (no attribute, no rows) when the ring is empty: byte-identical.
// What it does NOT mean: a ring function MAY change behaviour through a slot call; it is never a proven caller and never in
// reaches= / by_depth= / radius_*.
inline constexpr std::size_t kImpactRingRowCap = 16;

struct ImpactRingRow
{
    NodeId        fn    = kNoNode;   // the d=2 function: it may call `calls` through a struct field
    NodeId        calls = kNoNode;   // the d=1 function it may call
    BackHopVia    via   = BackHopVia::Slot;
    std::uint32_t ref   = 0;         // its slot call (called_at=)
};

struct ImpactRing
{
    std::size_t                hedged = 0;   // reaches_hedged=
    std::vector<ImpactRingRow> rows;         // d=2, in d=1 order then by name
};

inline ImpactRing impactSlotRing( const IngestResult& ing, const BackwardHops& hops, std::span<const NodeId> seeds, std::span<const NodeId> reach )
{
    ImpactRing           out;
    const std::size_t    S = ing.symbols.size();
    std::vector<char>    known( S, 0 );   // an answered definition or a proven reach member: never counted here
    std::vector<char>    counted( S, 0 );
    for( const NodeId n : seeds ) { if( n < S ) { known[ n ] = 1; } }
    for( const NodeId n : reach ) { if( n < S ) { known[ n ] = 1; } }
    std::vector<NodeId> d1;
    for( const NodeId s : seeds )
    {
        for( const BackHop& h : hops.backwardHops( s ) )
        {
            if( isSlotHop( h.via ) && std::ranges::find( d1, h.from ) == d1.end() )
            {
                d1.push_back( h.from );
            }
        }
    }
    const auto count = [ & ]( NodeId n )
    {
        if( n < S && known[ n ] == 0 && counted[ n ] == 0 )
        {
            counted[ n ] = 1;
            ++out.hedged;
        }
    };
    for( const NodeId f : d1 )
    {
        count( f );
    }
    for( const NodeId f : d1 )
    {
        for( const BackHop& h : hops.backwardHops( f ) )
        {
            if( !isSlotHop( h.via ) || h.from == f || ( h.from < S && known[ h.from ] != 0 ) || std::ranges::find( d1, h.from ) != d1.end() )
            {
                continue;   // a proven member, an answered definition or a d=1 function already speaks for itself
            }
            count( h.from );
            out.rows.push_back( ImpactRingRow{ h.from, f, h.via, h.ref } );
        }
    }
    return out;
}

inline std::string impactRingAttrXml( const ImpactRing& ring )
{
    return ring.hedged == 0 ? std::string() : " reaches_hedged=\"" + std::to_string( ring.hedged ) + "\"";
}
inline std::string impactRingKeyJson( const ImpactRing& ring )
{
    return ring.hedged == 0 ? std::string() : ",\"reaches_hedged\":" + std::to_string( ring.hedged );
}

inline std::string impactRingNext( const IngestResult& ing, const ImpactRing& ring )
{
    const std::size_t shown = std::min( ring.rows.size(), kImpactRingRowCap );
    return shown < ring.rows.size() ? nextFlag( "--callers=", ing.symbols[ ring.rows[ shown ].calls ].name ) : std::string();
}

// <ring d="2" total= shown=[ next=]><rh called_by= called_at= via= calls=/>…</ring>, or "" when there is no d=2 row.
inline std::string impactRingXml( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr )
{
    if( ring.rows.empty() )
    {
        return {};
    }
    std::vector<char> esc;
    const auto        ex    = [ & ]( std::string_view s ) { return std::string( escapeXml( s, esc ) ); };
    const std::size_t shown = std::min( ring.rows.size(), kImpactRingRowCap );
    std::string       out   = "<ring d=\"2\" total=\"" + std::to_string( ring.rows.size() ) + "\" shown=\"" + std::to_string( shown ) + "\"" + nextAttrXml( impactRingNext( ing, ring ) ) + ">";
    for( std::size_t i = 0; i < shown; ++i )
    {
        const ImpactRingRow& row = ring.rows[ i ];
        const Reference&     c   = ing.references[ row.ref ];
        out += "<rh called_by=\"" + ex( ing.symbols[ row.fn ].name ) + "\" called_at=\"" + ex( vrPath( ing, c.fileId, rr ) ) + ":" + std::to_string( c.line )
             + "\" via=\"" + std::string( backHopViaName( row.via ) ) + "\" calls=\"" + ex( ing.symbols[ row.calls ].name ) + "\"/>";
    }
    return out + "</ring>";
}
// The --json twin: `,"ring":{"d":2,"total":N,"shown":M[,"next":…],"rows":[{"called_by":…,"called_at":…,"via":…,"calls":…}]}`, or "".
inline std::string impactRingJson( const IngestResult& ing, const ImpactRing& ring, const VrRender& rr )
{
    if( ring.rows.empty() )
    {
        return {};
    }
    const std::size_t shown = std::min( ring.rows.size(), kImpactRingRowCap );
    std::string       out   = ",\"ring\":{\"d\":2,\"total\":" + std::to_string( ring.rows.size() ) + ",\"shown\":" + std::to_string( shown );
    if( const std::string next = impactRingNext( ing, ring ); !next.empty() )
    {
        out += ",\"next\":\"" + jsonStr( next ) + "\"";
    }
    out += ",\"rows\":[";
    for( std::size_t i = 0; i < shown; ++i )
    {
        const ImpactRingRow& row = ring.rows[ i ];
        const Reference&     c   = ing.references[ row.ref ];
        out += std::string( i == 0 ? "{" : ",{" ) + "\"called_by\":\"" + jsonStr( ing.symbols[ row.fn ].name ) + "\",\"called_at\":\""
             + jsonStr( vrPath( ing, c.fileId, rr ) + ":" + std::to_string( c.line ) ) + "\",\"via\":\"" + std::string( backHopViaName( row.via ) )
             + "\",\"calls\":\"" + jsonStr( ing.symbols[ row.calls ].name ) + "\"}";
    }
    return out + "]}";
}

inline constexpr std::string_view kImpactRingLegend =
    "reaches_hedged=N (absent when 0): N functions MAY reach of= through one or two calls through a struct FIELD (the IX-1 slot "
    "join): the <vr> rows' slot called_by= (one slot hop) and their own slot callers (two), not counted in reaches= and not an "
    "answered definition. A hedge: never a proven caller, never in reaches=/by_depth=/radius_*. ";
inline constexpr std::string_view kImpactRingRowsLegend =
    "<ring d=2 total= shown=> lists the two-hop ones: <rh called_by= called_at= via=slot|slot-name calls=> = called_by may call "
    "calls= through a field at called_at=; first 16, next= the callers of the first unshown calls=. ";
// `on`: reaches_hedged= rides; `rows`: the <ring> element rides too (XML/JSON; the columnar form carries the count alone).
inline std::string impactRingLegend( bool on, bool rows )
{
    return on ? std::string( kImpactRingLegend ) + ( rows ? std::string( kImpactRingRowsLegend ) : std::string() ) : std::string();
}

}   // namespace rw
