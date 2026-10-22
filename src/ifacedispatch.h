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
// (one outside it, or one a macro other than a mock attribute writes, may exist); otherwise the call is DECLINED with exactly that list (DeclinedSiteKind::Iface, gate="iface").
// No candidate: the tier stays out of the way, and the name tiers answer as before.
//
// EXACT OR NO ROW (fix round 1, reports/dispatch-classes.md §(a) 4 and 5). The one-candidate edge needs the WHOLE in-tree set:
//   * a list some implementor can join with no clause the map reads is OPEN, and the tier steps aside (graph.h implementorsOpen):
//     a Java / Kotlin anonymous class or object expression, a Kotlin `fun interface`, a Rust trait under a mock attribute (all
//     ImplBlock records with no implementor name), and a Java interface with one abstract method, which a lambda implements;
//   * a language that types structurally (TypeScript: an object literal or a matching class conforms unnamed) never proves one
//     candidate; two or more still decline, and cands=K is then a floor (the legend says so);
//   * Rust: an implementor's candidate sits inside its `impl T for X` block (an ImplBlock record's span); an inherent method of
//     X is never one, and a supertrait's method, whose impl block is another trait's, finds none here (the name tiers answer).
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
#include <iterator>
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
                   const std::vector<std::vector<std::uint8_t>>& bound, const std::vector<char>& open )
        : ing_( ing ), recvEv_( recvEv ), implementors_( implementors ), bound_( bound ), open_( open )
    {
        EXPECTS( bound.empty() || bound.size() == implementors.size(), "buildGraph sizes the bound map with the implementors map" );
        for( const Symbol& s : ing.symbols )
        {
            indexSymbol( s );
        }
        for( const Binding& b : ing.bindings )   // a trait impl block's span, keyed "T\x1fX" (model.h LocalBindKind::ImplBlock)
        {
            if( b.kind == LocalBindKind::ImplBlock && !b.typeName.empty() )
            {
                key_.assign( b.var ).push_back( '\x1f' );
                implBlocks_[ key_.append( b.typeName ) ].push_back( &b );
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
        const std::vector<Row>& rows   = closureOf( type );
        const auto              opened = [ & ]( NodeId t ) { return ( t < open_.size() && open_[ t ] != 0 ) || lambdaImplements( t ); };
        if( rows.empty() || opened( type ) || std::ranges::any_of( rows, [ & ]( const Row& row ) { return opened( row.id ); } ) )
        {
            return out;   // no implementor in the tree, or one the map cannot list: the name tiers answer, as before
        }
        bool provenAll = true;
        collect( type, callee, out.cand );   // T's own bodied default runs for every implementor without an override
        const bool hasDefault = !out.cand.empty();
        const bool implBlocks = langFacts( ing_.symbols[ type ].lang ).traitImplBlocks;
        for( const Row& row : rows )
        {
            const std::size_t before = out.cand.size();
            collect( row.id, callee, out.cand );
            if( implBlocks )
            {
                keepInImplBlock( type, row.id, out.cand, before );   // Rust: X's methods for T, never an inherent one
            }
            // a concrete implementor owning no body of the callee, with no default above it, holds one the index cannot see
            // (a macro-made override, a mock): the list is not the whole of what runs, so one candidate proves nothing
            const bool unseen = !hasDefault && out.cand.size() == before && ( row.id >= implementors_.size() || implementors_[ row.id ].empty() );
            provenAll = provenAll && row.proven && !unseen;
        }
        std::sort( out.cand.begin(), out.cand.end() );
        out.cand.erase( std::unique( out.cand.begin(), out.cand.end() ), out.cand.end() );
        out.fired  = !out.cand.empty();
        out.proven = provenAll && !langFacts( ing_.symbols[ type ].lang ).structuralTyping;   // structural: the list is a floor
        return out;
    }

private:
    // the bodyless members are an interface's or abstract class's contract; in a lambda-convertible language (langfacts.h) they are
    // also counted per owning class identity, and every class is listed under its name
    void indexSymbol( const Symbol& s )
    {
        const LangFacts& lf      = langFacts( s.lang );
        const bool       samLang = lf.samLambdas || lf.samFromJavaLambdas;
        javaInTree_              = javaInTree_ || lf.samLambdas;
        if( samLang && ReceiverEvidence::classLike( s.kind ) )
        {
            classesByName_[ s.name ].push_back( s.id );   // the identities one name stands for (a package apart)
        }
        if( !ReceiverEvidence::callableKind( s.kind ) || s.endByte > s.sigEndByte || s.id >= recvEv_.ownerClass.size() || recvEv_.ownerClass[ s.id ].empty() )
        {
            return;
        }
        contract_.try_emplace( recvEv_.ownerClass[ s.id ] + "::" + s.name, char( 1 ) );
        if( !samLang || std::ranges::contains( lf.samIgnoredMembers, std::string_view( s.name ) ) )
        {
            return;
        }
        if( const NodeId owner = recvEv_.classAt( s.fileId, s.sigStartByte, s.endByte, s.id ); owner != kNoNode )
        {
            std::vector<std::string_view>& names = abstractNames_[ owner ];   // per class IDENTITY, not per name: the SAM count
            if( !std::ranges::contains( names, std::string_view( s.name ) ) )
            {
                names.push_back( s.name );
            }
        }
    }

    // the one class-like symbol of a lambda-convertible language called `name`; kNoNode when none or several share it
    NodeId soleClassNamed( std::string_view name ) const
    {
        const auto it = classesByName_.find( name );
        return it != classesByName_.end() && it->second.size() == 1 ? it->second[ 0 ] : kNoNode;
    }

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

    // Rust: keep, of the candidates from `first` on (implementor `impl`'s methods named the callee), those inside one of its
    // `impl T for X` blocks (ImplBlock spans, by T's and X's names, as the inherit reference names them)
    void keepInImplBlock( NodeId type, NodeId impl, std::vector<NodeId>& cand, std::size_t first ) const
    {
        key_.assign( ing_.symbols[ type ].name ).push_back( '\x1f' );
        key_.append( ing_.symbols[ impl ].name );
        const auto hit = implBlocks_.find( key_ );
        const auto out = std::remove_if( cand.begin() + std::ptrdiff_t( first ), cand.end(), [ & ]( NodeId c )
        {
            const Symbol& m = ing_.symbols[ c ];
            return hit == implBlocks_.end() || std::none_of( hit->second.begin(), hit->second.end(), [ & ]( const Binding* b )
                   { return b->fileId == m.fileId && b->spanStart <= m.sigStartByte && m.endByte <= b->spanEnd; } );
        } );
        cand.erase( out, cand.end() );
    }

    // A lambda or a method reference implements an interface with exactly one abstract method (JLS 9.8), with no class the index
    // keeps, so such an interface's implementors list is open (langfacts.h samLambdas: Java; samFromJavaLambdas: Kotlin, when the
    // tree holds Java). EXACT OR NO ROW: every doubt opens the list. The count is per class IDENTITY (the index knows a base by
    // NAME only, and `a.Handler` and `b.Handler` are two interfaces): T's own members, plus those of the bases it names exactly.
    bool lambdaImplements( NodeId t ) const
    {
        const Symbol&    s  = ing_.symbols[ t ];
        const LangFacts& lf = langFacts( s.lang );
        // Kotlin's extractor reads an interface as a class (interfaceReadsAsClass), so there a class with no abstract member of
        // its own or above is a plain class, never an interface: the count must be exactly one
        if( !samConvertible( s.lang ) || !( s.kind == SymKind::Interface || ( lf.interfaceReadsAsClass && s.kind == SymKind::Class ) ) )
        {
            return false;
        }
        if( const auto hit = samMemo_.find( t ); hit != samMemo_.end() )
        {
            return hit->second != 0;
        }
        const AbstractCount ac = abstractCount( t, soleClassNamed( s.name ) != kNoNode );
        // a doubted 0 is a LOWER bound, never a certain 0: Kotlin's "exactly one" holds only for a count the index is sure of
        const bool sam = ac.n <= 1 && ( !lf.interfaceReadsAsClass || ac.n == 1 || ac.doubted );
        samMemo_.emplace( t, char( sam ? 1 : 0 ) );
        return sam;
    }

    // a lambda can implement this language's interfaces: Java's always, Kotlin's when a Java file in the tree can pass the lambda
    bool samConvertible( Lang l ) const
    {
        const LangFacts& lf = langFacts( l );
        return lf.samLambdas || ( lf.samFromJavaLambdas && javaInTree_ );
    }

    // the count of distinct abstract member names (stopping at 2) and whether it is only a LOWER bound
    struct AbstractCount
    {
        std::size_t n       = 0;
        bool        doubted = false;
    };

    // append the distinct abstract member names of class `id` to `names`
    void addOwnAbstract( NodeId id, std::vector<std::string_view>& names ) const
    {
        if( const auto it = abstractNames_.find( id ); it != abstractNames_.end() )
        {
            std::ranges::copy_if( it->second, std::back_inserter( names ),
                                  [ & ]( std::string_view n ) { return !std::ranges::contains( names, n ); } );
        }
    }

    // breadth-first through the bases the index names exactly, adding their abstract names; true when a base is not exactly one class
    // in the tree (shared name, or none) or the walk cap cut the walk short: the names are then only a lower bound
    bool walkAbstractBases( std::vector<std::string_view>& frontier, std::vector<std::string_view>& names ) const
    {
        bool        doubted = false;
        std::size_t i       = 0;
        for( ; i < frontier.size() && i < ReceiverEvidence::kWalkCap && names.size() <= 1; ++i )
        {
            recvEv_.forEachBase( frontier[ i ], [ & ]( std::string_view base )
            {
                const NodeId only = soleClassNamed( base );
                if( only == kNoNode )
                {
                    doubted = true;
                }
                else if( std::ranges::find( frontier, base ) == frontier.end() )
                {
                    frontier.push_back( base );
                    addOwnAbstract( only, names );
                }
            } );
        }
        return doubted || ( i < frontier.size() && names.size() <= 1 );
    }

    // the distinct abstract member names of class `c` (its own, plus those of the bases the index names exactly), stopping at 2:
    // a LOWER bound on the true count, so a doubt reads as "may be single" (`doubted`). An own name is certain; a base name shared by
    // several classes, or none in the tree, adds nothing and is not walked past (its bases are the union of every same-named class's).
    // `exactBases`: false when c's own name is shared, so its base names cannot be told apart per identity (doubted, not walked).
    AbstractCount abstractCount( NodeId c, bool exactBases ) const
    {
        std::vector<std::string_view> names;
        addOwnAbstract( c, names );
        std::vector<std::string_view> frontier{ ing_.symbols[ c ].name };
        const bool                    doubted = !exactBases || walkAbstractBases( frontier, names );
        return AbstractCount{ names.size(), doubted };
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
    const std::vector<char>&                      open_;       // graph.h implementorsOpen: a clause names an implementor the map cannot list
    HashMap<std::string, char>                    contract_;   // "Owner::name" of every bodyless member
    HashMap<NodeId, std::vector<std::string_view>>      abstractNames_;   // SAM-convertible languages: class id → its bodyless members' distinct names
    HashMap<std::string_view, std::vector<NodeId>>      classesByName_;   // SAM-convertible languages: name → every class-like symbol so named
    bool                                                javaInTree_ = false;   // a samLambdas-language symbol exists (a Kotlin SAM can take its lambda)
    HashMap<std::string, std::vector<const Binding*>>   implBlocks_;      // "T\x1fX" → the `impl T for X` blocks (ImplBlock)
    mutable HashMap<NodeId, std::vector<Row>>     memo_;       // T → its closure
    mutable HashMap<NodeId, char>                 samMemo_;    // T → lambdaImplements( T )
    mutable std::string                           key_;
};

}   // namespace rw
