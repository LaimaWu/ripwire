#pragma once
// valuerefindex.h — the resolution core of the reference-as-value rows (src/valuerefs.h says what a row means):
// ValueRefIndex, the rows every surface serves, the --uses filter and the --path count. No rendering and no
// serialize.h, so quality.h (the --quality-delta dead kind) can include it without an include cycle.

#include "graph.h"             // jsImportKey — the "fileId#name" key, reused for containers
#include "mention.h"           // pathStem — a module path's file stem
#include "resolve.h"           // includerDir, resolvePreciseInclude — a module path's file
#include "infra/Diagnostics.h"   // EXPECTS/ENSURES — the window and index invariants
#include "model.h"

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

using VrLangFamily = ValueRefFamily;   // model.h: the armed-language table the capture indexes too

// A Through key matches a Value key: "*" (a computed subscript) reaches every keyed or indexed slot, "" (a bare
// call through the variable) only the variable itself, anything else only its own key.
inline bool vrKeyMatches( std::string_view throughKey, std::string_view valueKey ) noexcept
{
    if( throughKey == "*" )
    {
        return !valueKey.empty() && ( valueKey.front() == '.' || valueKey.front() == '[' );
    }
    return throughKey == valueKey;
}

// A decorator row (`into=@NAME`) whose decorator only wraps the function and binds it back to its own name: the
// builtin descriptors, abc/typing markers, property accessors and functools.wraps. Keyed on the last dotted segment,
// so `@abc.abstractmethod`, `@functools.cached_property` and `@x.setter` all match.
inline bool vrIsWrapperDecorator( std::string_view into ) noexcept
{
    if( !into.starts_with( '@' ) )
    {
        return false;
    }
    std::string_view last = into.substr( 1 );
    if( const std::size_t dot = last.rfind( '.' ); dot != std::string_view::npos )
    {
        last = last.substr( dot + 1 );
    }
    static constexpr std::string_view kWrappers[] = {
        "classmethod", "staticmethod", "property", "cached_property", "abstractmethod", "abstractproperty",
        "abstractclassmethod", "abstractstaticmethod", "setter", "getter", "deleter", "overload", "override", "wraps",
    };
    return std::ranges::find( kWrappers, last ) != std::end( kWrappers );
}

// Two references in served order: by path, then by position in the file.
inline bool vrBindLess( const IngestResult& ing, const Reference& a, const Reference& b )
{
    if( a.fileId != b.fileId )
    {
        return ing.files[a.fileId] < ing.files[b.fileId];
    }
    return a.startByte < b.startByte;
}

// The index every verb queries. Built from one IngestResult in O(refs + symbols); no graph needed.
class ValueRefIndex
{
public:
    explicit ValueRefIndex( const IngestResult& ing ) : m_ing( ing )
    {
        for( NodeId id = 0; id < ing.symbols.size(); ++id )
        {
            const Symbol& s = ing.symbols[id];
            if( ( s.kind == SymKind::Function || s.kind == SymKind::Method ) && valueRefFamily( s.lang ) != VrLangFamily::None )
            {
                m_fnByName[ s.name ].push_back( id );
            }
        }
        markPythonMembers();
        for( std::uint32_t i = 0; i < ing.references.size(); ++i )
        {
            const Reference& r = ing.references[i];
            if( r.role == RefRole::Value && scopeOf( r ) == kValueRefDepthCutScope )
            {
                m_depthCuts.push_back( i );   // the file's depth-cut record: no row, a disclosure (depthCutsFor)
            }
            else if( r.role == RefRole::Value )
            {
                m_values.push_back( i );
                if( !r.recvVar.empty() )
                {
                    m_valueByContainer[ jsImportKey( r.fileId, r.recvVar ) ].push_back( i );
                }
            }
            else if( r.role == RefRole::Through )
            {
                if( scopeOf( r ) == 'p' )
                {
                    m_throughParamBySym[ r.fromSymbol ].push_back( i );
                }
                else
                {
                    m_throughByContainer[ jsImportKey( r.fileId, r.calleeName ) ].push_back( i );
                }
            }
        }
        for( const Binding& b : ing.bindings )
        {
            if( b.kind == LocalBindKind::JsImport || b.kind == LocalBindKind::Import )
            {
                m_importsByFile[ b.fileId ].push_back( &b );
            }
        }
        ENSURES( std::is_sorted( m_values.begin(), m_values.end() ), "m_values is built in reference order — targetsOf binary-searches it" );
        m_targets.resize( m_values.size() );
        for( std::size_t k = 0; k < m_values.size(); ++k )
        {
            m_targets[k] = resolveName( ing.references[ m_values[k] ], ing.references[ m_values[k] ].calleeName );
            for( const NodeId t : m_targets[k] )
            {
                m_valuesByTarget[ t ].push_back( static_cast<std::uint32_t>( k ) );
            }
        }
    }

    // Value references (indices into ing.references) whose resolved target is any of `defs`, in served order.
    std::vector<std::uint32_t> valueRefsTo( std::span<const NodeId> defs ) const
    {
        std::vector<std::uint32_t> out;
        for( const NodeId d : defs )
        {
            if( const auto it = m_valuesByTarget.find( d ); it != m_valuesByTarget.end() )
            {
                for( const std::uint32_t k : it->second )
                {
                    out.push_back( m_values[k] );
                }
            }
        }
        std::ranges::sort( out );
        out.erase( std::ranges::unique( out ).begin(), out.end() );
        return out;
    }

