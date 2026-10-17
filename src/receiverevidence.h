#pragma once
// receiverevidence.h — FE-B (test/receiverevidencecheck.sh): what proves a call's RECEIVER, and what does not.
//
// A member call `x.m()` (or, where the receiver is implicit, a bare `m()`) used to bind to every in-repo definition
// spelled `m` that the name ladder reached, and the answer showed each such row as a plain edge — a confident claim
// with nothing behind it but the name. This header holds the two halves of the fix:
//
//   1. ReceiverEvidence::narrow — a RESOLVE rule for receivers the source types: `this`/`self`/`cls` inside a class (and
//      the language's base-class receiver, isSuperRoot, through the bases only), a parameter or local whose class is
//      written (a TS/Python/Go annotation, a Go method receiver, a JS `new Foo()` / Python `Foo()` / Go `Foo{}` initializer), a constructed receiver
//      (`new Foo().m()`), a class-name receiver (`Foo.m()`), and a chain of fields whose classes are stated
//      (`this.bucket = new Schemas()`, Python `self.x = Foo()`, a Go struct field, a Go EMBEDDED field whose methods
//      are promoted). A hit is the call's whole answer, exactly like Rule 2. Ruby's own method lookup (mixins, typed and
//      class receivers) is deliberately NOT modelled here: Ruby keeps only the language-neutral proof below.
//   2. ReceiverEvidence::ladderProves — for a call the rules above did not answer, which name-ladder candidates the
//      language's own lookup can PROVE: an implicit receiver's own class and its bases, a free function in scope, a
//      definition in the module a receiver alias names. A call left with no proven candidate is NAME-ONLY: its rows
//      are kept, marked via="name" on every surface (graph.h Graph::outNameOnly), never dropped and never proven.
//
// What it does NOT do: infer a function's return type (`s = self.get_screen(); s.refresh()`), follow a property, or type
// a value that crossed a call boundary. Those receivers stay name-only — the honest answer, not a guess.

#include "model.h"
#include "resolve.h"   // includerDir; HashMap / SmallVec through it
#include "smallvec.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

// The languages whose name-ladder rows the hedge covers. Every other language keeps its ladder unchanged (a disclosed
// floor in test/receiverevidencecheck.sh: Objective-C, PHP, Lua, Zig, Dart, Elixir, Scala, Bash and the rest).
inline bool receiverHedgeLang( Lang l ) noexcept
{
    switch( l )
    {
        case Lang::JavaScript: case Lang::TypeScript: case Lang::Python: case Lang::Go: case Lang::Rust: case Lang::C:
        case Lang::Cpp: case Lang::Java: case Lang::Kotlin: case Lang::CSharp: case Lang::Swift: case Lang::Ruby:
            return true;
        default:
            return false;
    }
}

// FE-B: each Go file's package import paths, built by graph.h collectGoModules — the nearest go.mod's module path plus the
// directory below it, and the path a LOCAL `replace L => ./D` gives a file under D (L plus the directory below D). A
// module-alias call proves a candidate by one of these paths EXACTLY. The directory-tail rule is left only for a candidate
// with no path when no in-tree file carries the import path either (a tree with no module facts for it at all).
struct GoPackagePaths
{
    std::vector<std::string>   nearest;    // fileId → "" when no go.mod with a module line sits at or above the file
    std::vector<std::string>   replaced;   // fileId → "" when no local replace maps the file's directory
    HashMap<std::string, char> known;      // every non-empty path in either table

    // file `f`'s entry in `table` ("" when none)
    static std::string_view pathIn( const std::vector<std::string>& table, std::uint32_t f ) noexcept
    {
        return f < table.size() ? std::string_view( table[ f ] ) : std::string_view{};
    }
    bool hasPath( std::uint32_t f ) const noexcept
    {
        return !pathIn( nearest, f ).empty() || !pathIn( replaced, f ).empty();
    }
    // does `path` name file `f`'s package exactly
    bool names( std::uint32_t f, std::string_view path ) const noexcept
    {
        return !path.empty() && ( pathIn( nearest, f ) == path || pathIn( replaced, f ) == path );
    }
    void note( std::vector<std::string>& table, std::uint32_t f, std::string path )
    {
        EXPECTS( f < table.size(), "collectGoModules sizes both tables to ing.files" );
        if( !path.empty() )
        {
            known.try_emplace( path, '\0' );
        }
        table[ f ] = std::move( path );
    }
};

// The languages where a bare `m()` inside a method is an implicit `this.m()` (or reaches a free/top-level function).
inline bool implicitReceiverLang( Lang l ) noexcept
{
    return l == Lang::Java || l == Lang::Kotlin || l == Lang::CSharp || l == Lang::Swift || l == Lang::Cpp || l == Lang::Ruby;
}

// the languages where a call written on a receiver reaches a function only as a METHOD (or through a module alias): a
// free function is never what `x.f()` calls. JS/TS (an object property may hold one), Kotlin (an extension function is a
// top-level `fun T.f()`) and C (a function-pointer field) are left out.
inline bool memberNeverReachesFreeFunction( Lang l ) noexcept
{
    return l == Lang::Python || l == Lang::Go || l == Lang::Cpp || l == Lang::Rust || l == Lang::Ruby || l == Lang::Java
        || l == Lang::CSharp || l == Lang::Swift;
}

// a call through a module alias (`pkg.F()`) reaches the module's package-level functions only, never a method: Go, where a
// package qualifier never names a receiver (train 29 call-result-typing, review decision (b): nfpm's `nfpm.Get( … )` beside
// `( *Config ).Get`). A local spelled like the import hides it before this is asked (graph.h ladderProves).
inline bool moduleCallReachesFunctionsOnly( Lang l ) noexcept
{
    switch( l )
    {
        case Lang::Go: return true;
        default:       return false;
    }
}

// (L) the languages whose own receiver rules (Rules 1, 2c, FE-A) answer self, fields and paths, so receiver evidence adds
// only a LOCAL whose class a binding states (ReceiverEvidence::narrowTypedLocal): Rust, whose locals are typed by the call
// results graph.h's resolve loop records (train 29; the rust-receivers lane adds written `let x: T` and parameters).
inline bool receiverLocalsOnly( Lang l ) noexcept
{
    switch( l )
    {
        case Lang::Rust: return true;
        default:         return false;
    }
}

// (L) which members a call-result-typed local's CLASS owns (fix round 1: the typed fact carries the class's id, and a member
// call on the local reaches that class's own members, never a same-named class's): Lexical — the class whose span holds the
// member (Java, Kotlin, TS; a member outside every class, a Kotlin extension, belongs to the class's own file); PackageDir —
// Go, whose methods sit beside the type in its package's directory; FileOrUnique — Rust, whose `impl` blocks sit in the
// type's own file, or anywhere when no other in-tree type shares its name.
enum class MemberOwnerRule : std::uint8_t { Lexical, PackageDir, FileOrUnique };
inline MemberOwnerRule memberOwnerRuleOf( Lang l ) noexcept
{
    switch( l )
    {
        case Lang::Go:   return MemberOwnerRule::PackageDir;
        case Lang::Rust: return MemberOwnerRule::FileOrUnique;
        default:         return MemberOwnerRule::Lexical;
    }
}

// the languages where `Cls.m()` is a call on the CLASS (a static or an unbound method): C++ spells that `Cls::m()` (a
// qualifier, the canonical tier's), so a C++/C/Rust receiver spelled like a class is a variable or a member; Ruby's class
// receivers are its own lookup's question
inline bool classNameReceiverLang( Lang l ) noexcept
{
    return l == Lang::JavaScript || l == Lang::TypeScript || l == Lang::Python || l == Lang::Java || l == Lang::Kotlin
        || l == Lang::CSharp || l == Lang::Swift;
}

// a call written ON a receiver — every language's member shape: Python/C++/Ruby record a RecvKind, the rest memberCall
inline bool isMemberCallRef( const Reference& r ) noexcept
{
    return r.recv != RecvKind::None || r.memberCall;
}

inline bool isThisRoot( std::string_view root ) noexcept
{
    return root == "this" || root == "self" || root == "Self";
}

// Is `root` (the call's receiver root, ReceiverEvidence::chainOf) the language's BASE-CLASS receiver — a lookup that walks
// the caller's bases only? Each spelling counts only where the grammar makes it that keyword: C# `base`; `super` in
// Java, Kotlin, Swift, JS and TS; Python's `super()` / `super(C, self)` call, which ingest records as RecvKind::SuperObj.
// Everywhere else the same text is an ordinary name — a Python/JS/Go/Java/… local `base`, a Go or C# local `super`, a
// Python name `super` that shadows the builtin — and proves nothing about the bases (its own type, if any, decides).
inline bool isSuperRoot( const Reference& r, std::string_view root ) noexcept
{
    if( r.recv == RecvKind::SuperObj )
    {
        return true;
    }
    switch( r.lang )
    {
        case Lang::CSharp:
            return root == "base";
        case Lang::Java: case Lang::Kotlin: case Lang::Swift: case Lang::JavaScript: case Lang::TypeScript:
            return root == "super";
        default:
            return false;
    }
}

// The innermost function/method of `id`'s file whose span strictly holds `id`'s span; kNoNode when none. `byFile` is
// fileId → that file's functions/methods sorted by sigStartByte. Shared by ReceiverEvidence and graph.h FalseEdgeRules.
inline NodeId innermostEnclosingFn( const IngestResult& ing, const std::vector<std::vector<NodeId>>& byFile, NodeId id )
{
    const Symbol& inner = ing.symbols[ id ];
    if( inner.fileId >= byFile.size() )
    {
        return kNoNode;
    }
    NodeId best = kNoNode;
    for( NodeId c : byFile[ inner.fileId ] )
    {
        const Symbol& s = ing.symbols[ c ];
        if( s.sigStartByte > inner.sigStartByte )
        {
            break;   // sorted by start: nothing later can hold it
        }
        if( c != id && s.endByte >= inner.endByte && ( s.sigStartByte < inner.sigStartByte || s.endByte > inner.endByte ) )
        {
            best = c;   // later starts are more inner
        }
    }
    return best;
}

