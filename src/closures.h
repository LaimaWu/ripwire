#pragma once
// closures.h — CLOSURE CONTAINMENT (IX-2 P2, lane ix2-slice): a named function written INSIDE another function's body.
//
// THE GAP. `const createTemplate = ( err, info ) => { … new Template( … ) … }` inside nunjucks' Environment.getTemplate is
// its own definition, so the calls in its body are ITS calls, and `--callees=getTemplate` never listed Template, although
// the reader asking "what does getTemplate call" counts every call written in its body, closures included. Nothing calls
// createTemplate by name either: getTemplate hands it to lib.asyncIter as a value. The same shape hides koa's
// `const handleResponse = () => respond( ctx )` inside Application.handleRequest, and every Python helper `def` written
// inside another `def`.
//
// THE FACT. A definition's innermost enclosing definition is read off the parser's own spans: per file, the definitions
// that open a scope (functions, methods, classes, structs, interfaces) sorted by start (widest first), and a stack of the
// open ones. A FUNCTION-LIKE definition (a function or method with a body) whose innermost enclosing definition is itself
// function-like is that one's CLOSURE: written in its body, so it can only run once the outer one has run (and called it
// or handed it on). A function inside a class inside a function is that class's member, not a closure (the stack stops at
// the class). An anonymous closure (a lambda, a JS arrow passed inline, a Go func literal, a Rust closure, a Ruby block) is
// no definition, and its calls already belong to the enclosing one; so only NAMED inner definitions are this relation.
// Language-neutral by construction: it reads spans and kinds, which every grammar's definitions carry; no Lang:: compare.
//
// WHAT IT IS NOT. The link is NOT a call edge: it never enters the graph, count=, reaches=, the rank or any path hop.
// Every surface that prints it says via="closure" (containment: defined there, called from there or wherever it is
// handed). Computed at query time from the ingest's spans, so no cached content changes.

#include "model.h"
#include "infra/Diagnostics.h"   // EXPECTS — parentOf's id contract

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace rw
{

// A definition's part in closure containment: a FUNCTION-like one (a function or method with a body) can be a closure or have
// one; a CLASS-like one (class, struct, interface with a body) only opens a scope the stack stops at; a declaration opens none.
enum class ClosureRole : std::uint8_t { None, Scope, Function };
inline ClosureRole closureRoleOf( const Symbol& s ) noexcept
{
    if( s.endByte <= s.sigEndByte )
    {
        return ClosureRole::None;
    }
    switch( s.kind )
    {
        case SymKind::Function: case SymKind::Method:
            return ClosureRole::Function;
        case SymKind::Class: case SymKind::Struct: case SymKind::Interface:
            return ClosureRole::Scope;
        default:
            return ClosureRole::None;
    }
}

class ClosureIndex
{
public:
    explicit ClosureIndex( const IngestResult& ing ) : m_ing( ing ), m_parent( ing.symbols.size(), kNoNode )
    {
        // per file: the scope-opening definitions by start, the WIDER first on a tie (the outer one opens first), then id
        const SymbolsByFile byFile = symbolsByFile( ing, []( const Symbol& s ) { return closureRoleOf( s ) != ClosureRole::None; },
                                                    [ & ]( NodeId a, NodeId b )
                                                    {
                                                        const Symbol& x = ing.symbols[a];
                                                        const Symbol& y = ing.symbols[b];
                                                        if( x.sigStartByte != y.sigStartByte )
                                                        {
                                                            return x.sigStartByte < y.sigStartByte;
                                                        }
                                                        return x.endByte != y.endByte ? x.endByte > y.endByte : a < b;
                                                    } );
        std::vector<NodeId> open;
        for( const FileSymbols& bucket : byFile )
        {
            open.clear();
            for( const NodeId id : bucket )
            {
                const Symbol& s = ing.symbols[id];
                while( !open.empty() )
                {
                    const Symbol& o = ing.symbols[ open.back() ];
                    if( o.sigStartByte <= s.sigStartByte && s.endByte <= o.endByte && !( o.sigStartByte == s.sigStartByte && o.endByte == s.endByte ) )
                    {
                        break;   // `o` strictly holds `s`: the innermost open scope
                    }
                    open.pop_back();
                }
                if( !open.empty() && closureRoleOf( s ) == ClosureRole::Function && closureRoleOf( ing.symbols[ open.back() ] ) == ClosureRole::Function )
                {
                    m_parent[id] = open.back();
                    m_any        = true;
                }
                open.push_back( id );
            }
        }
    }

    bool any() const noexcept { return m_any; }

    // The function whose body `id` (a symbol id) is written in (kNoNode: not a closure).
    NodeId parentOf( NodeId id ) const
    {
        EXPECTS( id < m_parent.size(), "a symbol id of the ingest this index was built from" );
        return m_parent[id];
    }

    // Every closure written inside any of `fns`, at any depth (a closure's own closures too), in file then source order,
    // each once. `fns` themselves are never listed.
    std::vector<NodeId> closuresOf( std::span<const NodeId> fns ) const
    {
        std::vector<NodeId> out;
        if( !m_any || fns.empty() )
        {
            return out;
        }
        std::vector<char> isRoot( m_parent.size(), 0 );
        for( const NodeId f : fns )
        {
            if( f < isRoot.size() )
            {
                isRoot[f] = 1;
            }
        }
        for( NodeId id = 0; id < m_parent.size(); ++id )
        {
            // walk up the closure chain; it ends at a non-closure, so it is finite (a parent strictly holds its child)
            for( NodeId p = m_parent[id]; p != kNoNode; p = m_parent[p] )
            {
                if( isRoot[p] != 0 )
                {
                    if( isRoot[id] == 0 )
                    {
                        out.push_back( id );
                    }
                    break;
                }
            }
        }
        std::ranges::sort( out, [ & ]( NodeId a, NodeId b )
        {
            const Symbol& x = m_ing.symbols[a];
            const Symbol& y = m_ing.symbols[b];
            if( x.fileId != y.fileId )
            {
                return m_ing.files[ x.fileId ] < m_ing.files[ y.fileId ];
            }
            return x.sigStartByte != y.sigStartByte ? x.sigStartByte < y.sigStartByte : a < b;
        } );
        return out;
    }

private:
    const IngestResult& m_ing;
    std::vector<NodeId> m_parent;
    bool                m_any = false;
};

}   // namespace rw