    // True when at least one value reference resolves to `def` and can make it reachable: the dead-set question
    // (--dead-code, --quality-delta). A Python descriptor/typing WRAPPER decorator row does not count. `@classmethod`,
    // `@property` or `@functools.wraps` hands the function back to the same name, so it is still reached only by a
    // call. A registering decorator (`@app.route`, `@register`) is the value use this exclusion exists for. The row
    // itself is still served (ruling 5); the wrapper FLOOR for rows is a follow-up.
    // A reference made INSIDE `def` itself (`static void tick( void ) { timer_set( tick ); }`) does not count either: it
    // can only run once something else reaches `def`, exactly as the call graph drops a recursive self-call (the CSR has
    // no self-loops). The row is still served and is still a use site (--safe-delete's uses=).
    bool isValueReferenced( NodeId def ) const
    {
        const auto it = m_valuesByTarget.find( def );
        if( it == m_valuesByTarget.end() )
        {
            return false;
        }
        return std::ranges::any_of( it->second, [ & ]( std::uint32_t k )
        {
            const Reference& r = m_ing.references[ m_values[k] ];
            return r.fromSymbol != def && !vrIsWrapperDecorator( r.fieldName );
        } );
    }

    // The value walk's depth cut (ingest_valuerefs.h kVrMaxDepth) as it bears on an answer about `defs`: how many files
    // of the defs' language families stopped their walk at the cap (a value reference below it was never captured, so a
    // value_refs= count is a FLOOR there), and the first such record by path then line (the clue, value_refs_depth_at=).
    // `everyFamily` asks about every armed family (--dead-code's whole-tree answer). Only functions and methods have
    // value references, so a def of another kind brings no family in, and no def brings none.
    struct DepthCuts
    {
        std::uint32_t files = 0;
        std::uint32_t first = UINT32_MAX;   // a reference index, UINT32_MAX when files == 0
    };
    DepthCuts depthCutsFor( std::span<const NodeId> defs, bool everyFamily = false ) const
    {
        DepthCuts out;
        for( const std::uint32_t i : m_depthCuts )
        {
            const Reference&   c   = m_ing.references[i];
            const VrLangFamily fam = valueRefFamily( c.lang );
            const bool relevant = everyFamily || std::ranges::any_of( defs, [ & ]( NodeId d )
            {
                const Symbol& s = m_ing.symbols[d];
                return ( s.kind == SymKind::Function || s.kind == SymKind::Method ) && valueRefFamily( s.lang ) == fam;
            } );
            if( !relevant )
            {
                continue;
            }
            ++out.files;
            if( out.first == UINT32_MAX || vrBindLess( m_ing, c, m_ing.references[ out.first ] ) )
            {
                out.first = i;
            }
        }
        return out;
    }

    // The resolved targets of value reference `refIdx` (an index into ing.references).
    std::vector<NodeId> targetsOf( std::uint32_t refIdx ) const
    {
        const auto it = std::lower_bound( m_values.begin(), m_values.end(), refIdx );
        if( it == m_values.end() || *it != refIdx )
        {
            return {};
        }
        return m_targets[ static_cast<std::size_t>( it - m_values.begin() ) ];
    }

    // called_by=: the functions that may call through the slot value reference `refIdx` lands in, sorted by name.
    std::vector<NodeId> calledBy( std::uint32_t refIdx ) const
    {
        std::vector<NodeId> out;
        for( const std::uint32_t t : throughsFor( refIdx ) )
        {
            const NodeId f = m_ing.references[t].fromSymbol;
            if( f != kNoNode )
            {
                out.push_back( f );
            }
        }
        std::sort( out.begin(), out.end(), [ & ]( NodeId a, NodeId b )
        {
            const Symbol& sa = m_ing.symbols[a];
            const Symbol& sb = m_ing.symbols[b];
            return sa.name != sb.name ? sa.name < sb.name : a < b;
        } );
        out.erase( std::unique( out.begin(), out.end() ), out.end() );
        return out;
    }

    // The Through references matching value reference `refIdx` (indices into ing.references).
    std::vector<std::uint32_t> throughsFor( std::uint32_t refIdx ) const
    {
        const Reference&           v = m_ing.references[refIdx];
        std::vector<std::uint32_t> out;
        const char                 sc = scopeOf( v );
        if( sc == 'f' || sc == 'l' )
        {
            if( const auto it = m_throughByContainer.find( jsImportKey( v.fileId, v.recvVar ) ); it != m_throughByContainer.end() )
            {
                for( const std::uint32_t t : it->second )
                {
                    const Reference& tr = m_ing.references[t];
                    if( scopeOf( tr ) == sc && ( sc == 'f' || tr.fromSymbol == v.fromSymbol ) && vrKeyMatches( tr.composeRel, v.composeRel ) )
                    {
                        out.push_back( t );
                    }
                }
            }
        }
        else if( sc == 'a' || sc == 'p' )
        {
            // an argument of a bare callee: the callee's definitions that CALL that parameter; a parameter default: the
            // function the default belongs to.
            std::vector<NodeId> owners = sc == 'a' ? resolveName( v, v.recvVar ) : std::vector<NodeId>{ v.fromSymbol };
            for( const NodeId o : owners )
            {
                const auto it = m_throughParamBySym.find( o );
                if( o == kNoNode || it == m_throughParamBySym.end() )
                {
                    continue;
                }
                for( const std::uint32_t t : it->second )
                {
                    const Reference& tr = m_ing.references[t];
                    if( paramMatches( v.composeRel, tr ) )
                    {
                        out.push_back( t );
                    }
                }
            }
        }
        std::ranges::sort( out );
        out.erase( std::ranges::unique( out ).begin(), out.end() );
        return out;
    }

