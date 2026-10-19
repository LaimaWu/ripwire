#pragma once
// ifacedispatch.h — lane L6: a call through an INTERFACE-, TRAIT- or ABSTRACT-typed receiver names the implementors' methods
// that can run there (test/ifacedispatchcheck.sh; reports/gap-ideas.md "L6 iface-typed dispatch rows").
//
// A programmer facing `pkg.Package( info, f )` on an interface value lists the implementors and opens the ones that can run.
// The resolver's name tiers answered by NAME instead: every in-tree definition called `Package`, including a same-named
// interface's implementor elsewhere and an overload of another arity. This tier sits in buildGraph's resolve loop after the
// receiver rules and before the name tiers. When the receiver's recorded type T (ReceiverEvidence::dispatchReceiverOf) is an
// interface or trait, or a class declaring the callee without a body, the candidates are the bodied methods of that name the
// types implementing T own (the extends / implements / `impl T for X` edges, walked down to every depth), plus T's own bodied
// default. One candidate reached through proven edges only is an edge, rendered via="impl" — the only implementor in the tree
// (one outside it may exist); otherwise the call is DECLINED with exactly that list (DeclinedSiteKind::Iface, gate="iface").
// No candidate: the tier stays out of the way, and the name tiers answer as before.
//
// The rule reads only the implementors map and the receiver's type, so it is language-neutral; what each language supplies
// is the receiver typing (receiverevidence.h) and, for Rust, the langfacts.h column saying its trait signatures are not
// indexed. C++ keeps its own class identity (resolve.h IdentityNarrower's dispatch split), which graph.h hands to the same
// decision.

#include "model.h"
#include "langfacts.h"
#include "receiverevidence.h"
#include "smallvec.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

// lane lego-transitive fix 1: how an extends/implements edge base→derived was bound (Graph::implementorBound).
//   Proven — the language's own lookup names THIS base: a same-file declaration the clause's enclosing scopes declare
//            (ingest_relations.h localBaseBinding), the file an import binds the name (or its qualifier) to, an included
//            C-family header, the package directory (Java/Kotlin/Go), Ruby's constant scope (RubyBaseScope).
//   ByName — no such evidence, or it named two candidates: the edge rests on the spelling alone.
//   Other  — the evidence names a DIFFERENT same-named type: this edge is the name collision, not a subtype.
// The values order the merge of two references drawing the same edge: the smaller wins (a proven site proves the edge).
enum class BaseBound : std::uint8_t { Proven = 0, ByName = 1, Other = 2 };

// One tier answer: the candidates (ascending ids) and whether a single one is PROVEN (every implements edge to its owner proven).
struct IfaceDispatchAnswer
{
    bool                fired  = false;
    bool                proven = false;
    std::vector<NodeId> cand;
};

class IfaceDispatch
{
public:
    IfaceDispatch( const IngestResult& ing, const ReceiverEvidence& recvEv, const std::vector<std::vector<NodeId>>& implementors,
                   const std::vector<std::vector<std::uint8_t>>& bound )
        : ing_( ing ), recvEv_( recvEv ), implementors_( implementors ), bound_( bound )
    {
        EXPECTS( bound.empty() || bound.size() == implementors.size(), "buildGraph sizes the bound map with the implementors map" );
        for( const Symbol& s : ing.symbols )   // the bodyless members: an interface's or abstract class's contract
        {
            if( ReceiverEvidence::callableKind( s.kind ) && s.endByte <= s.sigEndByte && s.id < recvEv.ownerClass.size() && !recvEv.ownerClass[ s.id ].empty() )
            {
                contract_.try_emplace( recvEv.ownerClass[ s.id ] + "::" + s.name, char( 1 ) );
            }
        }
    }