// (L) fix round 1 — CallResultTypes: what a declared result type names, decided in its callee's file.
// A type is recorded for a call result only when its written text RESOLVES, through the callee file's own context —
// its imports, its qualifier, its own file, then its package / module / namespace — to exactly ONE in-tree class;
// the typed local then carries that class (LocalFact::cls), never its name alone. A name imported from outside, an
// outside qualifier, a global, a name that resolves nowhere or to two classes: kNoNode, nothing recorded. The import
// and qualifier readers differ per family (the rows below); the rule is one.
struct CallResultTypes
{
    const IngestResult&                                   ing;
    HashMap<std::string, rw::SmallVec<NodeId, 2>>         classDefsByName;            // class-like name → its definitions, id order
    std::vector<std::vector<const Binding*>>              importsByFile;              // fileId → its TypeImport / JsImport records
    const HashMap<std::string, std::string>*              moduleAlias     = nullptr;  // ReceiverEvidence's "<fileId>#name" → module
    const GoPackagePaths*                                 goPackages      = nullptr;  // graph.h FalseEdgeRules::goPackages
    const HashMap<std::string, NodeId>*                   jsPinnedClasses = nullptr;  // graph.h: "<fileId>#<local>" → the class a TS import pins
    const ClassIdentity*                                  classIds        = nullptr;  // graph.h: C-family class identity (resolve.h)
    const std::vector<std::vector<NodeId>>*               fileIncludes    = nullptr;  // graph.h: C-family transitive include set per file (sorted)
    const HashMap<std::string, std::vector<std::string>>* chaUp           = nullptr;  // class name → its direct base names
    // fix round 2: the Namespace records (scopes the index has no symbol for) in (file, start) order, the C++ files holding a
    // namespace- or file-scope using (sorted), and each class's innermost enclosing symbol as first asked (kNoNode: file scope)
    std::vector<const Binding*>                           namespaces;
    std::vector<std::uint32_t>                            cppUsingFiles;
    HashMap<std::string, char>                            jvmPackages;     // the Java / Kotlin packages the tree declares
    mutable HashMap<NodeId, NodeId>                       enclosingMemo;
    // Rust crate/module layout, built on the first Rust resolution: a file's crate (its nearest `lib.rs` / `main.rs`
    // directory) and module path (`a::b` for src/a/b.rs or src/a/b/mod.rs), the crates by name (`mdbook_core` for
    // crates/mdbook-core/src), and each module's files
    struct RustLayout
    {
        bool                                              built = false;
        std::vector<std::uint32_t>                        crateOf;    // fileId → crate index, kNoCrate when none
        std::vector<std::string>                          moduleOf;   // fileId → module path ("" = the crate root)
        HashMap<std::string, std::uint32_t>               crateByName;
        HashMap<std::string, std::vector<std::uint32_t>>  filesOf;    // "<crate>#<module>" → its files
    };
    static constexpr std::uint32_t                        kNoCrate = 0xFFFFFFFFu;
    mutable RustLayout                                    rust;
    mutable std::string                                   nameKey;    // classesNamed's lookup buffer
    mutable std::string                                   aliasKey;   // importPathOf's / the TS pin lookup's
    mutable std::vector<std::string_view>                 usingPath;  // forEachCppUsing's per-row namespace path
    mutable std::string                                   moduleKey;  // rustModuleKey's buffer

    explicit CallResultTypes( const IngestResult& i ) : ing( i ) {}

    // the index build: every class-like definition by name, and each file's import records
    void noteClass( const Symbol& s ) { classDefsByName[ s.name ].push_back( s.id ); }
    void noteImport( const Binding& b )
    {
        if( b.kind == LocalBindKind::Namespace && b.spanEnd == 0 )
        {
            jvmPackages.try_emplace( b.var, char( 1 ) );   // a Java / Kotlin `package` declaration (model.h)
            return;
        }
        if( b.kind == LocalBindKind::Namespace )
        {
            namespaces.push_back( &b );
            return;
        }
        if( ( b.kind == LocalBindKind::TypeImport || b.kind == LocalBindKind::JsImport ) && b.fileId < importsByFile.size() && !b.var.empty() )
        {
            importsByFile[ b.fileId ].push_back( &b );
            if( isCppUsing( b ) && ( cppUsingFiles.empty() || cppUsingFiles.back() != b.fileId ) )
            {
                cppUsingFiles.push_back( b.fileId );
            }
        }
    }
    // a C++ using-directive / using-declaration: the one TypeImport that carries its scope's span (model.h)
    static bool isCppUsing( const Binding& b ) noexcept { return b.kind == LocalBindKind::TypeImport && b.spanEnd != 0; }
    // after the last noteImport: the two lists in (file, start) / file order
    void finishIndex()
    {
        std::stable_sort( namespaces.begin(), namespaces.end(), []( const Binding* a, const Binding* b )
        {
            return a->fileId != b->fileId ? a->fileId < b->fileId : a->startByte < b->startByte;
        } );
        std::sort( cppUsingFiles.begin(), cppUsingFiles.end() );
        cppUsingFiles.erase( std::unique( cppUsingFiles.begin(), cppUsingFiles.end() ), cppUsingFiles.end() );
    }
    // the import path the file-scope name `name` binds in file `fileId` ("" when none, or rebound)
    std::string_view importPathOf( std::uint32_t fileId, std::string_view name ) const
    {
        if( moduleAlias == nullptr )
        {
            return {};
        }
        aliasKey.assign( std::to_string( fileId ) ).append( 1, '#' ).append( name );
        const auto it = moduleAlias->find( aliasKey );
        return it == moduleAlias->end() ? std::string_view{} : std::string_view( it->second );
    }
    std::string_view dirOf( std::uint32_t fileId ) const { return includerDir( rootRelPath( ing, fileId ) ); }
    // is `dir` the directory `tail` names, read from its end and segment-aligned (`a/b/model` ends `b/model`, not `del`):
    // resolve.h's tsimport::fileNamed, refusing an empty tail (the repository root names no package)
    static bool endsWithDir( std::string_view dir, std::string_view tail ) noexcept
    {
        return !tail.empty() && tsimport::fileNamed( dir, tail );
    }
    // the in-tree class-likes named `name` of family `fam` that `keep` admits: their count, and the last one in `one`
    template <class Keep>
    std::size_t classesNamed( std::string_view name, CallResultFamily fam, Keep&& keep, NodeId& one ) const
    {
        one = kNoNode;
        nameKey.assign( name );
        const auto it = classDefsByName.find( nameKey );
        if( it == classDefsByName.end() )
        {
            return 0;
        }
        std::size_t n = 0;
        for( NodeId c : it->second )
        {
            if( callResultFamilyOf( ing.symbols[ c ].lang ) == fam && keep( c ) )
            {
                one = c;
                ++n;
            }
        }
        return n;
    }
    template <class Keep>
    NodeId onlyClass( std::string_view name, CallResultFamily fam, Keep&& keep ) const
    {
        NodeId one = kNoNode;
        return classesNamed( name, fam, keep, one ) == 1 ? one : kNoNode;
    }
    // the import records of file `fileId` binding the local name `var` (TypeImport / JsImport)
    template <class Fn>
    void forEachImport( std::uint32_t fileId, std::string_view var, Fn&& fn ) const
    {
        if( fileId < importsByFile.size() )
        {
            for( const Binding* b : importsByFile[ fileId ] )
            {
                if( b->var == var )
                {
                    fn( *b );
                }
            }
        }
    }
    // ── lexical scope (fix round 2): is a class in scope, by its bare name, where the callee is declared ──
    // the Namespace records of file `f`, outermost first
    std::span<const Binding* const> namespacesOf( std::uint32_t f ) const
    {
        const auto lo = std::lower_bound( namespaces.begin(), namespaces.end(), f, []( const Binding* b, std::uint32_t v ) { return b->fileId < v; } );
        const auto hi = std::upper_bound( lo, namespaces.end(), f, []( std::uint32_t v, const Binding* b ) { return v < b->fileId; } );
        return { lo, hi };
    }
    // the innermost symbol of `c`'s file whose span strictly holds `c`'s — the class, module or function it is nested in;
    // kNoNode at file scope. Symbol ids ascend by (file, line), so a holder sits before `c` or after it on `c`'s own line.
    NodeId enclosingSymbol( NodeId c ) const
    {
        if( const auto it = enclosingMemo.find( c ); it != enclosingMemo.end() )
        {
            return it->second;
        }
        const Symbol& cs       = ing.symbols[ c ];
        NodeId        best     = kNoNode;
        const auto    consider = [ & ]( NodeId s )
        {
            const Symbol& o     = ing.symbols[ s ];
            const bool    holds = o.sigStartByte <= cs.sigStartByte && o.endByte >= cs.endByte && ( o.sigStartByte < cs.sigStartByte || o.endByte > cs.endByte );
            best = holds && ( best == kNoNode || o.sigStartByte > ing.symbols[ best ].sigStartByte ) ? s : best;
        };
        for( NodeId s = c; s > 0 && ing.symbols[ s - 1 ].fileId == cs.fileId; --s )
        {
            consider( s - 1 );
        }
        for( NodeId s = c + 1; s < ing.symbols.size() && ing.symbols[ s ].fileId == cs.fileId && ing.symbols[ s ].line == cs.line; ++s )
        {
            consider( s );
        }
        enclosingMemo.try_emplace( c, best );
        return best;
    }
    // does a Namespace record of `c`'s file hold `c`
    bool inNamespace( NodeId c ) const
    {
        const Symbol& cs = ing.symbols[ c ];
        for( const Binding* ns : namespacesOf( cs.fileId ) )
        {
            if( ns->spanStart <= cs.sigStartByte && cs.sigStartByte < ns->spanEnd )
            {
                return true;
            }
        }
        return false;
    }
    // Is class `c` in scope by its bare name at byte `at` of file `f` (a callee's declaration)? In its own file every scope
    // enclosing it — an indexed symbol (class, module, function) or a Namespace record — must hold `at` too (inside, not at,
    // the scope's own start: a function's local class is not in scope in its own signature); from another file — its package,
    // its module — it must sit at file scope: a nested class is reached through its outer name, never bare.
    bool visibleAt( NodeId c, std::uint32_t f, std::uint32_t at ) const
    {
        const Symbol& cs    = ing.symbols[ c ];
        const NodeId  outer = enclosingSymbol( c );
        if( cs.fileId != f )
        {
            return outer == kNoNode && !inNamespace( c );
        }
        if( outer != kNoNode && !( ing.symbols[ outer ].sigStartByte < at && at < ing.symbols[ outer ].endByte ) )
        {
            return false;
        }
        for( const Binding* ns : namespacesOf( f ) )
        {
            const bool holdsClass = ns->spanStart <= cs.sigStartByte && cs.sigStartByte < ns->spanEnd;
            if( holdsClass && !( ns->spanStart <= at && at < ns->spanEnd ) )
            {
                return false;
            }
        }
        return true;
    }
    // the unqualified `name` in scope at byte `at` of `fileId` itself (visibleAt): kNoNode with `found` false when none is;
    // several → kNoNode, found. `nestedOnly` keeps only classes nested in a scope that holds the callee.
    NodeId inOwnFile( std::uint32_t fileId, std::uint32_t at, std::string_view name, CallResultFamily fam, bool& found, bool nestedOnly = false ) const
    {
        NodeId one = kNoNode;
        const std::size_t n = classesNamed( name, fam, [ & ]( NodeId c )
        {
            return ing.symbols[ c ].fileId == fileId && ( !nestedOnly || enclosingSymbol( c ) != kNoNode ) && visibleAt( c, fileId, at );
        }, one );
        found = n > 0;
        return n == 1 ? one : kNoNode;
    }
    // Java / Kotlin: does a class enclosing the callee inherit — through an in-tree base, transitively — a nested class
    // `name`? Such a member type hides the package's and an import's class of that name, and is not resolved here.
    bool inheritsNested( std::uint32_t f, std::uint32_t at, std::string_view name ) const
    {
        NodeId one = kNoNode;
        if( chaUp == nullptr || classesNamed( name, CallResultFamily::Jvm, [ & ]( NodeId c ) { return enclosingSymbol( c ) != kNoNode; }, one ) == 0 )
        {
            return false;
        }
        std::vector<std::string_view> cone;   // the bases of every class enclosing the callee
        const auto lo = std::lower_bound( ing.symbols.begin(), ing.symbols.end(), f, []( const Symbol& o, std::uint32_t v ) { return o.fileId < v; } );
        for( auto it = lo; it != ing.symbols.end() && it->fileId == f; ++it )
        {
            if( it->sigStartByte < at && at < it->endByte && ( it->kind == SymKind::Class || it->kind == SymKind::Interface ) )
            {
                cone.push_back( it->name );
            }
        }
        for( std::size_t i = 0; i < cone.size() && i < 64; ++i )
        {
            if( const auto it = chaUp->find( std::string( cone[ i ] ) ); it != chaUp->end() )
            {
                for( const std::string& b : it->second )
                {
                    cone.push_back( b );
                }
            }
        }
        return classesNamed( name, CallResultFamily::Jvm, [ & ]( NodeId c )
        {
            const NodeId outer = enclosingSymbol( c );
            return outer != kNoNode && std::find( cone.begin(), cone.end(), std::string_view( ing.symbols[ outer ].name ) ) != cone.end()
                && !( ing.symbols[ outer ].fileId == f && ing.symbols[ outer ].sigStartByte < at && at < ing.symbols[ outer ].endByte );
        }, one ) > 0;
    }

