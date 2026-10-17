#pragma once
#include "infra/emit.h"   // house rule: every header that may grow an emitter starts from THE emitter
// typeuse.h — L7 type-use impact: the symbols that NAME a type, read by the parser, so --impact of a class / struct /
// named type / interface starts its walk from them and not from call edges alone.
//
// THE DEFECT. --impact walks CALL edges into SYM (graph.h transitiveCallersDepth). A struct has no callers, so
// `--impact=grid_cell` on a C tree answered reaches="0" while 32 files declare, copy or access the struct; a TS class
// reached its constructor callers but never the file that defines it, nor a function that only takes it as a parameter.
//
// THE MECHANISM. One structural query per language (kTypeMentionRows, DATA rows — no Lang:: branch in the walk) finds
// every node that spells SYM's name AS A TYPE: a declaration, a parameter, a field, a return type, a base or implements
// clause, a template argument; for TS/JS also the class operand of `instanceof` and `extends`. The query runs through
// the shared --match walk (ingest.h astQueryGrouped), restricted to files whose bytes contain the name (mustContain —
// a file that never spells it cannot hold a node spelling it). Each hit maps to its innermost indexed definition
// (flipimpact.h innermostAtLine, the shared line → def lookup); that definition is a TYPE USER, the walk's first hop.
// A hit outside every definition (a file-scope declaration, a prototype the index does not keep) names its FILE.
//
// WHAT IT IS NOT. A mention is matched by its spelled NAME (the parser says it is a type position, not WHICH type):
// two indexed type definitions sharing the name make every row a possible namesake's (TypeUseTier::typeDefs, disclosed).
// The capture is not the --uses index: that index is built only by the rich ingest and records role=type for C++/ObjC
// alone (graphlegend.h kUsesLegend), so --impact (a lean verb) could not read it, and C — the motivating repo — is not in
// it. Languages without a row (C#, Kotlin, Swift, Ruby, PHP, Lua, Dart, Elixir, …) contribute no type users: a floor the
// legend names.
#include "model.h"
#include "graph.h"        // Graph, transitiveCallersDepth, provenCallerReach, namespaceCompatible
#include "ingest.h"       // AstQuerySpec / AstQueryGroup / astQueryGrouped — the shared structural-query walk
#include "flipimpact.h"   // buildSymbolLineIndex / innermostAtLine — the shared line → innermost-def lookup
#include "lintrules.h"    // langOfPath — the file's language, the key the rows are matched on
#include "graphlegend.h"  // ImpactRollupTypeClauses — the legend's view of the tier

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

// One row per language that can spell a type mention. `node` is a tree-sitter pattern capturing the type NAME as @t;
// the query wraps it with an #eq? on SYM's name. Several rows may share a pattern (the C family); a hit counts only
// when its file's language has a row with that pattern, so a pattern that happens to compile in another grammar
// (astQueryGrouped compiles each spec for every grammar it fits) contributes nothing there.
struct TypeMentionRow
{
    Lang             lang;
    std::string_view node;
};

inline constexpr TypeMentionRow kTypeMentionRows[] = {
    { Lang::C,          "(type_identifier) @t" },   // `struct grid_cell gc;`, `struct grid_cell *gc` params, fields, casts
    { Lang::Cpp,        "(type_identifier) @t" },   // + return types, template arguments, base clauses
    { Lang::ObjC,       "(type_identifier) @t" },
    { Lang::Go,         "(type_identifier) @t" },   // params, fields, composite literals `Cell{}`, receivers `(c *Cell)`
    { Lang::Rust,       "(type_identifier) @t" },   // params, fields, `impl Cell`, `impl Trait for Cell`, struct literals
    { Lang::Java,       "(type_identifier) @t" },   // declarations, params, `new Cell()`, extends / implements
    { Lang::TypeScript, "(type_identifier) @t" },   // annotations, `implements Router`, generic arguments
    { Lang::TypeScript, "(extends_clause value: (identifier) @t)" },                               // `class S extends Base`
    { Lang::TypeScript, "(binary_expression operator: \"instanceof\" right: (identifier) @t)" },   // `e instanceof Base`
    { Lang::JavaScript, "(class_heritage (identifier) @t)" },                                       // `class S extends Base`
    { Lang::JavaScript, "(binary_expression operator: \"instanceof\" right: (identifier) @t)" },
    { Lang::Python,     "(type (identifier) @t)" },                                    // annotations: `def f( c: Cell ) -> Cell`
    { Lang::Python,     "(class_definition superclasses: (argument_list (identifier) @t))" },   // `class S( Base ):`
};