    // The value references a Through reference reaches (the callees side of the same join).
    std::vector<std::uint32_t> valuesThrough( std::uint32_t throughIdx ) const
    {
        const Reference&           t = m_ing.references[throughIdx];
        std::vector<std::uint32_t> out;
        const char                 sc = scopeOf( t );
        if( sc == 'f' || sc == 'l' )
        {
            if( const auto it = m_valueByContainer.find( jsImportKey( t.fileId, t.calleeName ) ); it != m_valueByContainer.end() )
            {
                for( const std::uint32_t v : it->second )
                {
                    const Reference& vr = m_ing.references[v];
                    if( scopeOf( vr ) == sc && ( sc == 'f' || vr.fromSymbol == t.fromSymbol ) && vrKeyMatches( t.composeRel, vr.composeRel ) )
                    {
                        out.push_back( v );
                    }
                }
            }
        }
        else if( sc == 'p' && t.fromSymbol != kNoNode )
        {
            // every argument of a bare call that resolves to this function, at this parameter; and its own default
            const Symbol& owner = m_ing.symbols[ t.fromSymbol ];
            for( const std::uint32_t v : m_values )
            {
                const Reference& vr  = m_ing.references[v];
                const char       vsc = scopeOf( vr );
                if( vsc == 'p' && vr.fromSymbol == t.fromSymbol && paramMatches( vr.composeRel, t ) )
                {
                    out.push_back( v );
                }
                else if( vsc == 'a' && vr.recvVar == owner.name && paramMatches( vr.composeRel, t ) )
                {
                    const std::vector<NodeId> owners = resolveName( vr, vr.recvVar );
                    if( std::find( owners.begin(), owners.end(), t.fromSymbol ) != owners.end() )
                    {
                        out.push_back( v );
                    }
                }
            }
        }
        std::ranges::sort( out );
        out.erase( std::ranges::unique( out ).begin(), out.end() );
        return out;
    }

    // Through references (role Through) or Value references (role Value) made inside any of `fns`.
    std::vector<std::uint32_t> madeIn( std::span<const NodeId> fns, RefRole role ) const
    {
        std::vector<std::uint32_t> out;
        const auto inFns = [ & ]( std::uint32_t i ) { return std::ranges::find( fns, m_ing.references[i].fromSymbol ) != fns.end(); };
        if( role == RefRole::Value )
        {
            std::ranges::copy_if( m_values, std::back_inserter( out ), inFns );
            return out;
        }
        for( const auto& [ key, list ] : m_throughByContainer )
        {
            std::ranges::copy_if( list, std::back_inserter( out ), inFns );
        }
        for( const auto& [ sym, list ] : m_throughParamBySym )
        {
            if( std::ranges::find( fns, sym ) != fns.end() )
            {
                out.insert( out.end(), list.begin(), list.end() );
            }
        }
        std::ranges::sort( out );
        return out;
    }

    static char scopeOf( const Reference& r ) noexcept
    {
        return r.qualifier.empty() ? 'x' : r.qualifier[0];
    }

    // A class member, in every armed language: a Method, or a Python def directly in its class (public: PatternIndex reads it).
    bool isClassMember( const Symbol& s ) const noexcept
    {
        if( s.kind == SymKind::Method )
        {
            return true;
        }
        return s.lang == Lang::Python && s.id < m_pyMember.size() && m_pyMember[ s.id ] != 0;
    }

private:

    // A value key "#N" (an argument position) or "#name" (a keyword / a parameter default) against a parameter Through.
    static bool paramMatches( std::string_view valueKey, const Reference& through ) noexcept
    {
        if( valueKey.size() < 2 || valueKey.front() != '#' )
        {
            return false;
        }
        const std::string_view tail = valueKey.substr( 1 );
        if( tail.front() >= '0' && tail.front() <= '9' )
        {
            return through.argCountKnown && std::to_string( through.argCount ) == tail;
        }
        return through.calleeName == tail;
    }

    std::uint32_t importedModuleFile( const Binding& b ) const
    {
        if( b.typeName.empty() || b.fileId >= m_ing.files.size() )
        {
            return kNoFile;
        }
        if( m_fileIndex.empty() )
        {
            m_fileIndex.reserve( m_ing.files.size() );
            for( std::uint32_t f = 0; f < m_ing.files.size(); ++f )
            {
                m_fileIndex.emplace( lexicalNormalize( rootRelPath( m_ing, f ) ), f );
            }
        }
        const std::uint32_t precise = resolvePreciseInclude( rootRelPath( m_ing, b.fileId ), b.typeName, /*isAngle=*/false, m_fileIndex );
        if( precise != kNoFile || b.kind != LocalBindKind::Import )
        {
            return precise;
        }
        return resolvePythonModuleSuffix( b.typeName, m_fileIndex, m_ing.fileRoot.empty() ? nullptr : &m_ing.fileRoot, b.fileId );
    }