    // Go: `pkg.T` names a class of THIS tree only when `pkg` imports an in-tree package and that package defines exactly one
    // `T` — the exact package path when the tree has module facts (go.mod), else a directory that ends the path,
    // segment-aligned. Shared by written types (a local, parameter or field `f *os.File`) and call results.
    NodeId goQualifiedClass( std::uint32_t fileId, std::string_view whole, std::string_view cls ) const
    {
        const std::size_t dot = whole.find( '.' );
        if( dot == std::string_view::npos || cls.empty() )
        {
            return kNoNode;
        }
        const std::string path( importPathOf( fileId, whole.substr( 0, dot ) ) );
        if( path.empty() )
        {
            return kNoNode;
        }
        const bool modules = goPackages != nullptr && !goPackages->known.empty();
        return onlyPackageClass( cls, [ & ]( NodeId c )
        {
            return ( modules ? goPackages->names( ing.symbols[ c ].fileId, path ) : endsWithDir( path, dirOf( ing.symbols[ c ].fileId ) ) )
                && enclosingSymbol( c ) == kNoNode;   // a package-level type, never one local to a function
        } );
    }
    NodeId goResultClass( std::uint32_t fileId, std::uint32_t at, std::string_view whole, std::string_view cls ) const
    {
        if( !whole.empty() )
        {
            return goQualifiedClass( fileId, whole, cls );
        }
        const std::string_view dir = dirOf( fileId );   // one package is one directory
        return onlyPackageClass( cls, [ & ]( NodeId c ) { return dirOf( ing.symbols[ c ].fileId ) == dir && visibleAt( c, fileId, at ); } );
    }
    // Go: the class `cls` of the ONE package directory `keep` admits — a type a package defines once per build-tag file
    // (`Executor` in util_unix.go and util_windows.go) is one class to the member rule (PackageDir), so its first
    // definition stands for it; two packages' namesakes are kNoNode
    template <class Keep>
    NodeId onlyPackageClass( std::string_view cls, Keep&& keep ) const
    {
        NodeId           first = kNoNode;
        std::string_view dir;
        bool             one   = true;
        NodeId           last  = kNoNode;
        classesNamed( cls, CallResultFamily::Go, [ & ]( NodeId c )
        {
            if( !keep( c ) )
            {
                return false;
            }
            const std::string_view d = dirOf( ing.symbols[ c ].fileId );
            one   = one && ( first == kNoNode || d == dir );
            first = first == kNoNode ? c : first;
            dir   = first == c ? d : dir;
            return true;
        }, last );
        return one ? first : kNoNode;
    }

    // Java / Kotlin: a single-type import of the name (Kotlin's `as` alias included) decides, then the file's own class, then
    // the package (its directory). An imported or qualified `a.b.T` names an in-tree class only in a directory ending `a/b`.
    NodeId jvmClassIn( std::string_view path, std::string_view cls ) const
    {
        const std::size_t dot = path.rfind( '.' );
        if( dot == std::string_view::npos )
        {
            return kNoNode;
        }
        std::string pkgDir( path.substr( 0, dot ) );
        std::replace( pkgDir.begin(), pkgDir.end(), '.', '/' );
        return onlyClass( cls, CallResultFamily::Jvm, [ & ]( NodeId c )
        {
            return endsWithDir( dirOf( ing.symbols[ c ].fileId ), pkgDir ) && enclosingSymbol( c ) == kNoNode && !inNamespace( c );   // a top-level class
        } );
    }
    // Java / Kotlin (fix round 2), the order the languages scope a simple type name in: a class nested in a scope that holds
    // the callee, a member type the callee's classes inherit (not resolved: nothing), a single-type import, the file's own
    // top-level class, then the package's top-level classes.
    NodeId jvmResultClass( std::uint32_t fileId, std::uint32_t at, std::string_view whole, std::string_view cls ) const
    {
        if( !whole.empty() )
        {
            return jvmClassIn( whole, cls );
        }
        bool         nested = false;
        const NodeId inner  = inOwnFile( fileId, at, cls, CallResultFamily::Jvm, nested, true );
        if( nested )
        {
            return inner;
        }
        if( inheritsNested( fileId, at, cls ) )
        {
            return kNoNode;
        }
        bool   imported = false;
        NodeId hit      = kNoNode;
        forEachImport( fileId, cls, [ & ]( const Binding& b )
        {
            if( b.kind == LocalBindKind::TypeImport )
            {
                hit      = imported ? kNoNode : jvmClassIn( b.importedName, b.typeName );
                imported = true;
            }
        } );
        if( imported )
        {
            return hit;
        }
        bool         own   = false;
        const NodeId local = inOwnFile( fileId, at, cls, CallResultFamily::Jvm, own );
        if( own )
        {
            return local;
        }
        const std::string_view dir = dirOf( fileId );
        return onlyClass( cls, CallResultFamily::Jvm, [ & ]( NodeId c ) { return dirOf( ing.symbols[ c ].fileId ) == dir && visibleAt( c, fileId, at ); } );
    }
    // D3: does file `fileId` import the simple name `name` (a single-type import, Kotlin's `as` alias included) from OUTSIDE
    // the tree — a path no package the tree DECLARES leads (`package` records: a Kotlin file's package need not match its
    // directory), read at any depth: `lib.Outer.Inner` is in tree when `lib` is declared and holds a class `Outer`. A
    // class-name receiver spelled `name` is then that outside class, never an in-tree namesake.
    bool importsFromOutside( std::uint32_t fileId, std::string_view name ) const
    {
        bool outside = false;
        forEachImport( fileId, name, [ & ]( const Binding& b )
        {
            if( b.kind != LocalBindKind::TypeImport || b.typeName.empty() || b.importedName.find( '.' ) == std::string::npos )
            {
                return;   // a Java / Kotlin import names a dotted path; a root-package name is no outside evidence
            }
            const std::string_view path = b.importedName;
            bool                   inTree = false;
            for( std::size_t dot = path.rfind( '.' ); dot != std::string_view::npos && dot > 0 && !inTree; dot = path.rfind( '.', dot - 1 ) )
            {
                const std::string_view next = namesplit::afterLast( path.substr( 0, path.find( '.', dot + 1 ) ), "." );   // the class the package holds
                NodeId one = kNoNode;
                inTree = jvmPackages.contains( std::string( path.substr( 0, dot ) ) )
                      && classesNamed( next, CallResultFamily::Jvm, []( NodeId ) { return true; }, one ) > 0;
            }
            outside = outside || !inTree;
        } );
        return outside;
    }

    // TypeScript: a name imported by name (`import { T }`, `import { U as T }`) names the ONE in-tree class graph.h's import
    // tables pin for it (buildJsImportTables: the module resolved, the export found) — an outside module, a type-only import
    // the tables do not pin, or a re-export they cannot follow names nothing; else a class of the file itself; else nothing:
    // a module neighbour is never in scope without an import, and an unimported name is a global (`Response`). A class inside
    // a `namespace` (a Namespace record) is in scope only within it. A namespace-qualified `ns.T` is not resolved (a floor).
    NodeId tsResultClass( std::uint32_t fileId, std::uint32_t at, std::string_view whole, std::string_view cls ) const
    {
        if( !whole.empty() )
        {
            return kNoNode;
        }
        bool imported = false;
        forEachImport( fileId, cls, [ & ]( const Binding& b ) { imported = imported || b.kind == LocalBindKind::JsImport; } );
        if( imported )
        {
            if( jsPinnedClasses == nullptr )
            {
                return kNoNode;
            }
            aliasKey.assign( std::to_string( fileId ) ).append( 1, '#' ).append( cls );
            const auto it = jsPinnedClasses->find( aliasKey );
            return it == jsPinnedClasses->end() ? kNoNode : it->second;
        }
        bool own = false;
        return inOwnFile( fileId, at, cls, CallResultFamily::TypeScript, own );
    }