// Does `lang` have a row whose pattern is `node`?
inline bool typeMentionRowFor( Lang lang, std::string_view node ) noexcept
{
    for( const TypeMentionRow& row : kTypeMentionRows )
    {
        if( row.lang == lang && row.node == node )
        {
            return true;
        }
    }
    return false;
}

// The name a type mention spells: the final segment of a definition's name (`ns::Widget` → `Widget`). Empty when that
// segment is not a plain identifier, which the #eq? literal could not carry safely — the tier then stays unarmed.
inline std::string_view typeMentionName( std::string_view name ) noexcept
{
    const std::size_t cut = name.find_last_of( ":." );
    const std::string_view seg = cut == std::string_view::npos ? name : name.substr( cut + 1 );
    const auto identChar = []( char c ) { return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' || c == '$'; };
    if( seg.empty() || ( seg[0] >= '0' && seg[0] <= '9' ) || !std::all_of( seg.begin(), seg.end(), identChar ) )
    {
        return {};
    }
    return seg;
}

// The kinds a type mention can name — the SAME set graph.h namespaceCompatible admits for RefRole::Type (reused, not
// restated: a class, a struct / named type, an interface).
inline bool isTypeImpactKind( SymKind k ) noexcept
{
    return namespaceCompatible( RefRole::Type, k );
}

struct TypeUseTier
{
    bool                       armed       = false;   // at least one seed is a type definition with a spellable name
    std::vector<NodeId>        users;                 // innermost defs holding a mention; seeds excluded; ascending
    std::vector<std::uint32_t> defFiles;              // the files defining the type seeds (the rollup's d="0" rows); ascending
    std::vector<std::uint32_t> scopeFiles;            // files naming the type outside every indexed def, minus defFiles; ascending
    std::size_t                typeDefs    = 0;       // indexed type definitions (the whole corpus) sharing the spelled name
    bool                       sitesCapped = false;   // a pattern's matches hit the walk budget: users/scopeFiles are a floor
};

// The per-pattern match budget. A type named on every line of a large tree is the only way to reach it; reaching it
// is disclosed (type_capped="1"), never silent.
inline constexpr std::size_t kTypeMentionMatchBudget = 50000;

inline TypeUseTier typeUseTier( const IngestResult& ing, std::span<const NodeId> seeds )
{
    TypeUseTier tier;
    std::string name;
    for( const NodeId s : seeds )
    {
        if( s >= ing.symbols.size() || !isTypeImpactKind( ing.symbols[s].kind ) )
        {
            continue;
        }
        const std::string_view nm = typeMentionName( ing.symbols[s].name );
        if( nm.empty() || ( !name.empty() && nm != name ) )
        {
            continue;   // one selector spells one name; a seed spelled otherwise (a qualified alias) keeps its call walk only
        }
        name.assign( nm );
        tier.defFiles.push_back( ing.symbols[s].fileId );
    }
    if( name.empty() )
    {
        return tier;   // no type seed: the caller's walk is the plain call walk, byte-identical
    }
    tier.armed = true;
    std::sort( tier.defFiles.begin(), tier.defFiles.end() );
    tier.defFiles.erase( std::unique( tier.defFiles.begin(), tier.defFiles.end() ), tier.defFiles.end() );
    for( const Symbol& s : ing.symbols )
    {
        tier.typeDefs += std::size_t( isTypeImpactKind( s.kind ) && typeMentionName( s.name ) == name );
    }

    // One spec per DISTINCT pattern; the spec's tag is the pattern itself, so a hit is matched back to its rows.
    std::vector<AstQuerySpec> specs;
    for( const TypeMentionRow& r : kTypeMentionRows )
    {
        if( std::none_of( specs.begin(), specs.end(), [ & ]( const AstQuerySpec& sp ) { return sp.tag == r.node; } ) )
        {
            specs.push_back( AstQuerySpec{ "(" + std::string( r.node ) + " (#eq? @t \"" + name + "\"))", std::string( r.node ) } );
        }
    }
    AstQueryGroup group;
    group.specs       = &specs;
    group.maxMatches  = kTypeMentionMatchBudget;
    group.mustContain = name;
    const std::vector<std::vector<AstMatch>> got = astQueryGrouped( ing, { group } );
    ASSUME( got.size() == 1, "astQueryGrouped answers one match list per group" );

    std::vector<std::size_t> perTag( specs.size(), 0 );
    const flipimpact::SymbolLineIndex lines = flipimpact::buildSymbolLineIndex( ing );
    std::vector<char> isSeed( ing.symbols.size(), 0 );
    for( const NodeId s : seeds )
    {
        if( s < isSeed.size() ) { isSeed[s] = 1; }
    }
    for( const AstMatch& m : got[0] )
    {
        for( std::size_t i = 0; i < specs.size(); ++i )
        {
            perTag[i] += std::size_t( specs[i].tag == m.tag );
        }
        if( m.fileId >= ing.files.size() || !typeMentionRowFor( langOfPath( ing.files[m.fileId] ), m.tag ) )
        {
            continue;   // the pattern compiled in a grammar no row names for this file's language
        }
        const NodeId host = flipimpact::innermostAtLine( ing, lines, m.fileId, m.line );
        if( host == kNoNode )
        {
            tier.scopeFiles.push_back( m.fileId );
        }
        else if( !isSeed[host] )
        {
            tier.users.push_back( host );
        }
    }
    tier.sitesCapped = std::any_of( perTag.begin(), perTag.end(), []( std::size_t n ) { return n >= kTypeMentionMatchBudget; } );
    std::sort( tier.users.begin(), tier.users.end() );
    tier.users.erase( std::unique( tier.users.begin(), tier.users.end() ), tier.users.end() );
    std::sort( tier.scopeFiles.begin(), tier.scopeFiles.end() );
    tier.scopeFiles.erase( std::unique( tier.scopeFiles.begin(), tier.scopeFiles.end() ), tier.scopeFiles.end() );
    std::erase_if( tier.scopeFiles, [ & ]( std::uint32_t f ) { return std::binary_search( tier.defFiles.begin(), tier.defFiles.end(), f ); } );
    ENSURES( std::is_sorted( tier.users.begin(), tier.users.end() ), "users ascend (the reach merge below relies on it)" );
    return tier;
}

// The --impact walk with the type tier folded in. Unarmed: exactly the call walk (transitiveCallersDepth +
// provenCallerReach over the seeds), so a function impact is byte-identical. Armed: every type user enters the walk at
// d=1 beside SYM's direct callers, their callers follow; `proven` counts a type user as proven (the parser read the
// mention — it is not a name-only CALL edge), and `typeOnly` marks the nodes the plain call walk does not reach.
struct ImpactReach
{
    std::vector<NodeId>        reach;      // ascending, seeds excluded
    std::vector<std::uint32_t> depth;      // per node
    std::vector<char>          proven;     // provenCallerReach's shape (empty when the graph has no name-only flags)
    std::vector<char>          typeOnly;   // per node; empty when the tier is unarmed
};

inline ImpactReach impactReachWithTypes( const Graph& g, std::span<const NodeId> seeds, const TypeUseTier& tier )
{
    ImpactReach r;
    if( !tier.armed )
    {
        r.reach  = transitiveCallersDepth( g, seeds, &r.depth );
        r.proven = provenCallerReach( g, seeds );
        return r;
    }
    r.reach = transitiveCallersDepth( g, seeds, &r.depth, tier.users );
    std::vector<NodeId> both( seeds.begin(), seeds.end() );
    both.insert( both.end(), tier.users.begin(), tier.users.end() );
    r.proven = provenCallerReach( g, both );
    const std::vector<NodeId> byCalls = transitiveCallers( g, seeds );
    r.typeOnly.assign( r.depth.size(), 0 );
    for( const NodeId n : r.reach )
    {
        r.typeOnly[n] = std::binary_search( byCalls.begin(), byCalls.end(), n ) ? 0 : 1;
    }
    return r;
}

// The rollup's view of the tier (graph.h ImpactFileRollup), or null when the tier is unarmed — `out` is the caller's storage
// and must outlive the rollup call; its spans borrow `tier`, its typeOnly `walk`.
inline const ImpactTypeFold* impactTypeFold( const TypeUseTier& tier, const ImpactReach& walk, ImpactTypeFold& out )
{
    if( !tier.armed )
    {
        return nullptr;
    }
    out = ImpactTypeFold{ tier.defFiles, tier.scopeFiles, &walk.typeOnly, tier.users.size(), tier.typeDefs, tier.sitesCapped };
    return &out;
}

// The legend's view of a rollup's type tier (graphlegend.h impactRollupLegend).
inline ImpactRollupTypeClauses impactRollupTypeClauses( const ImpactFileRollup& r ) noexcept
{
    return ImpactRollupTypeClauses{ r.typed, r.typeDefs, r.typeCapped };
}

} // namespace rw