    // Python's tags.scm has no @definition.method: every `def` is a Function, and its `scope` is the nearest
    // enclosing class at ANY depth (a helper nested inside a method carries the class too). A Function is a class
    // MEMBER only when its innermost enclosing definition is that class itself, which this sweep decides from the
    // def spans: per file, defs sorted by start (widest first), a stack of the open ones.
    void markPythonMembers()
    {
        std::vector<NodeId> py;
        for( NodeId id = 0; id < m_ing.symbols.size(); ++id )
        {
            const Symbol& s = m_ing.symbols[id];
            if( s.lang == Lang::Python
                && ( s.kind == SymKind::Function || s.kind == SymKind::Method || s.kind == SymKind::Class ) )
            {
                py.push_back( id );
            }
        }
        if( py.empty() )
        {
            return;
        }
        m_pyMember.assign( m_ing.symbols.size(), 0 );
        std::ranges::sort( py, [ & ]( NodeId a, NodeId b )
        {
            const Symbol& x = m_ing.symbols[a];
            const Symbol& y = m_ing.symbols[b];
            if( x.fileId != y.fileId )
            {
                return x.fileId < y.fileId;
            }
            if( x.sigStartByte != y.sigStartByte )
            {
                return x.sigStartByte < y.sigStartByte;
            }
            return x.endByte != y.endByte ? x.endByte > y.endByte : a < b;
        } );
        std::vector<NodeId> open;
        for( const NodeId id : py )
        {
            const Symbol& s = m_ing.symbols[id];
            while( !open.empty() )
            {
                const Symbol& o = m_ing.symbols[ open.back() ];
                if( o.fileId == s.fileId && o.sigStartByte <= s.sigStartByte && s.endByte <= o.endByte )
                {
                    break;
                }
                open.pop_back();
            }
            if( s.kind != SymKind::Class && !s.scope.empty() && !open.empty() )
            {
                const Symbol& owner = m_ing.symbols[ open.back() ];
                m_pyMember[id] = owner.kind == SymKind::Class && owner.name == s.scope ? 1 : 0;
            }
            open.push_back( id );
        }
    }

    // Is the class member `m` in bare-name scope at reference `r`? The call graph's own visibility, per language:
    //   * a decorator row names the definition it decorates — the decorated def itself, in this file;
    //   * Python: only the class BODY sees its members bare (`__str__ = render`, `property( _get )`); a method body
    //     does not (`sorted( xs, key=sibling )` is a NameError), nor does any other code (`isinstance( x, (list,
    //     tuple) )` never means a method named `list`);
    //   * C++: the class body, and any member function of the same class (in-class or `T::f` out of class);
    //   * JS/TS: never — a method is reached through `this`/an instance, a bare name is a binding or a global
    //     (the JS call graph binds a bare `render()` to a method by name; this resolver deliberately does not);
    //   * Go: never — a method value is always `recv.M`.
    bool memberVisibleFrom( const Reference& r, const Symbol& m, VrLangFamily fam ) const
    {
        if( r.role == RefRole::Value && r.fieldName.starts_with( '@' ) )
        {
            return m.fileId == r.fileId;
        }
        if( ( fam != VrLangFamily::Py && fam != VrLangFamily::C ) || r.fromSymbol == kNoNode || r.fromSymbol >= m_ing.symbols.size()
            || m.scope.empty() )
        {
            return false;
        }
        const Symbol& f      = m_ing.symbols[ r.fromSymbol ];
        const bool    fIsFn  = f.kind == SymKind::Function || f.kind == SymKind::Method;
        const bool    fClass = ( f.kind == SymKind::Class || isStructOrNamedType( f.kind ) ) && f.name == m.scope;
        // a statement of the class body (owned by the class, or by an annotated attribute of it); a Python NESTED class's
        // body does not see the outer class's names, a C++ one does
        const bool    inBody = !fIsFn && f.fileId == m.fileId
                            && ( fClass || ( f.scope == m.scope && ( fam == VrLangFamily::C || f.kind != SymKind::Class ) ) );
        if( inBody )
        {
            return true;   // the class body itself
        }
        return fam == VrLangFamily::C && fIsFn && f.scope == m.scope;   // a C++ member function of the same class
    }

    // `name` as seen from reference `r`'s file, under the visibility rules in the header comment.
    std::vector<NodeId> resolveName( const Reference& r, std::string_view name ) const
    {
        const auto it = m_fnByName.find( std::string( name ) );
        if( it == m_fnByName.end() )
        {
            return {};
        }
        const VrLangFamily  fam = valueRefFamily( r.lang );
        // A C/C++ PROTOTYPE (a bodyless declaration) names the function its definition lives in: it is the target only
        // when no definition with a body is visible — a prototype in this file never hides the extern definition.
        std::vector<NodeId> sameFile, other, declsHere;
        for( const NodeId id : it->second )
        {
            const Symbol& s = m_ing.symbols[id];
            if( valueRefFamily( s.lang ) != fam )
            {
                continue;
            }
            if( isClassMember( s ) && !memberVisibleFrom( r, s, fam ) )
            {
                continue;   // a class member is never in BARE-name scope outside its class (R1 of the final review)
            }
            const bool prototype = fam == VrLangFamily::C && s.sigEndByte >= s.endByte;
            if( prototype )
            {
                if( s.fileId == r.fileId )
                {
                    declsHere.push_back( id );
                }
                continue;
            }
            ( s.fileId == r.fileId ? sameFile : other ).push_back( id );
        }
        if( !sameFile.empty() )
        {
            return sameFile;
        }
        const bool fileShadow = r.qualifier.size() >= 2 && r.qualifier[1] == '1';
        if( fileShadow || other.empty() )
        {
            return fileShadow ? std::vector<NodeId>{} : declsHere;
        }
        const std::string_view refPath = m_ing.files[ r.fileId ];
        std::vector<NodeId>    out;
        switch( fam )
        {
            case VrLangFamily::C:
            {
                for( const NodeId id : other )
                {
                    if( m_ing.symbols[id].internalLinkage == 0 )
                    {
                        out.push_back( id );
                    }
                }
                break;
            }
            case VrLangFamily::Go:
            {
                for( const NodeId id : other )
                {
                    if( includerDir( m_ing.files[ m_ing.symbols[id].fileId ] ) == includerDir( refPath ) )
                    {
                        out.push_back( id );
                    }
                }
                break;
            }
            case VrLangFamily::Js:
            case VrLangFamily::Py:
            {
                const auto imp = m_importsByFile.find( r.fileId );
                if( imp == m_importsByFile.end() )
                {
                    break;
                }
                for( const Binding* b : imp->second )
                {
                    const bool named = b->kind == LocalBindKind::JsImport ? ( b->var == name && b->importedName == name )
                                                                         : ( b->var == name && b->importedName.empty() );
                    if( !named )
                    {
                        continue;
                    }
                    // The import's module, resolved by the call graph's own Step-A (resolve.h resolvePreciseInclude: a
                    // relative JS/TS specifier against the importer, a Python module relative-to-file then root, then the
                    // whole-component suffix for an absolute Python spec). A bare package specifier resolves to no file:
                    // no row — a stem match across directories bound `import { test } from "./a"` to every a.js.
                    const std::uint32_t moduleFile = importedModuleFile( *b );
                    for( const NodeId id : other )
                    {
                        if( m_ing.symbols[id].fileId == moduleFile )
                        {
                            out.push_back( id );
                        }
                    }
                }
                break;
            }
            case VrLangFamily::None: break;
        }
        std::sort( out.begin(), out.end() );
        out.erase( std::unique( out.begin(), out.end() ), out.end() );
        return out.empty() ? declsHere : out;
    }

