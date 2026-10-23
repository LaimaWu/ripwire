#pragma once
// memberowner.h — QUALNAME-1: a member symbol row names its owner, n="Owner.name", from the STORED scope.
//
// A bare member name made the reader re-resolve it: a --callers list of `run`, `run`, `run` (three owners) or a <d n="render"
// sc="CmdRenderer"> row that a reader (or a grader) has to recombine. The owner is read off the parser's own scope (Symbol::scope,
// the sc= every other surface prints), never off the name, and printed only when the index DEFINES the scope's last segment as a
// member owner in the row's language — a class, struct, interface or named type, or a module (`@definition.module` is
// SymKind::Other: Rust `mod`, Ruby `module`, Elixir `defmodule`). A printed scope is a claim, so it is exact or bare:
//   * no stored scope (TS/JS, Java, C#, Swift, Kotlin `object` today) → bare: a floor, not a claim;
//   * a type or module row → bare (a class carries its own name as sc=; never `Box.Box`);
//   * a C++ namespace scope → bare (a namespace is no indexed definition, so a namespace-scope function stays a free function);
//   * an EXTENT-SUSPECT symbol → bare: a recovered parse can file a free function under the struct that swallowed it (extentsuspect.h);
//     its scope is an inference from the ERROR region, not a claim the source made;
//   * a FUNCTION-LOCAL def (Symbol::fnLocal: a Python/JS closure, whose scope walk skips the enclosing method) → bare: its owner
//     would read `Class.fn` for a function no `Class` instance carries.
// The spelling is the full stored scope, the language's member separator (langfacts.h memberSeparator: `::` C/C++/Rust/PHP, `#`
// Ruby, `.` elsewhere), then the name — every spelling a selector resolves. A constructor is a member by the same rule and prints
// in its language's own form (`Route.__init__`, `NinjaMain::NinjaMain`, `Config::new`).
// ONE rule for every row kind that prints a member: --path hops (defattrib.h emitPathHopRows), the --callers/--callees and --impact
// rows, the <d> signature rows (serialize.h sigRowHead, where it FOLDS sc= into n=). Built once per answer: one pass collecting the
// (language, name) of every owner definition, then a binary search per row.

#include "graph.h"       // namespaceCompatible — the one statement of "a type definition"
#include "langfacts.h"   // memberSeparator — the per-language spelling, as a column
#include "model.h"
#include "infra/sortutil.h"   // svLess: the explicit byte order a std::sort over string_view needs

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rw
{

// The kinds a scope must be defined as to print as an owner — and the kinds of a row that is itself an owner (never qualified).
// Reuse-first: the type half is graph.h namespaceCompatible's Type rule, plus a module (SymKind::Other). (Train 30's PATH-OWNERS
// rule, moved here from defattrib.h so the serialize.h rows can reach it.)
inline bool isMemberOwnerKind( SymKind k ) noexcept
{
    return k == SymKind::Other || namespaceCompatible( RefRole::Type, k );
}
// The last segment of a scope as written ("outer::Scanner" or "pkg.Outer" -> the innermost owner): the segment the index must
// define as an owner (graph.h scopeSuffixMatches resolves a selector by it).
inline std::string_view scopeLastSegment( std::string_view sc ) noexcept
{
    const std::size_t colons = sc.rfind( "::" );
    const std::size_t dot    = sc.rfind( '.' );
    const std::size_t cut    = colons == std::string_view::npos ? ( dot == std::string_view::npos ? 0 : dot + 1 )
                                                                : ( dot == std::string_view::npos || dot < colons ? colons + 2 : dot + 1 );
    return sc.substr( cut );
}

class MemberOwners
{
public:
    // One pass over the symbols: the (language, name) of every member-owner definition, sorted for the per-row lookup.
    explicit MemberOwners( const IngestResult& ing )
    {
        for( const Symbol& t : ing.symbols )
        {
            if( isMemberOwnerKind( t.kind ) && !t.name.empty() )
            {
                defined_.emplace_back( t.lang, std::string_view( t.name ) );
            }
        }
        std::sort( defined_.begin(), defined_.end(), keyLess );
        defined_.erase( std::unique( defined_.begin(), defined_.end() ), defined_.end() );
    }
    // The owner a member row prints ("" = bare): the symbol's full stored scope, when the rule above holds.
    std::string_view ownerOf( const Symbol& s ) const noexcept
    {
        if( s.scope.empty() || isMemberOwnerKind( s.kind ) || s.fnLocal || s.extentSuspect != 0 )
        {
            return {};
        }
        const std::string_view last = scopeLastSegment( s.scope );
        return !last.empty() && std::binary_search( defined_.begin(), defined_.end(), Key{ s.lang, last }, keyLess )
                   ? std::string_view( s.scope )
                   : std::string_view();
    }

private:
    using Key = std::pair<Lang, std::string_view>;
    static bool keyLess( const Key& a, const Key& b ) noexcept
    {
        if( a.first != b.first )
        {
            return a.first < b.first;   // language first: an owner counts only in the row's own language
        }
        return sortutil::svLess( a.second, b.second );
    }
    std::vector<Key> defined_;
};

// Whether any of `rows` prints an owner — a legend clause's presence. The owner pass runs only when some row has a scope at all.
inline bool anyMemberOwnerRow( const IngestResult& ing, std::span<const NodeId> rows )
{
    if( std::none_of( rows.begin(), rows.end(), [ & ]( NodeId id ) { return !ing.symbols[ id ].scope.empty(); } ) )
    {
        return false;
    }
    const MemberOwners owners( ing );
    return std::any_of( rows.begin(), rows.end(), [ & ]( NodeId id ) { return !owners.ownerOf( ing.symbols[ id ] ).empty(); } );
}

// The separator between an owner and a member name in `l` (langfacts.h).
inline std::string_view memberSeparator( Lang l ) noexcept
{
    return langFacts( l ).memberSeparator;
}
// The name a member row prints, unescaped: "Owner<sep>name" when `owner` is set, else the bare name.
inline std::string qualifiedMemberName( std::string_view owner, const Symbol& s )
{
    if( owner.empty() )
    {
        return s.name;
    }
    const std::string_view sep = memberSeparator( s.lang );
    std::string            q;
    q.reserve( owner.size() + sep.size() + s.name.size() );
    q.append( owner ).append( sep ).append( s.name );
    return q;
}
inline std::string qualifiedMemberName( const MemberOwners& owners, const Symbol& s )
{
    return qualifiedMemberName( owners.ownerOf( s ), s );
}

}   // namespace rw