    // Rust: the crate layout (RustLayout above), then a path's module seen from a file — `crate::`, `self::`, `super::`, an
    // in-tree crate's name, a `use`d module, or a child module of the file's own; any other root (`std`, an outside crate)
    // leaves the tree.
    void buildRustLayout() const
    {
        rust.built = true;
        const std::size_t files = ing.files.size();
        rust.crateOf.assign( files, kNoCrate );
        rust.moduleOf.assign( files, {} );
        HashMap<std::string, std::uint32_t> rootIndex;   // crate root directory → crate index
        for( std::uint32_t f = 0; f < files; ++f )
        {
            const std::string_view path = rootRelPath( ing, f );
            if( path.ends_with( "lib.rs" ) || path.ends_with( "main.rs" ) )
            {
                const std::string_view base = path.substr( includerDir( path ).empty() ? 0 : includerDir( path ).size() + 1 );
                if( base != "lib.rs" && base != "main.rs" )
                {
                    continue;
                }
                const std::string dir( includerDir( path ) );
                if( rootIndex.try_emplace( dir, std::uint32_t( rootIndex.size() ) ).second )
                {
                    // the crate's name: its package directory (`crates/mdbook-core/src` → mdbook_core), `-` read as `_`
                    std::string_view pkg = dir;
                    if( pkg == "src" || pkg.ends_with( "/src" ) )
                    {
                        pkg = includerDir( pkg );
                    }
                    std::string name( pkg.substr( includerDir( pkg ).empty() ? 0 : includerDir( pkg ).size() + 1 ) );
                    std::replace( name.begin(), name.end(), '-', '_' );
                    if( !name.empty() )
                    {
                        const auto [ it, fresh ] = rust.crateByName.try_emplace( name, rootIndex[ dir ] );
                        if( !fresh )
                        {
                            it->second = kNoCrate;   // two crates of one name: the name decides nothing
                        }
                    }
                }
            }
        }
        for( std::uint32_t f = 0; f < files; ++f )
        {
            const std::string_view path = rootRelPath( ing, f );
            if( !path.ends_with( ".rs" ) )
            {
                continue;
            }
            for( std::string_view dir = includerDir( path );; dir = includerDir( dir ) )
            {
                if( const auto it = rootIndex.find( std::string( dir ) ); it != rootIndex.end() )
                {
                    std::string module( path.substr( dir.empty() ? 0 : dir.size() + 1 ) );
                    module.resize( module.size() - 3 );   // ".rs"
                    if( module == "lib" || module == "main" || module == "mod" )
                    {
                        module.clear();
                    }
                    else if( module.ends_with( "/mod" ) )
                    {
                        module.resize( module.size() - 4 );
                    }
                    for( std::size_t at = module.find( '/' ); at != std::string::npos; at = module.find( '/', at + 2 ) )
                    {
                        module.replace( at, 1, "::" );
                    }
                    rust.crateOf[ f ]  = it->second;
                    rust.moduleOf[ f ] = module;
                    rust.filesOf[ std::to_string( it->second ) + "#" + module ].push_back( f );
                    break;
                }
                if( dir.empty() )
                {
                    break;
                }
            }
        }
    }
    // `a::b` + `c` → `a::b::c`; the crate root ("") + `c` → `c`
    static std::string joinModule( std::string_view module, std::string_view seg )
    {
        return module.empty() ? std::string( seg ) : std::string( module ).append( "::" ).append( seg );
    }
    // "<crate>#<module>", RustLayout::filesOf's key, built in the reused buffer
    const std::string& rustModuleKey( std::uint32_t crate, std::string_view module ) const
    {
        moduleKey.assign( std::to_string( crate ) ).append( 1, '#' ).append( module );
        return moduleKey;
    }
    bool rustModuleExists( std::uint32_t crate, const std::string& module ) const
    {
        return rust.filesOf.contains( rustModuleKey( crate, module ) );
    }
    // the (crate, module) `modPath` (`crate::model`, `super::x`, `mdbook_core::book`, a `use`d `model`) names from file `f`
    bool rustModuleOf( std::uint32_t f, std::string_view modPath, std::uint32_t& crate, std::string& module, int depth ) const
    {
        if( !rust.built )
        {
            buildRustLayout();
        }
        if( f >= rust.crateOf.size() || rust.crateOf[ f ] == kNoCrate || modPath.empty() || depth > 4 )
        {
            return false;
        }
        std::vector<std::string_view> segs;
        for( std::size_t at = 0;; )
        {
            const std::size_t sep = modPath.find( "::", at );
            segs.push_back( modPath.substr( at, sep == std::string_view::npos ? std::string_view::npos : sep - at ) );
            if( sep == std::string_view::npos )
            {
                break;
            }
            at = sep + 2;
        }
        crate  = rust.crateOf[ f ];
        module = rust.moduleOf[ f ];
        std::size_t i = 1;
        if( segs[ 0 ] == "crate" )
        {
            module.clear();
        }
        else if( segs[ 0 ] == "super" )
        {
            for( i = 0; i < segs.size() && segs[ i ] == "super"; ++i )
            {
                if( module.empty() )
                {
                    return false;
                }
                const std::size_t up = module.rfind( "::" );
                module.resize( up == std::string::npos ? 0 : up );
            }
        }
        else if( segs[ 0 ] != "self" )
        {
            // a `use` binding of the name shadows a crate of that name; two `use`s of one name decide nothing
            std::size_t uses = 0;
            std::string usedPath;
            forEachImport( f, segs[ 0 ], [ & ]( const Binding& b )
            {
                if( b.kind == LocalBindKind::TypeImport )
                {
                    ++uses;
                    usedPath = b.importedName;
                }
            } );
            const auto byName = rust.crateByName.find( std::string( segs[ 0 ] ) );
            if( uses > 1 )
            {
                return false;
            }
            if( uses == 1 )
            {
                if( !rustModuleOf( f, usedPath, crate, module, depth + 1 ) )
                {
                    return false;
                }
            }
            else if( byName != rust.crateByName.end() && byName->second != kNoCrate )
            {
                crate = byName->second;
                module.clear();
            }
            else if( rustModuleExists( crate, joinModule( module, segs[ 0 ] ) ) )
            {
                module = joinModule( module, segs[ 0 ] );
            }
            else
            {
                return false;   // `std`, an outside crate, an unknown name
            }
        }
        for( ; i < segs.size(); ++i )
        {
            module = joinModule( module, segs[ i ] );
        }
        return rustModuleExists( crate, module );
    }
    // the ONE Rust class `cls` module `modPath` (seen from `f`) defines — or re-exports: a module file's own `use` of the
    // name (`pub use mdbook::MDBook;` in lib.rs) is followed to the module it names (depth-bounded)
    NodeId rustClassAt( std::uint32_t f, std::string_view modPath, std::string_view cls, int depth = 0 ) const
    {
        std::uint32_t crate = kNoCrate;
        std::string   module;
        if( depth > 4 || !rustModuleOf( f, modPath, crate, module, 0 ) )
        {
            return kNoNode;
        }
        NodeId            one = kNoNode;
        const std::size_t n   = classesNamed( cls, CallResultFamily::Rust, [ & ]( NodeId c )
        {
            const std::uint32_t cf = ing.symbols[ c ].fileId;   // an item of the module file itself, never of an inline `mod` in it
            return cf < rust.crateOf.size() && rust.crateOf[ cf ] == crate && rust.moduleOf[ cf ] == module && enclosingSymbol( c ) == kNoNode;
        }, one );
        return n > 0 ? ( n == 1 ? one : kNoNode ) : rustReexport( crate, module, cls, depth );
    }
    NodeId rustReexport( std::uint32_t crate, const std::string& module, std::string_view cls, int depth ) const
    {
        const auto files = rust.filesOf.find( rustModuleKey( crate, module ) );
        NodeId     hit   = kNoNode;
        bool       seen  = false;
        if( files == rust.filesOf.end() )
        {
            return kNoNode;
        }
        for( std::uint32_t mf : files->second )   // by reference: the layout's own list, never a copy
        {
            forEachImport( mf, cls, [ & ]( const Binding& b )
            {
                if( b.kind == LocalBindKind::TypeImport )
                {
                    const auto [ modPath, item ] = splitLast( b.importedName, "::" );
                    const NodeId c = modPath.empty() ? kNoNode : rustClassAt( mf, modPath, item, depth + 1 );
                    hit  = seen ? kNoNode : c;
                    seen = true;
                }
            } );
        }
        return hit;
    }
    // (L) a qualified Rust call whose qualifier is a module path reaching `target`'s own module (`model::load()`,
    // `crate::nest::holder()`, a `use`d `nest::holder()`): the evidence the canonical tier, keyed by type scopes, lacks
    bool rustQualifierReaches( const Reference& r, NodeId target ) const
    {
        std::uint32_t crate = kNoCrate;
        std::string   module;
        if( callResultFamilyOf( r.lang ) != CallResultFamily::Rust || r.qualifier.empty() || !rustModuleOf( r.fileId, r.qualifier, crate, module, 0 ) )
        {
            return false;
        }
        const std::uint32_t tf = ing.symbols[ target ].fileId;
        return tf < rust.crateOf.size() && rust.crateOf[ tf ] == crate && rust.moduleOf[ tf ] == module;
    }
    // `a::b::C` → ( `a::b`, `C` ); `C` → ( "", `C` ) — the last segment is namesplit::afterLast's
    static std::pair<std::string_view, std::string_view> splitLast( std::string_view path, std::string_view sep ) noexcept
    {
        const std::size_t at = path.rfind( sep );
        return { at == std::string_view::npos ? std::string_view{} : path.substr( 0, at ), namesplit::afterLast( path, sep ) };
    }
    NodeId rustResultClass( std::uint32_t fileId, std::uint32_t at, std::string_view whole, std::string_view cls ) const
    {
        if( !whole.empty() )
        {
            return rustClassAt( fileId, splitLast( whole, "::" ).first, cls );
        }
        bool         own   = false;
        const NodeId local = inOwnFile( fileId, at, cls, CallResultFamily::Rust, own );
        if( own )
        {
            return local;
        }
        bool   used = false;
        NodeId hit  = kNoNode;
        forEachImport( fileId, cls, [ & ]( const Binding& b )
        {
            if( b.kind == LocalBindKind::TypeImport )
            {
                const auto [ modPath, item ] = splitLast( b.importedName, "::" );
                hit  = used ? kNoNode : rustClassAt( fileId, modPath, item );
                used = true;
            }
        } );
        if( used )
        {
            return hit;
        }
        // globs (`use super::*;`): every one must stay in the tree, and exactly one class of the name across them
        NodeId globHit = kNoNode;
        bool   decided = true;
        forEachImport( fileId, "*", [ & ]( const Binding& b )
        {
            if( b.kind != LocalBindKind::TypeImport )
            {
                return;
            }
            std::uint32_t crate = kNoCrate;
            std::string   module;
            if( !rustModuleOf( fileId, b.importedName, crate, module, 0 ) )
            {
                decided = false;   // an outside glob may supply the name
                return;
            }
            const NodeId c = rustClassAt( fileId, b.importedName, cls );
            if( c != kNoNode )
            {
                decided = decided && globHit == kNoNode;
                globHit = c;
            }
        } );
        return decided ? globHit : kNoNode;
    }