    const IngestResult&                                  m_ing;
    HashMap<std::string, std::vector<NodeId>>            m_fnByName;
    std::vector<std::uint8_t>                            m_pyMember;   // per symbol: 1 = a Python def directly in its class (empty: no Python)
    std::vector<std::uint32_t>                           m_values;            // Value reference indices, ascending
    std::vector<std::uint32_t>                           m_depthCuts;         // the files' depth-cut records (no row), ascending
    std::vector<std::vector<NodeId>>                     m_targets;           // parallel to m_values
    HashMap<NodeId, std::vector<std::uint32_t>>          m_valuesByTarget;    // target def → positions in m_values
    HashMap<std::string, std::vector<std::uint32_t>>     m_valueByContainer;  // "file#container" → Value refs
    HashMap<std::string, std::vector<std::uint32_t>>     m_throughByContainer;
    HashMap<NodeId, std::vector<std::uint32_t>>          m_throughParamBySym;
    HashMap<std::uint32_t, std::vector<const Binding*>>  m_importsByFile;
    mutable HashMap<std::string, std::uint32_t>          m_fileIndex;   // root-relative normalised path → fileId, built on first import
};

// ── IX-3 name-template dispatch ───────────────────────────────────────────────────────────────────────────────
// The Pattern references (model.h RefRole::Pattern) and what they may name. Its own index beside ValueRefIndex, built
// from the same IngestResult: a template site (indices into ing.references, in reference order) can name a class member
// of the site's value-ref family whose name holds the literal prefix and suffix around a non-empty hole. A `self` /
// `this` / `cls` receiver narrows the members to the site's class family (its ancestors and its subclasses, by base
// name) when the site's method has a class scope (Python today; a JS/TS method's scope is empty in this index, so a
// `this` site there is unnarrowed — the nunjucks census family equals the whole tree's 50); any other receiver is
// unnarrowed. A MAY-dispatch clue matched by name: never a call. Class membership is ValueRefIndex::isClassMember,
// read once per symbol at construction (only when a Pattern reference exists), so no reference to `vri` is kept.
class PatternIndex
{
public:
    PatternIndex( const IngestResult& ing, const ValueRefIndex& vri ) : m_ing( ing )
    {
        for( std::uint32_t i = 0; i < ing.references.size(); ++i )
        {
            if( ing.references[i].role == RefRole::Pattern )
            {
                ( ValueRefIndex::scopeOf( ing.references[i] ) == 'w' ? m_patternsWide : m_patterns ).push_back( i );   // 'w' = too wide, a count, never a row
            }
        }
        if( !m_patterns.empty() || !m_patternsWide.empty() )
        {
            m_member.resize( ing.symbols.size() );
            for( NodeId id = 0; id < ing.symbols.size(); ++id )
            {
                m_member[id] = vri.isClassMember( ing.symbols[id] ) ? 1 : 0;
            }
        }
    }

    // The usable Pattern sites whose template can name one of `defs`, in reference order, once each.
    std::vector<std::uint32_t> sitesTo( std::span<const NodeId> defs ) const
    {
        return sitesNaming( m_patterns, defs );
    }
    // The class members Pattern site `refIdx` can name, in symbol order (the dispatcher's --callees side).
    std::vector<NodeId> targets( std::uint32_t refIdx ) const
    {
        std::vector<NodeId> out;
        const Reference& r = m_ing.references[ refIdx ];
        for( NodeId id = 0; id < m_ing.symbols.size(); ++id )
        {
            if( isMember( id ) && reaches( r, m_ing.symbols[id] ) )
            {
                out.push_back( id );
            }
        }
        return out;
    }
    // The usable Pattern sites made inside any of `fns` (the dispatcher's own sites), in reference order.
    std::vector<std::uint32_t> sitesIn( std::span<const NodeId> fns ) const
    {
        std::vector<std::uint32_t> out;
        std::ranges::copy_if( m_patterns, std::back_inserter( out ), [ & ]( std::uint32_t i ) { return std::ranges::find( fns, m_ing.references[i].fromSymbol ) != fns.end(); } );
        return out;
    }
    // Too-wide templates (`f"_{m}"`: no literal side of kVrPatternMinLiteral identifier characters) whose written sides
    // could name one of `defs`: pattern_too_wide=, a count beside the rows, never a row of its own.
    std::size_t tooWideTo( std::span<const NodeId> defs ) const
    {
        return sitesNaming( m_patternsWide, defs ).size();
    }

private:
    bool isMember( NodeId id ) const noexcept
    {
        return id < m_member.size() && m_member[id] != 0;
    }