    // The tier's answer for a call named `callee` on a receiver typed `type`: fired = false when T is no dispatch type for
    // the callee or no candidate is in the tree.
    IfaceDispatchAnswer answer( NodeId type, std::string_view callee ) const
    {
        IfaceDispatchAnswer out;
        if( type == kNoNode || type >= ing_.symbols.size() || !dispatchesThrough( type, callee ) )
        {
            return out;
        }
        const std::vector<Row>& rows = closureOf( type );
        if( rows.empty() )
        {
            return out;   // no implementor in the tree: the name tiers answer, as before
        }
        bool provenAll = true;
        collect( type, callee, out.cand );   // T's own bodied default runs for every implementor without an override
        for( const Row& row : rows )
        {
            provenAll = provenAll && row.proven;
            collect( row.id, callee, out.cand );
        }
        std::sort( out.cand.begin(), out.cand.end() );
        out.cand.erase( std::unique( out.cand.begin(), out.cand.end() ), out.cand.end() );
        out.fired  = !out.cand.empty();
        out.proven = provenAll;
        return out;
    }

private:
    struct Row
    {
        NodeId id     = kNoNode;
        bool   proven = false;   // every implements edge from T down to it is proven
    };

    // T dispatches a call of `callee`: an interface / trait whose contract (its members, or a base's) names the callee — or
    // any trait of a language whose trait signatures are not indexed — or a class whose contract declares it without a body
    bool dispatchesThrough( NodeId type, std::string_view callee ) const
    {
        const Symbol& t = ing_.symbols[ type ];
        if( t.kind == SymKind::Interface && langFacts( t.lang ).contractSignaturesUnindexed )
        {
            return true;
        }
        if( !ReceiverEvidence::classLike( t.kind ) )
        {
            return false;
        }
        const bool iface = t.kind == SymKind::Interface;
        return recvEv_.walkBaseLevels( t.name, [ & ]( const std::vector<std::string_view>& level, std::size_t )
        {
            for( std::string_view c : level )
            {
                key_.assign( c ).append( "::" ).append( callee );
                if( contract_.contains( key_ ) || ( iface && recvEv_.methodsByType.contains( key_ ) ) )
                {
                    return true;
                }
            }
            return false;
        } );
    }

    // the bodied methods named `callee` class `cls` owns (ReceiverEvidence::collectMethods: by name, then ownership); the
    // caller keeps those of the call's language and root, as for every candidate list
    void collect( NodeId cls, std::string_view callee, std::vector<NodeId>& out ) const
    {
        recvEv_.found.clear();
        recvEv_.collectMethods( ing_.symbols[ cls ].name, callee, cls );
        out.insert( out.end(), recvEv_.found.begin(), recvEv_.found.end() );
    }

    // breadth-first down the implementors map from T, each type once (a cycle or a diamond is walked once): an Other edge (the
    // evidence names a different same-named type) is not followed; a ByName one is, its row and everything below it unproven
    const std::vector<Row>& closureOf( NodeId type ) const
    {
        if( const auto hit = memo_.find( type ); hit != memo_.end() )
        {
            return hit->second;
        }
        std::vector<Row>        rows;
        HashMap<NodeId, char>   seen;
        seen.emplace( type, char( 1 ) );
        const auto expand = [ & ]( NodeId parent, bool provenSoFar )
        {
            if( parent >= implementors_.size() )
            {
                return;
            }
            for( std::size_t k = 0; k < implementors_[ parent ].size(); ++k )
            {
                const BaseBound b = bound_.empty() ? BaseBound::ByName : BaseBound( bound_[ parent ][ k ] );
                if( b == BaseBound::Other )
                {
                    continue;
                }
                const NodeId child = implementors_[ parent ][ k ];
                if( seen.emplace( child, char( 1 ) ).second )
                {
                    rows.push_back( Row{ child, provenSoFar && b == BaseBound::Proven } );
                }
            }
        };
        expand( type, true );
        for( std::size_t i = 0; i < rows.size(); ++i )   // rows grow inside the loop: index, never iterate by reference
        {
            expand( rows[ i ].id, rows[ i ].proven );
        }
        return memo_.emplace( type, std::move( rows ) ).first->second;
    }

    const IngestResult&                           ing_;
    const ReceiverEvidence&                       recvEv_;
    const std::vector<std::vector<NodeId>>&       implementors_;
    const std::vector<std::vector<std::uint8_t>>& bound_;
    HashMap<std::string, char>                    contract_;   // "Owner::name" of every bodyless member
    mutable HashMap<NodeId, std::vector<Row>>     memo_;       // T → its closure
    mutable std::string                           key_;
};

}   // namespace rw