    // C++ / ObjC: Rule 2 keys a member by its class's NAME, so the class must be the ONLY in-tree class definition of that
    // name; a qualified result (`ns::T`) must also name its scope — an enclosing class of that name, the namespace path the
    // class is declared in, or a namespace its file declares non-members in — so an outside `ext::Config` never names the
    // in-tree `cfg::Config`. An unqualified result is cppUnqualifiedClass's.
    NodeId cppResultClass( std::string_view whole, std::string_view cls ) const
    {
        if( classIds == nullptr )
        {
            return kNoNode;
        }
        const auto it = classIds->classesByName.find( std::string( cls ) );
        if( it == classIds->classesByName.end() || it->second.size() != 1 )
        {
            return kNoNode;
        }
        const NodeId c = it->second[ 0 ];
        const auto [ scope, last ] = splitLast( whole, "::" );
        if( last != cls || scope.empty() )
        {
            return kNoNode;
        }
        const NodeId outer = c < classIds->enclosingClass.size() ? classIds->enclosingClass[ c ] : kNoNode;
        if( outer != kNoNode && ing.symbols[ outer ].name == splitLast( scope, "::" ).second )
        {
            return c;
        }
        // (fix round 2) the class's own namespace path (its file's Namespace records) ends with the written scope,
        // segment-aligned: `net::wire::Frame` or `wire::Frame` for a Frame declared in namespace net::wire
        std::vector<std::string_view> classPath;
        std::vector<std::string_view> written;
        namespacePathAt( ing.symbols[ c ].fileId, ing.symbols[ c ].sigStartByte, classPath );
        appendSegments( scope, written );
        if( outer == kNoNode && written.size() <= classPath.size() && std::equal( written.rbegin(), written.rend(), classPath.rbegin() ) )
        {
            return c;
        }
        std::string nsKey;
        buildShadowKey( nsKey, ing.symbols[ c ].fileId, scope );
        return classIds->fileNamespaces.contains( nsKey ) ? c : kNoNode;
    }
    // `a::b::c` → a, b, c appended to `path`
    static void appendSegments( std::string_view qualified, std::vector<std::string_view>& path )
    {
        for( std::string_view rest = qualified;; )
        {
            const std::size_t sep = rest.find( "::" );
            path.push_back( rest.substr( 0, sep ) );
            if( sep == std::string_view::npos )
            {
                return;
            }
            rest.remove_prefix( sep + 2 );
        }
    }
    // C++ (fix round 2): the namespace path at byte `at` of file `f` — the Namespace records holding it, outermost first,
    // each name split at `::` (`namespace a::b` is two)
    void namespacePathAt( std::uint32_t f, std::uint32_t at, std::vector<std::string_view>& path ) const
    {
        path.clear();
        for( const Binding* ns : namespacesOf( f ) )
        {
            if( ns->spanStart <= at && at < ns->spanEnd )
            {
                appendSegments( ns->var, path );
            }
        }
    }
    static bool isPrefixPath( const std::vector<std::string_view>& head, const std::vector<std::string_view>& path ) noexcept
    {
        return head.size() <= path.size() && std::equal( head.begin(), head.end(), path.begin() );
    }
    // the namespace- and file-scope usings in force at byte `at` of file `f`: the file's own, declared before `at` in a scope
    // that holds it, and those of every in-tree header `f` includes (transitively) whose namespace path leads to `at`'s
    // (`calleePath`) — a header's `namespace cfg { using ext::Config; }` is in force in `namespace cfg` of the includer
    template <class Fn>
    void forEachCppUsing( std::uint32_t f, std::uint32_t at, const std::vector<std::string_view>& calleePath, Fn&& fn ) const
    {
        const auto rowsOf = [ & ]( std::uint32_t file, bool own )
        {
            if( file >= importsByFile.size() )
            {
                return;
            }
            for( const Binding* b : importsByFile[ file ] )
            {
                if( !isCppUsing( *b ) )
                {
                    continue;
                }
                if( own && !( b->startByte < at && b->spanStart <= at && at < b->spanEnd ) )
                {
                    continue;   // declared after the callee, or in a namespace that does not hold it
                }
                if( !own )
                {
                    namespacePathAt( file, b->startByte, usingPath );
                    if( !isPrefixPath( usingPath, calleePath ) )
                    {
                        continue;
                    }
                }
                fn( *b );
            }
        };
        rowsOf( f, true );
        if( fileIncludes == nullptr || f >= fileIncludes->size() )
        {
            return;
        }
        const std::vector<NodeId>& inc = ( *fileIncludes )[ f ];
        if( inc.size() < cppUsingFiles.size() )
        {
            for( NodeId h : inc )
            {
                if( h != f && std::binary_search( cppUsingFiles.begin(), cppUsingFiles.end(), std::uint32_t( h ) ) )
                {
                    rowsOf( std::uint32_t( h ), false );
                }
            }
            return;
        }
        for( std::uint32_t h : cppUsingFiles )
        {
            if( h != f && std::binary_search( inc.begin(), inc.end(), NodeId( h ) ) )
            {
                rowsOf( h, false );
            }
        }
    }
    // C++ (fix round 2): an UNQUALIFIED result `T` declared at byte `at` of file `f` names the ONLY in-tree class `T` only
    // where C++'s own lookup reaches it: a using-declaration of `T` in force there decides (its path resolves as a written
    // `ns::T` would — an outside `using ext::T;` names nothing); else a nested or local class must enclose the declaration
    // (visibleAt), and a namespace-scope class's namespace must enclose the declaration's own namespace or be named by a
    // using-directive in force there. An outside using-directive cannot make the in-tree class wrong — `T` would then be
    // ambiguous — so it never needs a refusal of its own.
    NodeId cppUnqualifiedClass( std::uint32_t f, std::uint32_t at, std::string_view cls ) const
    {
        if( classIds == nullptr )
        {
            return kNoNode;
        }
        const auto it = classIds->classesByName.find( std::string( cls ) );
        if( it == classIds->classesByName.end() || it->second.size() != 1 )
        {
            return kNoNode;
        }
        const NodeId c = it->second[ 0 ];
        std::vector<std::string_view> calleePath;
        namespacePathAt( f, at, calleePath );
        bool   declared = false;
        NodeId byDecl   = kNoNode;
        forEachCppUsing( f, at, calleePath, [ & ]( const Binding& b )
        {
            if( b.var == cls )
            {
                const NodeId named = cppResultClass( b.importedName, cls );
                byDecl   = declared && named != byDecl ? kNoNode : named;
                declared = true;
            }
        } );
        if( declared )
        {
            return byDecl;
        }
        if( enclosingSymbol( c ) != kNoNode )
        {
            return visibleAt( c, f, at ) ? c : kNoNode;
        }
        std::vector<std::string_view> classPath;
        namespacePathAt( ing.symbols[ c ].fileId, ing.symbols[ c ].sigStartByte, classPath );
        if( isPrefixPath( classPath, calleePath ) )
        {
            return c;
        }
        bool named = false;
        forEachCppUsing( f, at, calleePath, [ & ]( const Binding& b )
        {
            if( b.var != "*" || named )
            {
                return;
            }
            // `using namespace a::b;` names the class's path itself, or its tail relative to a namespace enclosing the callee
            std::vector<std::string_view> dir;
            appendSegments( b.importedName, dir );
            if( dir.size() <= classPath.size() && std::equal( dir.rbegin(), dir.rend(), classPath.rbegin() ) )
            {
                const std::vector<std::string_view> head( classPath.begin(), classPath.end() - std::ptrdiff_t( dir.size() ) );
                named = isPrefixPath( head, calleePath );
            }
        } );
        return named ? c : kNoNode;
    }

    // the class a ReturnType record's written type resolves to in its own (the callee's) file — kNoNode for none or several
    NodeId resultClassOf( const Binding& rt, Lang lang ) const
    {
        switch( callResultFamilyOf( lang ) )
        {
            case CallResultFamily::Go:         return goResultClass( rt.fileId, rt.startByte, rt.importedName, rt.typeName );
            case CallResultFamily::Jvm:        return jvmResultClass( rt.fileId, rt.startByte, rt.importedName, rt.typeName );
            case CallResultFamily::TypeScript: return tsResultClass( rt.fileId, rt.startByte, rt.importedName, rt.typeName );
            case CallResultFamily::Rust:       return rustResultClass( rt.fileId, rt.startByte, rt.importedName, rt.typeName );
            case CallResultFamily::Cpp:        return rt.importedName.empty() ? cppUnqualifiedClass( rt.fileId, rt.startByte, rt.typeName )
                                                                              : cppResultClass( rt.importedName, rt.typeName );
            default:                           return kNoNode;
        }
    }

    // the in-tree class-likes of `cls`'s language sharing its name (cls included)
    std::size_t sameNamedClasses( NodeId cls ) const
    {
        NodeId one = kNoNode;
        return classesNamed( ing.symbols[ cls ].name, callResultFamilyOf( ing.symbols[ cls ].lang ), []( NodeId ) { return true; }, one );
    }
};

struct ReceiverEvidence
{
    static constexpr std::size_t kWalkCap = 64;   // classes visited per base walk; deterministic (sorted chaUp)

    const IngestResult&                                   ing;
    const HashMap<std::string, std::vector<std::string>>& chaUp;       // class name → its direct base names (sorted)
    const HashMap<std::string, char>&                     classNames;  // every in-repo class-like name
    std::vector<std::string>                              ownerClass;  // per symbol: the class that owns it, "" none
    std::vector<std::uint8_t>                             staticMember;    // per symbol: 1 = a JS/TS `static` member (the CLASS side)
    HashMap<std::string, rw::SmallVec<NodeId, 2>>         methodsByType;   // "Class::name" → its callables
    HashMap<std::string, std::string>                     memberType;      // "Class#field" → field class ("" = tombstone)
    HashMap<std::string, std::vector<std::string>>        embeds;          // "Class" → its other bases: Go embedded classes (promotion)
    HashMap<std::string, std::string>                     nameAlias;       // "<fileId>#local" → the class name it imports
    // FE-B (review B4): one binding of a local name — the class it names ("" = unknown: it hides an outer binding of the
    // name and proves nothing) and the bytes it is visible in. end == 0: a record with no span (a Python name, a reassignment,
    // a base pass Type record): somewhere in its definition, extent unknown.
    struct LocalFact
    {
        std::uint32_t start = 0;
        std::uint32_t end   = 0;
        std::string   type;
        NodeId        cls   = kNoNode;   // (L) fix round 1: the ONE in-tree class `type` resolved to (kNoNode: the name alone)
        bool operator==( const LocalFact& ) const = default;
    };
    HashMap<std::string, std::vector<LocalFact>>          localType;       // "<fromSymbol>#var" → its bindings
    HashMap<std::string, std::pair<std::string, std::string>> methodAlias; // "<fromSymbol>#var" → ( object var, method )
    HashMap<std::string, std::string>                     moduleAlias;     // "<fileId>#name" → module as written ("" = tombstone)
    std::vector<std::vector<NodeId>>                      fnsByFile;       // fileId → functions/methods sorted by start (enclosing walk)
    std::vector<std::vector<NodeId>>                      classesByFile;   // fileId → class-like symbols sorted by start
    mutable std::string                                   key;
    mutable std::vector<std::string_view>                 frontier;
    mutable rw::SmallVec<NodeId, 2>                       found;
    bool                                                  active = false;
    const HashMap<std::string, char>*                     localNames = nullptr;   // graph.h FieldNarrowTables::localNameSet
    const GoPackagePaths*                                 goPackages = nullptr;   // graph.h FalseEdgeRules::goPackages
    HashMap<std::string, std::uint32_t>                   callResultDecls;          // "<scope>#var" → its CallResult records (typeCallResult)
    CallResultTypes                                       types;                    // (L) fix round 1: a declared result type's ONE class

    ReceiverEvidence( const IngestResult& i, const HashMap<std::string, std::vector<std::string>>& up, const HashMap<std::string, char>& classes )
        : ing( i ), chaUp( up ), classNames( classes ), types( i ) {}

    static bool classLike( SymKind k ) noexcept
    {
        return k == SymKind::Class || k == SymKind::Struct || k == SymKind::Interface;
    }
    static bool callableKind( SymKind k ) noexcept
    {
        return k == SymKind::Method || k == SymKind::Function;
    }
    const std::string& keyOf( std::uint32_t id, char sep, std::string_view name ) const
    {
        key.clear();
        key.append( std::to_string( id ) );
        key.push_back( sep );
        key.append( name );
        return key;
    }
    const std::string& typeKey( std::string_view type, std::string_view sep, std::string_view name ) const
    {
        key.assign( type );
        key.append( sep );
        key.append( name );
        return key;
    }

    // ── build ─────────────────────────────────────────────────────────────────────────────────────────────────────────
    static void tombstoneInsert( HashMap<std::string, std::string>& m, const std::string& k, std::string_view v )
    {
        const auto [ it, fresh ] = m.try_emplace( k, std::string( v ) );
        if( !fresh && it->second != v )
        {
            it->second.clear();   // two different classes for one name: decide nothing
        }
    }

    void addLocalFact( const std::string& k, const Binding& b ) { addTypedFact( k, b, b.typeName, kNoNode ); }
    // the same fact naming `type` instead of the binding's own (a qualified written type refused: ""), and the one class it
    // resolved to (kNoNode: the name alone)
    void addTypedFact( const std::string& k, const Binding& b, std::string_view type, NodeId cls )
    {
        const bool                  spanned = b.spanEnd > b.spanStart;
        LocalFact                   f{ spanned ? b.spanStart : 0u, spanned ? b.spanEnd : 0u, std::string( type ), cls };
        std::vector<LocalFact>&     facts = localType[ k ];
        if( std::none_of( facts.begin(), facts.end(), [ & ]( const LocalFact& g ) { return g == f; } ) )
        {
            facts.push_back( std::move( f ) );
        }
    }