    // The Pattern sites of `sites` (reference order) that can name at least one class member of `defs`, in that order.
    std::vector<std::uint32_t> sitesNaming( const std::vector<std::uint32_t>& sites, std::span<const NodeId> defs ) const
    {
        std::vector<std::uint32_t> out;
        std::ranges::copy_if( sites, std::back_inserter( out ), [ & ]( std::uint32_t i )
        {
            return std::ranges::any_of( defs, [ & ]( NodeId d ) { return isMember( d ) && reaches( m_ing.references[i], m_ing.symbols[d] ); } );
        } );
        return out;
    }
    // Can Pattern reference `r` name class member `s`? Same value-ref family; the literal sides around a non-empty hole;
    // a self/this/cls receiver inside a class keeps only that class's family.
    bool reaches( const Reference& r, const Symbol& s ) const
    {
        if( valueRefFamily( s.lang ) != valueRefFamily( r.lang ) )
        {
            return false;
        }
        const std::string_view pre = r.fieldName;
        const std::string_view suf = r.composeRel;
        const std::string_view n   = s.name;
        if( n.size() <= pre.size() + suf.size() || !n.starts_with( pre ) || !n.ends_with( suf ) )
        {
            return false;
        }
        const std::string_view recv = r.recvVar;
        if( ( recv == "self" || recv == "this" || recv == "cls" ) && r.fromSymbol < m_ing.symbols.size() )
        {
            const std::string& site = m_ing.symbols[ r.fromSymbol ].scope;
            if( !site.empty() )
            {
                return inClassFamily( site, s.scope );
            }
        }
        return true;
    }
    // Is class `member` in the family of class `site`: `site` itself, an ancestor of it, or a subclass of it (by name)?
    bool inClassFamily( const std::string& site, const std::string& member ) const
    {
        if( member.empty() )
        {
            return false;
        }
        if( site == member )
        {
            return true;
        }
        buildBases();
        const auto ancestor = [ & ]( const std::string& from, const std::string& to )   // `to` is an ancestor of `from`
        {
            std::vector<std::string_view> todo { from };
            std::vector<std::string_view> seen;
            while( !todo.empty() && seen.size() < 4096 )
            {
                const std::string_view c = todo.back();
                todo.pop_back();
                if( std::ranges::find( seen, c ) != seen.end() )
                {
                    continue;
                }
                seen.push_back( c );
                if( const auto it = m_basesOf.find( std::string( c ) ); it != m_basesOf.end() )
                {
                    for( const std::string& b : it->second )
                    {
                        if( b == to )
                        {
                            return true;
                        }
                        todo.push_back( b );
                    }
                }
            }
            return false;
        };
        return ancestor( site, member ) || ancestor( member, site );
    }
    // class name → its written base names, from the inherit references (built on first use; name-keyed, as the census)
    void buildBases() const
    {
        if( m_basesBuilt )
        {
            return;
        }
        m_basesBuilt = true;
        for( const Reference& r : m_ing.references )
        {
            if( ( r.isInherit || r.role == RefRole::Extends ) && r.fromSymbol < m_ing.symbols.size() && m_ing.symbols[ r.fromSymbol ].kind == SymKind::Class )
            {
                std::vector<std::string>& b = m_basesOf[ m_ing.symbols[ r.fromSymbol ].name ];
                if( std::ranges::find( b, r.calleeName ) == b.end() )
                {
                    b.push_back( r.calleeName );
                }
            }
        }
    }

    const IngestResult&                                    m_ing;
    std::vector<std::uint32_t>                             m_patterns;       // usable Pattern references, in reference order
    std::vector<std::uint32_t>                             m_patternsWide;   // too-wide Pattern references
    std::vector<std::uint8_t>                              m_member;         // per symbol: 1 = a class member (empty: no Pattern reference)
    mutable HashMap<std::string, std::vector<std::string>> m_basesOf;        // class name → base names (buildBases)
    mutable bool                                           m_basesBuilt = false;
};

// ── the rows every surface serves ─────────────────────────────────────────────────────────────────────────────
// One <vr> row. Callers side: in = the enclosing symbol of the binding site, calledBy = may-call functions.
// Callees side: to = the referenced function, through = the written callee of the call through the slot, sites =
// how many binding sites one (to, through) pair joins.
struct ValueRefRow
{
    std::uint32_t ref     = 0;          // the Value reference (bind=, into=)
    NodeId        in      = kNoNode;    // callers side: the enclosing symbol
    NodeId        to      = kNoNode;    // callees side: the referenced function
    std::string   calledBy;             // callers side: comma-joined names
    std::string   through;              // callees side
    std::uint32_t sites   = 1;
};

struct ValueRefRows
{
    std::vector<ValueRefRow> rows;      // every row, served order; the window is the caller's
    ValueRefIndex::DepthCuts depthCut;  // the walk's depth cut over the answered defs' families (value_refs_depth_capped=)
    std::vector<std::uint32_t> patternSites;   // IX-3 (the PatternIndex overloads): callers — template lookups that may name the defs; callees — the templates in them
    std::size_t              patternTooWide = 0;   // IX-3 (callers): too-wide templates whose sides could name the defs
};