    void buildOwners()
    {
        const std::size_t N = ing.symbols.size();
        ownerClass.assign( N, {} );
        // the classes of each file by start; a symbol's owner is the innermost class whose span holds it
        classesByFile.assign( ing.files.size(), {} );
        fnsByFile.assign( ing.files.size(), {} );
        for( const Symbol& s : ing.symbols )
        {
            if( s.fileId >= ing.files.size() )
            {
                continue;
            }
            if( classLike( s.kind ) )
            {
                classesByFile[ s.fileId ].push_back( s.id );
            }
            if( classLike( s.kind ) || s.kind == SymKind::NamedType )
            {
                types.noteClass( s );   // a Go `type N int` has methods too: a result or field type may name it
            }
            if( callableKind( s.kind ) )
            {
                fnsByFile[ s.fileId ].push_back( s.id );
            }
        }
        const auto byStart = [ & ]( NodeId a, NodeId b ) { return ing.symbols[ a ].sigStartByte != ing.symbols[ b ].sigStartByte
                                                                  ? ing.symbols[ a ].sigStartByte < ing.symbols[ b ].sigStartByte : a < b; };
        for( std::vector<NodeId>& v : classesByFile ) { std::sort( v.begin(), v.end(), byStart ); }
        for( std::vector<NodeId>& v : fnsByFile )     { std::sort( v.begin(), v.end(), byStart ); }
        for( const Symbol& s : ing.symbols )
        {
            if( !s.scope.empty() )
            {
                ownerClass[ s.id ] = s.scope;   // the extractor's own scope (Python, C++, Kotlin, Ruby, JS prototype members)
                continue;
            }
            if( s.fileId >= ing.files.size() || classLike( s.kind ) )
            {
                continue;
            }
            if( const NodeId best = classAt( s.fileId, s.sigStartByte, s.endByte, s.id ); best != kNoNode )
            {
                ownerClass[ s.id ] = ing.symbols[ best ].name;
            }
        }
        // Go spells a method's class only on its receiver: the RecvType record that sets isFromAssignment
        for( const Binding& b : ing.bindings )
        {
            if( b.kind == LocalBindKind::RecvType && b.isFromAssignment && b.fromSymbol < N && !b.typeName.empty() )
            {
                ownerClass[ b.fromSymbol ] = b.typeName;
            }
        }
        ENSURES( ownerClass.size() == N && fnsByFile.size() == ing.files.size() && classesByFile.size() == ing.files.size(),
                 "one owner per symbol, one sorted list per file" );
    }

    // the innermost class-like symbol of `fileId` whose span holds [start, end) (never `self`); kNoNode when none
    NodeId classAt( std::uint32_t fileId, std::uint32_t start, std::uint32_t end, NodeId self = kNoNode ) const
    {
        NodeId best = kNoNode;
        if( fileId >= classesByFile.size() )
        {
            return best;
        }
        for( NodeId c : classesByFile[ fileId ] )
        {
            const Symbol& cs = ing.symbols[ c ];
            if( cs.sigStartByte > start )
            {
                break;
            }
            if( cs.id != self && cs.endByte >= end )
            {
                best = c;   // later starts are more inner
            }
        }
        return best;
    }
    void addBase( const std::string& owner, std::string_view base )
    {
        std::vector<std::string>& e = embeds[ owner ];
        if( std::find( e.begin(), e.end(), base ) == e.end() )
        {
            e.emplace_back( base );
            std::sort( e.begin(), e.end() );
        }
    }
    void build()
    {
        buildOwners();
        staticMember.assign( ing.symbols.size(), 0u );
        for( const Symbol& s : ing.symbols )
        {
            // a BODIED callable only: an interface's or an abstract class's bodyless signature is the contract a call
            // dispatches through, never the body it reaches — that call stays with the ladder (declined_iface= reads it)
            if( callableKind( s.kind ) && !ownerClass[ s.id ].empty() && receiverHedgeLang( s.lang ) && s.endByte > s.sigEndByte )
            {
                methodsByType[ typeKey( ownerClass[ s.id ], "::", s.name ) ].push_back( s.id );
            }
        }
        const std::size_t N = ing.symbols.size();
        std::vector<const Binding*> qualified;   // locals, parameters and fields written `pkg.T`: decided once every import alias is known
        types.importsByFile.assign( ing.files.size(), {} );
        types.moduleAlias = &moduleAlias;
        types.goPackages  = goPackages;
        for( const Binding& b : ing.bindings )
        {
            noteCallResultFacts( b );
            if( b.kind == LocalBindKind::ModuleAlias && b.fromSymbol == kNoNode && !b.var.empty() && !b.isFromAssignment )
            {
                tombstoneInsert( moduleAlias, keyOf( b.fileId, '#', b.var ), b.typeName );
                continue;
            }
            if( b.kind == LocalBindKind::NameAlias && !b.var.empty() )
            {
                tombstoneInsert( nameAlias, keyOf( b.fileId, '#', b.var ), b.typeName );
                continue;
            }
            if( b.kind == LocalBindKind::JsImport && !b.var.empty() && !b.importedName.empty() && b.importedName != b.var )
            {
                tombstoneInsert( nameAlias, keyOf( b.fileId, '#', b.var ), b.importedName );   // `import { Context as Ctx }`
                continue;
            }
            if( b.fromSymbol >= N )
            {
                continue;
            }
            const NodeId from = factScope( b );
            const bool typedLocal = b.kind == LocalBindKind::RecvType
                                 || ( b.kind == LocalBindKind::Type && ( ing.symbols[ b.fromSymbol ].lang == Lang::TypeScript
                                                                         || ing.symbols[ b.fromSymbol ].lang == Lang::Python ) );
            if( b.kind == LocalBindKind::StaticMember )
            {
                if( ing.symbols[ from ].name == b.var )   // recorded inside the member's body: the member itself
                {
                    staticMember[ from ] = 1;
                }
            }
            else if( ( b.kind == LocalBindKind::RecvType || b.kind == LocalBindKind::MemberType ) && !b.var.empty() && !b.importedName.empty() )
            {
                qualified.push_back( &b );
            }
            else if( typedLocal && !b.var.empty() )
            {
                addLocalFact( keyOf( from, '#', b.var ), b );
            }
            else if( b.kind == LocalBindKind::MemberType && !b.var.empty() )
            {
                noteMemberType( b, b.typeName );
            }
            else if( b.kind == LocalBindKind::MethodAlias && !b.var.empty() )
            {
                const auto [ it, fresh ] = methodAlias.try_emplace( keyOf( from, '#', b.var ), b.typeName, b.importedName );
                if( !fresh && ( it->second.first != b.typeName || it->second.second != b.importedName ) )
                {
                    it->second.first.clear();   // rebound to something else: decide nothing
                }
            }
        }
        types.finishIndex();
        for( const Binding* q : qualified )
        {
            noteQualifiedWritten( *q );
        }
        active = true;
    }
    // (L) the call-result facts one binding feeds: the file's imports (CallResultTypes) and the function's call-initialised
    // bindings of each name (typeCallResult's TS guess rule)
    void noteCallResultFacts( const Binding& b )
    {
        types.noteImport( b );
        if( b.kind == LocalBindKind::CallResult && b.fromSymbol < ing.symbols.size() && !b.var.empty() )
        {
            ++callResultDecls[ keyOf( factScope( b ), '#', b.var ) ];
        }
    }
    // a field's class on its owner (`type` "": unknown, a tombstone); an embedded field (Go) is also a base
    void noteMemberType( const Binding& b, std::string_view type )
    {
        const Symbol&      from  = ing.symbols[ b.fromSymbol ];
        const std::string& owner = classLike( from.kind ) ? from.name : ownerClass[ b.fromSymbol ];
        if( owner.empty() )
        {
            return;
        }
        tombstoneInsert( memberType, typeKey( owner, "#", b.var ), type );
        if( b.isFromAssignment && !type.empty() )
        {
            addBase( owner, type );
        }
    }
    // a local, parameter or field written `pkg.T` (the Go capture is its only writer: importedName = the qualified text) names
    // a class of THIS tree only when `pkg` imports an in-tree package defining it (CallResultTypes::goQualifiedClass): `f
    // *os.File` beside an in-tree `File` is os's File, never ours (test/receiverevidencecheck.sh (L) NearOSParam, NearOSField).
    // Anything else keeps the name as a tombstone: it proves nothing. A local carries the class it resolved to.
    void noteQualifiedWritten( const Binding& q )
    {
        const NodeId           cls  = types.goQualifiedClass( q.fileId, q.importedName, q.typeName );
        const std::string_view type = cls != kNoNode ? std::string_view( q.typeName ) : std::string_view{};
        if( q.kind == LocalBindKind::MemberType )
        {
            noteMemberType( q, type );
            return;
        }
        addTypedFact( keyOf( factScope( q ), '#', q.var ), q, type, cls );
    }
    // the definition a local binding belongs to: a typed local the extractor also indexed as a symbol of its own (a Go
    // `var c T` inside a function) belongs to the function that declares it
    NodeId factScope( const Binding& b ) const
    {
        NodeId from = b.fromSymbol;
        if( from < ing.symbols.size() && !callableKind( ing.symbols[ from ].kind ) && !classLike( ing.symbols[ from ].kind ) )
        {
            if( const NodeId fn = enclosingFn( from ); fn != kNoNode )
            {
                from = fn;
            }
        }
        return from;
    }
    // (L) call-result typing: the local `b` (a LocalBindKind::CallResult) holds class `cls` (the definition `id`) over its
    // span — the class REPLACES the empty tombstone its own declaration recorded there (the same span), so no other binding
    // of the name changes. TS: the declaration records no tombstone; its base-pass Type record (unspanned) read the call
    // initializer as a constructor naming the CALLEE (`const c = makeConfig()` → `makeConfig`). When no class has that name
    // that guess is replaced — but an unspanned record has no span to tell two declarations apart, so only when this is the
    // function's ONLY call-initialised binding of the name (callResultDecls): sibling blocks' `const c = a.make()` / `const c
    // = b.make()` type neither. False when nothing was replaced.
    bool typeCallResult( const Binding& b, std::string_view cls, NodeId id )
    {
        const auto it = localType.find( keyOf( factScope( b ), '#', b.var ) );
        if( it == localType.end() || cls.empty() )
        {
            return false;
        }
        const bool spanned = b.spanEnd > b.spanStart;
        for( LocalFact& f : it->second )
        {
            if( f.start == ( spanned ? b.spanStart : 0u ) && f.end == ( spanned ? b.spanEnd : 0u ) && f.type.empty() )
            {
                f.type.assign( cls );
                f.cls = id;
                return true;
            }
        }
        const auto decls = callResultDecls.find( it->first );
        for( LocalFact& f : it->second )
        {
            if( f.type == b.typeName && !classNames.contains( f.type ) && decls != callResultDecls.end() && decls->second == 1 )
            {
                f.type.assign( cls );
                f.cls = id;
                return true;
            }
        }
        return false;
    }

    // ── queries ───────────────────────────────────────────────────────────────────────────────────────────────────────
    // the innermost function/method of the same file whose span strictly holds `id`; kNoNode when none
    NodeId enclosingFn( NodeId id ) const { return innermostEnclosingFn( ing, fnsByFile, id ); }
    // visit `from`, then each function enclosing it, innermost first (at most 8 scopes): `visit( scope )` returns true
    // to stop there. Returns the scope it stopped at, kNoNode when no visit stopped the walk.
    template <class Visit>
    NodeId walkScopes( NodeId from, Visit&& visit ) const
    {
        NodeId cur = from;
        for( int depth = 0; depth < 8 && cur != kNoNode; ++depth )
        {
            if( visit( cur ) )
            {
                return cur;
            }
            cur = enclosingFn( cur );
        }
        return kNoNode;
    }
    // FE-B (review B4): the class local `var` holds at byte `site` of definition `scope`, by lexical scope — the innermost
    // spanned binding that holds the site (a later start is more inner; a tie of two classes is unknown), checked against
    // every binding with no span (its extent is unknown, so a disagreement is unknown). nullopt: no binding of `var` is
    // visible there; "": one is, and it names no single class. SeenLocal::cls (fix round 1): the ONE class definition the
    // binding resolved to (a call result's), kNoNode for a name alone — two facts that agree on the name but not on the
    // definition disagree.
    struct SeenLocal
    {
        std::string_view type;
        NodeId           cls = kNoNode;
        bool operator==( const SeenLocal& ) const = default;
    };
    std::optional<SeenLocal> seenLocal( NodeId scope, std::string_view var, std::uint32_t site ) const
    {
        const auto it = localType.find( keyOf( scope, '#', var ) );
        if( it == localType.end() )
        {
            return std::nullopt;
        }
        const LocalFact*         inner = nullptr;
        bool                     innerSplit = false;
        std::optional<SeenLocal> whole;
        for( const LocalFact& f : it->second )
        {
            if( f.end == 0 )
            {
                whole = ( whole.has_value() && *whole != SeenLocal{ f.type, f.cls } ) ? SeenLocal{} : SeenLocal{ f.type, f.cls };
                continue;
            }
            if( site < f.start || site >= f.end )
            {
                continue;
            }
            if( inner == nullptr || f.start > inner->start || ( f.start == inner->start && f.end < inner->end ) )
            {
                inner      = &f;
                innerSplit = false;
            }
            else if( f.start == inner->start && f.end == inner->end && SeenLocal{ f.type, f.cls } != SeenLocal{ inner->type, inner->cls } )
            {
                innerSplit = true;
            }
        }
        if( inner == nullptr )
        {
            return whole;
        }
        const SeenLocal seen = innerSplit ? SeenLocal{} : SeenLocal{ inner->type, inner->cls };
        return ( whole.has_value() && *whole != seen ) ? SeenLocal{} : seen;
    }
    static std::optional<std::string_view> typeOf( const std::optional<SeenLocal>& seen ) noexcept
    {
        return seen.has_value() ? std::optional<std::string_view>( seen->type ) : std::nullopt;
    }
    std::optional<std::string_view> visibleLocal( NodeId scope, std::string_view var, std::uint32_t site ) const
    {
        return typeOf( seenLocal( scope, var, site ) );
    }
    // the local `var` visible at byte `site` of `from` or of a function enclosing it (the innermost that has one)
    std::optional<SeenLocal> localSeenAt( NodeId from, std::string_view var, std::uint32_t site ) const
    {
        std::optional<SeenLocal> hit;
        walkScopes( from, [ & ]( NodeId scope )
        {
            hit = seenLocal( scope, var, site );
            return hit.has_value();
        } );
        return hit;
    }
    std::optional<std::string_view> localAt( NodeId from, std::string_view var, std::uint32_t site ) const
    {
        return typeOf( localSeenAt( from, var, site ) );
    }
    // the ( object var, method ) a method alias `var` names in `from` or a function enclosing it; `scope` = where it is bound
    const std::pair<std::string, std::string>* aliasedMethod( NodeId from, std::string_view var, NodeId& scope ) const
    {
        const std::pair<std::string, std::string>* hit = nullptr;
        if( const NodeId at = walkScopes( from, [ & ]( NodeId s )
            {
                const auto it = methodAlias.find( keyOf( s, '#', var ) );
                if( it == methodAlias.end() )
                {
                    return false;
                }
                hit = it->second.first.empty() ? nullptr : &it->second;
                return true;
            } ); at != kNoNode )
        {
            scope = at;
        }
        return hit;
    }
    // is `name` a parameter or local of `from` or of a function enclosing it (any binding the extractor recorded)
    bool isLocalName( NodeId from, std::string_view name ) const
    {
        return localNames != nullptr && walkScopes( from, [ & ]( NodeId s )
        {
            return localNames->contains( keyOf( s, '#', name ) ) || localType.contains( keyOf( s, '#', name ) );
        } ) != kNoNode;
    }
    // does `from` or a function enclosing it DECLARE `name` (a parameter or a local, typed or not) — not merely bind it: Java
    // copies each field name onto its class's methods as a bound-only name (graph.h shadowJavaFieldsOntoMethods), which must
    // not hide the field itself
    bool declaresLocal( NodeId from, std::string_view name, std::uint32_t site ) const
    {
        return walkScopes( from, [ & ]( NodeId s )
        {
            if( visibleLocal( s, name, site ).has_value() )
            {
                return true;   // a binding of the name is visible at the site (review B4: by its span)
            }
            if( localNames == nullptr )
            {
                return false;
            }
            const auto it = localNames->find( keyOf( s, '#', name ) );
            return it != localNames->end() && ( it->second & kLocalNameDeclared ) != 0;
        } ) != kNoNode;
    }
    // a receiver root that names a CLASS at this site: a class-name receiver language, a class of that name, and no local
    // of the caller hiding it (`Interval = make(); Interval.validate( v )`)
    // (L) fix round 2 (D3): a JVM caller's single-type import of `root` from outside the tree makes it that outside class
    bool namesClass( const Reference& r, std::string_view root, std::string_view cls ) const
    {
        return classNameReceiverLang( r.lang ) && classNames.contains( std::string( cls ) ) && !isLocalName( r.fromSymbol, root )
            && !types.importsFromOutside( r.fileId, root );
    }
    // the class owning the caller — the caller's own owner, else the nearest enclosing function's (a closure in a method)
    std::string_view callerClass( NodeId from ) const
    {
        const NodeId at = walkScopes( from, [ & ]( NodeId s ) { return !ownerClass[ s ].empty(); } );
        return at == kNoNode ? std::string_view{} : std::string_view( ownerClass[ at ] );
    }
    // `type`'s direct bases: the inheritance names, then (Go) its embedded classes
    template <class Fn>
    void forEachBase( std::string_view type, Fn&& fn ) const
    {
        if( const auto it = chaUp.find( std::string( type ) ); it != chaUp.end() )
        {
            for( const std::string& b : it->second ) { fn( std::string_view( b ) ); }
        }
        if( const auto it = embeds.find( std::string( type ) ); it != embeds.end() )
        {
            for( const std::string& b : it->second ) { fn( std::string_view( b ) ); }
        }
    }
    // is `ancestor` the class `type` or one of its (transitive) bases
    bool inCone( std::string_view type, std::string_view ancestor ) const
    {
        if( type.empty() || ancestor.empty() )
        {
            return false;
        }
        frontier.clear();
        frontier.push_back( type );
        for( std::size_t i = 0; i < frontier.size() && i < kWalkCap; ++i )
        {
            if( frontier[ i ] == ancestor )
            {
                return true;
            }
            forEachBase( frontier[ i ], [ & ]( std::string_view b )
            {
                if( std::find( frontier.begin(), frontier.end(), b ) == frontier.end() ) { frontier.push_back( b ); }
            } );
        }
        return false;
    }
    // visit `type`'s level (depth 0), then its base levels shallowest first, each class once: `visit( level, depth )`
    // returns true to stop. Returns whether a visit stopped the walk.
    template <class Visit>
    bool walkBaseLevels( std::string_view type, Visit&& visit ) const
    {
        std::vector<std::string_view> level{ type };
        std::vector<std::string_view> seen{ type };
        for( std::size_t depth = 0; depth < kWalkCap && !level.empty(); ++depth )
        {
            if( visit( level, depth ) )
            {
                return true;
            }
            std::vector<std::string_view> next;
            for( std::string_view t : level )
            {
                forEachBase( t, [ & ]( std::string_view b )
                {
                    if( std::find( seen.begin(), seen.end(), b ) == seen.end() ) { seen.push_back( b ); next.push_back( b ); }
                } );
            }
            level.swap( next );
        }
        return false;
    }
    // `type`'s own `name`, else the shallowest base level defining it (several bases at one level: their union). The
    // answer is in `found`; false when nothing in the cone defines it. superOnly skips `type` itself (`super.m()`).
    // `cls` (fix round 1): the ONE definition `type` resolved to — its own level keeps only the members that class owns
    // (memberOwnerRuleOf), and its bases, which are known by NAME only, are walked only when no other in-tree class of its
    // language shares that name.
    bool methodOf( std::string_view type, std::string_view name, bool superOnly, NodeId cls = kNoNode ) const
    {
        EXPECTS( active, "methodOf reads the tables build() fills" );
        found.clear();
        if( type.empty() )
        {
            return false;
        }
        const bool basesByName = cls == kNoNode || types.sameNamedClasses( cls ) == 1;
        const bool stopped     = walkBaseLevels( type, [ & ]( const std::vector<std::string_view>& level, std::size_t depth )
        {
            if( ( superOnly && depth == 0 ) || ( depth > 0 && !basesByName ) )
            {
                return depth > 0;
            }
            for( std::string_view t : level )
            {
                collectMethods( t, name, depth == 0 ? cls : kNoNode );
            }
            return !found.empty();
        } );
        return stopped && !found.empty();   // a walk stopped at an unwalkable base level found nothing
    }
    // `type`'s own callables named `name` into `found` — only those class `cls` owns, when it is a definition
    void collectMethods( std::string_view type, std::string_view name, NodeId cls ) const
    {
        if( const auto it = methodsByType.find( typeKey( type, "::", name ) ); it != methodsByType.end() )
        {
            for( NodeId c : it->second )
            {
                if( cls == kNoNode || ownedBy( c, cls ) )
                {
                    found.push_back( c );
                }
            }
        }
    }
    // does class `cls` own member `m` (memberOwnerRuleOf)
    bool ownedBy( NodeId m, NodeId cls ) const
    {
        const Symbol& ms = ing.symbols[ m ];
        const Symbol& cs = ing.symbols[ cls ];
        switch( memberOwnerRuleOf( cs.lang ) )
        {
            case MemberOwnerRule::PackageDir:   return types.dirOf( ms.fileId ) == types.dirOf( cs.fileId );
            case MemberOwnerRule::FileOrUnique: return ms.fileId == cs.fileId || types.sameNamedClasses( cls ) == 1;
            case MemberOwnerRule::Lexical:
            default:
            {
                const NodeId at = classAt( ms.fileId, ms.sigStartByte, ms.endByte, m );
                return at == cls || ( at == kNoNode && ms.fileId == cs.fileId );
            }
        }
    }
    // the class of field `field` on `type` (or the shallowest base declaring it); "" unknown
    std::string_view fieldOf( std::string_view type, std::string_view field ) const
    {
        std::string_view fieldType;
        walkBaseLevels( type, [ & ]( const std::vector<std::string_view>& level, std::size_t )
        {
            for( std::string_view t : level )
            {
                if( const auto it = memberType.find( typeKey( t, "#", field ) ); it != memberType.end() )
                {
                    fieldType = it->second;   // "" = tombstone: unknown
                    return true;
                }
            }
            return false;
        } );
        return fieldType;
    }