// --callers side: every value reference resolving to one of `defs`.
inline ValueRefRows valueRefCallerRows( const IngestResult& ing, const ValueRefIndex& idx, std::span<const NodeId> defs )
{
    ValueRefRows out;
    out.depthCut = idx.depthCutsFor( defs );
    for( const std::uint32_t r : idx.valueRefsTo( defs ) )
    {
        ValueRefRow row;
        row.ref = r;
        row.in  = ing.references[r].fromSymbol;
        for( const NodeId f : idx.calledBy( r ) )
        {
            if( !row.calledBy.empty() )
            {
                row.calledBy.push_back( ',' );
            }
            row.calledBy += ing.symbols[f].name;
        }
        out.rows.push_back( std::move( row ) );
    }
    std::sort( out.rows.begin(), out.rows.end(), [ & ]( const ValueRefRow& a, const ValueRefRow& b )
    {
        const Reference& ra = ing.references[a.ref];
        const Reference& rb = ing.references[b.ref];
        if( ra.fileId != rb.fileId || ra.startByte != rb.startByte )
        {
            return vrBindLess( ing, ra, rb );
        }
        return a.ref < b.ref;   // two value references at one site: parse order within the file (a total order)
    } );
    return out;
}

// The --callees row order, a total order on content: the binding site, then the target's name, then through=. Rows tied
// on all three are two definitions of one name (`fp = helper;` with a `helper` in each of two files): std::sort leaves
// equal rows in an order each standard library picks differently, and the 64-row window would then show a
// toolchain-dependent subset — so the definition's path, then its line, decide.
inline bool vrCalleeRowLess( const IngestResult& ing, const ValueRefRow& a, const ValueRefRow& b )
{
    const Reference& ra = ing.references[a.ref];
    const Reference& rb = ing.references[b.ref];
    if( ra.fileId != rb.fileId || ra.startByte != rb.startByte )
    {
        return vrBindLess( ing, ra, rb );
    }
    const Symbol& ta = ing.symbols[a.to];
    const Symbol& tb = ing.symbols[b.to];
    if( ta.name != tb.name )
    {
        return ta.name < tb.name;
    }
    if( a.through != b.through )
    {
        return a.through < b.through;
    }
    if( ta.fileId != tb.fileId )
    {
        return ing.files[ta.fileId] < ing.files[tb.fileId];
    }
    if( ta.line != tb.line )
    {
        return ta.line < tb.line;
    }
    return a.to < b.to;   // two same-named definitions on one line (C++ overloads): their rows render alike, the order is still total
}

// The function stores AND calls through the slot: the stored row (binding site v, function target) carries through=, the
// first written callee. A second call through the same slot and callee is that row again, never a new one. False when
// no stored row joins (the call goes through a slot another function or the file filled).
inline bool vrJoinStoredRow( std::vector<ValueRefRow>& rows, std::uint32_t v, NodeId target, const std::string& through )
{
    const auto same = std::ranges::find_if( rows, [ & ]( const ValueRefRow& r )
    {
        return r.ref == v && r.to == target && ( r.through.empty() || r.through == through );
    } );
    if( same == rows.end() )
    {
        return false;
    }
    if( same->through.empty() )
    {
        same->through = through;
    }
    return true;
}

// The rows a call THROUGH a slot reaches: one per (to, through), bind= its first binding site, sites= the number of
// DISTINCT binding sites — two calls through one slot never count one site twice.
struct VrViaRows
{
    std::vector<ValueRefRow>                rows;
    std::vector<std::vector<std::uint32_t>> sites;   // parallel to rows, ascending Value refs

    void add( const IngestResult& ing, std::uint32_t v, NodeId target, const std::string& through )
    {
        const auto grouped = std::ranges::find_if( rows, [ & ]( const ValueRefRow& r ) { return r.to == target && r.through == through; } );
        if( grouped == rows.end() )
        {
            ValueRefRow row;
            row.ref     = v;
            row.to      = target;
            row.through = through;
            rows.push_back( std::move( row ) );
            sites.push_back( { v } );
            return;
        }
        std::vector<std::uint32_t>& s  = sites[ static_cast<std::size_t>( grouped - rows.begin() ) ];
        const auto                  at = std::ranges::lower_bound( s, v );
        if( at == s.end() || *at != v )
        {
            s.insert( at, v );
        }
        if( vrBindLess( ing, ing.references[v], ing.references[grouped->ref] ) )
        {
            grouped->ref = v;
        }
    }
    void appendTo( std::vector<ValueRefRow>& out )
    {
        for( std::size_t i = 0; i < rows.size(); ++i )
        {
            rows[i].sites = static_cast<std::uint32_t>( sites[i].size() );
            out.push_back( std::move( rows[i] ) );
        }
    }
};

// --callees side: the functions `fns` store/pass as values (through= absent unless the same function also calls
// through that very slot), and the functions they may call through a parameter or a container (through= the written
// callee; one row per (to, through), bind= its first site, sites= the number of distinct binding sites).
// Rows are per BINDING SITE and WRITTEN callee, never per call through it: two calls `tbl.k(); tbl.k();` through one slot
// are one row, and `TABLE.k()` called twice through a slot one site fills is sites=1, not 2. FLOOR: one slot called under two
// spellings (`tbl.k()` and `tbl["k"]()`) is two written callees, so two rows.
inline ValueRefRows valueRefCalleeRows( const IngestResult& ing, const ValueRefIndex& idx, std::span<const NodeId> fns )
{
    ValueRefRows out;
    out.depthCut = idx.depthCutsFor( fns );
    for( const std::uint32_t v : idx.madeIn( fns, RefRole::Value ) )
    {
        for( const NodeId t : idx.targetsOf( v ) )
        {
            ValueRefRow row;
            row.ref = v;
            row.to  = t;
            out.rows.push_back( std::move( row ) );
        }
    }
    VrViaRows via;
    for( const std::uint32_t t : idx.madeIn( fns, RefRole::Through ) )
    {
        const Reference& tr = ing.references[t];
        for( const std::uint32_t v : idx.valuesThrough( t ) )
        {
            for( const NodeId target : idx.targetsOf( v ) )
            {
                if( !vrJoinStoredRow( out.rows, v, target, tr.fieldName ) )
                {
                    via.add( ing, v, target, tr.fieldName );
                }
            }
        }
    }
    via.appendTo( out.rows );
    std::sort( out.rows.begin(), out.rows.end(), [ & ]( const ValueRefRow& a, const ValueRefRow& b ) { return vrCalleeRowLess( ing, a, b ); } );
    return out;
}

// The same rows with the IX-3 Pattern rows riding inside them (patternSites / patternTooWide): the callers side's
// template lookups that may name `defs`, and the callees side's templates made in `fns`.
inline ValueRefRows valueRefCallerRows( const IngestResult& ing, const ValueRefIndex& idx, std::span<const NodeId> defs, const PatternIndex& pix )
{
    ValueRefRows out   = valueRefCallerRows( ing, idx, defs );
    out.patternSites   = pix.sitesTo( defs );
    out.patternTooWide = pix.tooWideTo( defs );
    return out;
}
inline ValueRefRows valueRefCalleeRows( const IngestResult& ing, const ValueRefIndex& idx, std::span<const NodeId> fns, const PatternIndex& pix )
{
    ValueRefRows out = valueRefCalleeRows( ing, idx, fns );
    out.patternSites = pix.sitesIn( fns );
    return out;
}

// The --uses / MCP uses site filter, ONE copy for both surfaces. A role="value" site is served only when the resolver
// binds it to `valueDefs` (empty = any function of that name); a call THROUGH a value is never a use site; and a value
// site replaces the value scanner's role="read" row at the same identifier (one site, one row).
class UsesValueFilter
{
public:
    UsesValueFilter( const IngestResult& ing, std::string_view name, std::span<const NodeId> valueDefs, const ValueRefIndex* cached = nullptr )
    {
        const std::optional<ValueRefIndex> local = cached == nullptr ? std::optional<ValueRefIndex>( std::in_place, ing ) : std::nullopt;
        const ValueRefIndex&               vri   = cached != nullptr ? *cached : *local;
        for( std::uint32_t i = 0; i < ing.references.size(); ++i )
        {
            const Reference& r = ing.references[i];
            if( r.role != RefRole::Value || r.calleeName != name || r.fieldName.starts_with( '@' ) )
            {
                continue;   // a decorator row is a fact about the DEFINITION, not a use site elsewhere: the callers verb lists it
            }
            const std::vector<NodeId> targets = vri.targetsOf( i );
            const bool chosen = !targets.empty()
                             && ( valueDefs.empty() || std::any_of( targets.begin(), targets.end(), [ & ]( NodeId t )
                                                                    { return std::find( valueDefs.begin(), valueDefs.end(), t ) != valueDefs.end(); } ) );
            if( chosen )
            {
                m_accepted.push_back( i );
                m_sites.push_back( ( std::uint64_t( r.fileId ) << 32 ) | r.startByte );
            }
        }
        std::sort( m_sites.begin(), m_sites.end() );
    }
    // True when reference `refIndex` is NOT a use-site row of this answer.
    bool skip( std::uint32_t refIndex, const Reference& r ) const
    {
        if( r.role == RefRole::Through || r.role == RefRole::Subscript || r.role == RefRole::Pattern )   // a subscript is a use only as the call it may resolve to; a name template never is one
        {
            return true;
        }
        if( r.role == RefRole::Value )
        {
            return !std::binary_search( m_accepted.begin(), m_accepted.end(), refIndex );   // decorator rows included: never accepted
        }
        if( ( r.role == RefRole::Read || r.role == RefRole::Write ) && !m_sites.empty() )
        {
            return std::binary_search( m_sites.begin(), m_sites.end(), ( std::uint64_t( r.fileId ) << 32 ) | r.startByte );
        }
        return false;
    }
    bool any() const noexcept { return !m_accepted.empty(); }

private:
    std::vector<std::uint32_t> m_accepted;   // ascending (built in index order)
    std::vector<std::uint64_t> m_sites;
};

// --path / path_between: with NO directed call path (`unreachable`), how often `dstDefs` are used as values (0 when a
// path exists, so the attribute is absent and the answer byte-identical) and the depth cut over their language families
// (ValueRefRows::depthCut: a count read off a cut file is a floor, and the answer says so, as --callers' does).
struct ToValueRefs
{
    std::size_t              count = 0;
    ValueRefIndex::DepthCuts depthCut;
};
inline ToValueRefs toValueRefs( const IngestResult& ing, bool unreachable, std::span<const NodeId> dstDefs, const ValueRefIndex* cached = nullptr )
{
    if( !unreachable )
    {
        return {};
    }
    const ValueRefRows rows = cached != nullptr ? valueRefCallerRows( ing, *cached, dstDefs ) : valueRefCallerRows( ing, ValueRefIndex( ing ), dstDefs );
    return { rows.rows.size(), rows.depthCut };
}

}   // namespace rw