    // The receiver chain of a member call as ( root, path, constructed class ), in every language's spelling.
    struct Chain
    {
        std::string_view root;
        std::string_view path;
        std::string_view ctor;
        bool             valid = false;
    };
    static Chain chainOf( const Reference& r ) noexcept
    {
        Chain c;
        if( r.memberCall )
        {
            c.root  = r.memberRoot;
            c.path  = r.memberPath;
            c.ctor  = r.memberCtor;
            c.valid = !c.root.empty() || !c.ctor.empty();
            return c;
        }
        switch( r.recv )   // Python / C++ / Ruby shapes (ingest_binds.h receiverOf)
        {
            case RecvKind::ThisObj:     c.root = "self";     c.valid = true; break;
            case RecvKind::NamedVar:    c.root = r.recvVar;  c.valid = !r.recvVar.empty(); break;
            case RecvKind::FieldOfThis: c.root = "self";     c.path = r.fieldName; c.valid = !r.fieldName.empty(); break;
            case RecvKind::FieldOfVar:  c.root = r.recvVar;  c.path = r.fieldName; c.valid = !r.recvVar.empty() && !r.fieldName.empty(); break;
            case RecvKind::SuperObj:    c.root = "super";    c.valid = true; break;
            default: break;
        }
        return c;
    }

    // The class the receiver ROOT names at this site; static/super set for a class-name or super receiver. "" unknown.
    // a class name as the caller's FILE spells it, read through that file's import alias (`Ctx` → `Context`)
    std::string_view unalias( std::uint32_t fileId, std::string_view name ) const
    {
        if( const auto it = nameAlias.find( keyOf( fileId, '#', name ) ); it != nameAlias.end() && !it->second.empty() )
        {
            return it->second;
        }
        return name;
    }
    // `rootCls` (when asked, and left as the caller set it otherwise): the ONE class a call-result-typed local resolved to
    // a typed local's class as the caller reads it: a call result's resolved class as it is (its callee's file decided it,
    // so the caller's import aliases do not apply), any other through the caller's import alias
    std::string_view typedLocalClass( const Reference& r, const SeenLocal& t, NodeId* rootCls ) const
    {
        if( rootCls != nullptr && t.cls != kNoNode )
        {
            *rootCls = t.cls;
        }
        return t.cls != kNoNode ? t.type : unalias( r.fileId, t.type );
    }
    std::string_view rootClass( const Reference& r, const Chain& ch, bool& isStatic, bool& superOnly, NodeId* rootCls = nullptr ) const
    {
        isStatic  = false;
        superOnly = false;
        if( !ch.ctor.empty() )
        {
            return unalias( r.fileId, ch.ctor );
        }
        const std::string_view root = ch.root;
        if( isThisRoot( root ) || ( r.lang == Lang::Python && root == "cls" ) )
        {
            return callerClass( r.fromSymbol );
        }
        if( isSuperRoot( r, root ) )
        {
            superOnly = true;
            return callerClass( r.fromSymbol );
        }
        if( const std::optional<SeenLocal> t = localSeenAt( r.fromSymbol, root, r.startByte ); t.has_value() && !t->type.empty() )
        {
            return typedLocalClass( r, *t, rootCls );
        }
        // Java/C#/Kotlin/Swift (the implicit-receiver languages but C++, whose Rule 2b reads fields, and Ruby, whose fields
        // are @ivars): a bare `field.m()` inside a method is `this.field.m()` — the field the caller's class (or a base)
        // declares, when no parameter or local of that name hides it
        if( implicitReceiverLang( r.lang ) && r.lang != Lang::Cpp && r.lang != Lang::Ruby && !declaresLocal( r.fromSymbol, root, r.startByte ) )
        {
            const std::string_view owner = callerClass( r.fromSymbol );
            if( const std::string_view ft = owner.empty() ? std::string_view{} : fieldOf( owner, root ); !ft.empty() )
            {
                return unalias( r.fileId, ft );
            }
        }
        const std::string_view cls = unalias( r.fileId, root );
        if( namesClass( r, root, cls ) )
        {
            isStatic = true;
            return cls;
        }
        return {};
    }

    // The two SIDES of a JS/TS class lookup (#373's Ruby class-object rule, here): a call on the CLASS object reaches its
    // `static` members only, a call on an INSTANCE its other members only. True for every target in another language.
    bool onSide( const Reference& r, NodeId target, bool classSide ) const noexcept
    {
        if( r.lang != Lang::JavaScript && r.lang != Lang::TypeScript )
        {
            return true;
        }
        return target < staticMember.size() && ( staticMember[ target ] != 0 ) == classSide;
    }
    // Does `this` inside the caller name the CLASS? Only inside a static member.
    bool callerIsStatic( NodeId from ) const noexcept
    {
        return from < staticMember.size() && staticMember[ from ] != 0;
    }

    // RESOLVE: the definitions this member call's typed receiver reaches, in `found`; false when the receiver proves
    // nothing (untyped, a type this tree does not define, or a type that defines no such member anywhere in its cone).
    bool narrow( const Reference& r ) const
    {
        // C++, Ruby, Rust and C keep their own receiver rules (Rules 1, 2, 2b, 2c): this one adds the languages that had none
        const bool typedLang = r.lang == Lang::JavaScript || r.lang == Lang::TypeScript || r.lang == Lang::Python || r.lang == Lang::Go
                            || r.lang == Lang::Java || r.lang == Lang::Kotlin || r.lang == Lang::CSharp || r.lang == Lang::Swift;
        const bool localsOnly = receiverLocalsOnly( r.lang );
        if( !active || r.fromSymbol >= ing.symbols.size() || !r.qualifier.empty() || !( typedLang || localsOnly ) )
        {
            return false;
        }
        if( localsOnly )
        {
            return narrowTypedLocal( r );
        }
        if( !isMemberCallRef( r ) )
        {
            return r.lang == Lang::Python && narrowAliasedMethod( r );
        }
        const Chain ch = chainOf( r );
        if( !ch.valid )
        {
            return false;
        }
        bool isStatic = false, superOnly = false;
        NodeId           cls  = kNoNode;
        std::string_view type = rootClass( r, ch, isStatic, superOnly, &cls );
        if( type.empty() || ( cls != kNoNode && !ch.path.empty() && types.sameNamedClasses( cls ) != 1 ) )
        {
            return false;   // a field path is read by NAME (memberType): below a class another in-tree class names too, a floor
        }
        // the side the receiver names: the class object (a class-name receiver, or `this`/`super` inside a static member),
        // else an instance; a stated field below always holds an instance
        bool classSide = isStatic || ( ch.ctor.empty() && ( isThisRoot( ch.root ) || isSuperRoot( r, ch.root ) ) && callerIsStatic( r.fromSymbol ) );
        std::string_view rest = ch.path;
        while( !rest.empty() )   // walk the stated field classes
        {
            const std::size_t dot = rest.find( '.' );
            const std::string_view field = rest.substr( 0, dot );
            rest = dot == std::string_view::npos ? std::string_view{} : rest.substr( dot + 1 );
            type = fieldOf( type, field );
            superOnly = false;
            classSide = false;
            if( type.empty() )
            {
                return false;
            }
        }
        if( !methodOf( type, r.calleeName, superOnly, cls ) )
        {
            return false;
        }
        // a wrong-side definition is never the answer: what is left is, and nothing left proves nothing (the ladder hedges)
        std::size_t kept = 0;
        for( std::size_t i = 0; i < found.size(); ++i )
        {
            if( onSide( r, found[ i ], classSide ) )
            {
                found[ kept++ ] = found[ i ];
            }
        }
        found.resize( kept );
        return !found.empty();
    }

    // (L) a language whose own receiver rules answer self / fields / paths (receiverLocalsOnly): only `local.m()` on a local
    // whose class a binding states — a call result typed by graph.h's resolve loop — reaches the class's members here
    bool narrowTypedLocal( const Reference& r ) const
    {
        if( !isMemberCallRef( r ) )
        {
            return false;
        }
        const Chain ch = chainOf( r );
        if( !ch.valid || !ch.path.empty() || !ch.ctor.empty() || ch.root.empty() || isThisRoot( ch.root ) )
        {
            return false;
        }
        const std::optional<SeenLocal> t = localSeenAt( r.fromSymbol, ch.root, r.startByte );
        return t.has_value() && !t->type.empty() && methodOf( t->type, r.calleeName, false, t->cls );
    }

    // Python `feed = parser.feed; … feed( data )`: the alias names a typed object's method
    bool narrowAliasedMethod( const Reference& r ) const
    {
        if( r.recv != RecvKind::None || r.calleeName.empty() )
        {
            return false;
        }
        NodeId scope = kNoNode;
        const std::pair<std::string, std::string>* alias = aliasedMethod( r.fromSymbol, r.calleeName, scope );
        if( alias == nullptr )
        {
            return false;
        }
        if( isThisRoot( alias->first ) || alias->first == "cls" )
        {
            return methodOf( callerClass( scope ), alias->second, false );   // `write = self.write`: the class's own member
        }
        const std::optional<std::string_view> type = localAt( scope, alias->first, r.startByte );
        return type.has_value() && !type->empty() && methodOf( unalias( r.fileId, *type ), alias->second, false );
    }

    // does module alias `root` (bound in the caller's file) name the file defining `c`
    bool moduleNames( const Reference& r, std::string_view root, NodeId c ) const
    {
        const auto it = moduleAlias.find( keyOf( r.fileId, '#', root ) );
        if( it == moduleAlias.end() || it->second.empty() )
        {
            return false;
        }
        const std::string_view source = it->second;
        const std::string_view target = rootRelPath( ing, ing.symbols[ c ].fileId );
        if( r.lang == Lang::Go )
        {
            // an import path names a package: the candidate's own package import path (its go.mod's module path and the
            // directory below it — a nested module's `quoted/lib` is `example.com/qm/lib` — or the path a local replace
            // maps its directory to) proves it EXACTLY. A candidate with a path, or an import path some in-tree file
            // carries, is proven by nothing else: a directory that merely ends the path is another package
            const std::uint32_t cf = ing.symbols[ c ].fileId;
            if( goPackages != nullptr && ( goPackages->hasPath( cf ) || goPackages->known.contains( key.assign( source ) ) ) )
            {
                return goPackages->names( cf, source );
            }
            // no module facts name this path or the candidate (a tree with no go.mod): its directory must end the path,
            // segment-aligned
            const std::string_view dir = includerDir( target );
            return !dir.empty() && source.size() >= dir.size() && source.ends_with( dir )
                && ( source.size() == dir.size() || source[ source.size() - dir.size() - 1 ] == '/' );
        }
        if( !source.starts_with( "./" ) && !source.starts_with( "../" ) )
        {
            return false;   // a package specifier: outside the tree, or resolved by a bundler this rule does not model
        }
        // join the caller's directory with the relative specifier, then compare without the extension (or /index)
        std::vector<std::string_view> parts;
        std::string_view base = includerDir( rootRelPath( ing, r.fileId ) );
        const auto push = [ & ]( std::string_view p )
        {
            while( !p.empty() )
            {
                const std::size_t slash = p.find( '/' );
                const std::string_view seg = p.substr( 0, slash );
                p = slash == std::string_view::npos ? std::string_view{} : p.substr( slash + 1 );
                if( seg.empty() || seg == "." ) { continue; }
                if( seg == ".." ) { if( !parts.empty() ) { parts.pop_back(); } continue; }
                parts.push_back( seg );
            }
        };
        push( base );
        push( source );
        std::string joined;
        for( std::string_view p : parts ) { if( !joined.empty() ) { joined.push_back( '/' ); } joined.append( p ); }
        std::string_view stem = target;
        if( const std::size_t dot = stem.rfind( '.' ); dot != std::string_view::npos && stem.find( '/', dot ) == std::string_view::npos )
        {
            stem = stem.substr( 0, dot );
        }
        return stem == joined || ( stem.ends_with( "/index" ) && stem.substr( 0, stem.size() - 6 ) == joined );
    }
};

}   // namespace rw
