#pragma once
// rustusebounds.h — THE RUST PRECISE-CAPTURE RE-PARSE: the Rust layer of the blank-and-re-parse family whose shared
// half (BlankSpan, blankInvocations, adoptsReparse) lives in src/macroreparse.h. Wired into the ingest by
// ingest_sidecap.h (measureHealthAdoptingMemberMacroReparse). Gates: test/macroreparsecheck.sh section R (real parses)
// and test/macroreparse_unit.cpp (the scanner shapes below, one case per boundary).
//
// WHAT IT REPAIRS. Rust 1.82 (stable 2024-10) added precise capturing, a `use<..>` bound on a return-position
// `impl Trait`: `fn f() -> impl Future<Output = T> + use<> { … }`. The vendored tree-sitter-rust predates it. The
// bound is an ERROR, the function body that follows no longer belongs to the function, and its stray braces leave
// the parser unable to reduce `source_file`: the ROOT becomes one ERROR node over the whole file (err_ratio=1.000).
// Every associated fn in that file is then indexed with no impl owner, so `Type::method` selectors cannot bind.
// Measured on the zed corpus: 79 files carry the bound, 25 of them parse as one whole-file ERROR, and with only
// these bounds blanked all 25 parse with err_ratio 0.000-0.002 (reports/rust-degraded.md, Stage 0).
//
// WHY NOT THE GRAMMAR. A grammar bump changes every Rust parse in every corpus. This touches only a Rust file whose
// first parse already holds error bytes, only the bounds the scanner below names, and keeps the result only when the
// parser measurably agrees it is better (macroreparse::adoptsReparse: strictly fewer error bytes).
//
// WHAT BLANKING LOSES: nothing the index records. A `use<..>` list may name only the generic parameters and lifetimes
// already in scope (`use<'a, T, Self>`), never a path, a call or a new name, so no definition, call or type reference
// comes from inside it. The rest of the bound (`impl Send + Future<…>`) is kept and parses as written.
//
// THE CANDIDATE, all of: the keyword `use` (not the raw identifier r#use) as code, followed by `<`; the previous code
// token is `+` (`… + use<..>`) or the keyword `impl` (`impl use<..> + Trait`); the `<…>` balances before any `;` `{`
// `}` `(` `)` `=` or end of file. The span blanked runs from the `+` (or from `use` after `impl`) through the closing
// `>`; after `impl`, a following `+` is blanked too so the bound list stays well formed. Comments, string, byte,
// C and raw string literals and char literals are skipped; a lifetime (`'a`) is not mistaken for a char literal.
//
// COST. The scan runs only on a Rust file whose first parse holds error bytes (one linear pass, no allocation beyond
// the span list); the second parse only when the scan names at least one bound.

#include "infra/namesplit.h"   // namesplit::isIdentStart / isIdentChar — the ONE ASCII identifier byte class
#include "macroreparse.h"      // BlankSpan + blankInvocations + adoptsReparse — the family's shared half

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rw::rustusebounds
{

enum class TokKind : std::uint8_t { End, Word, Punct, Literal };

struct Tok
{
    TokKind       kind  = TokKind::End;
    char          punct = 0;   // a Punct token's byte; 0 on every other token
    std::uint32_t start = 0;
    std::uint32_t end   = 0;
    std::uint32_t line  = 1;   // 1-based line of `start`
};

struct Cursor
{
    std::string_view src;
    std::uint32_t    pos  = 0;
    std::uint32_t    line = 1;
};

inline bool at( const Cursor& c, std::uint32_t pos, std::string_view text ) noexcept
{
    return pos <= c.src.size() && c.src.substr( pos ).starts_with( text );
}

// Advance to `to`, counting the newlines crossed.
inline void moveTo( Cursor& c, std::uint32_t to ) noexcept
{
    for( ; c.pos < to; ++c.pos )
    {
        c.line += c.src[ c.pos ] == '\n' ? 1u : 0u;
    }
}

// `/* … */`, nested as Rust nests them; unterminated runs to the end of the source.
inline void skipBlockComment( Cursor& c ) noexcept
{
    std::uint32_t depth = 0;
    std::uint32_t p     = c.pos;
    const auto    size  = std::uint32_t( c.src.size() );
    while( p < size )
    {
        if( at( c, p, "/*" ) )
        {
            ++depth;
            p += 2;
        }
        else if( at( c, p, "*/" ) )
        {
            p += 2;
            if( --depth == 0 )
            {
                break;
            }
        }
        else
        {
            ++p;
        }
    }
    moveTo( c, p );
}

// Whitespace and comments. Returns with c.pos on code or at the end.
inline void skipTrivia( Cursor& c ) noexcept
{
    const auto size = std::uint32_t( c.src.size() );
    while( c.pos < size )
    {
        const char ch = c.src[ c.pos ];
        if( ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\f' || ch == '\v' )
        {
            moveTo( c, c.pos + 1 );
        }
        else if( at( c, c.pos, "//" ) )
        {
            const std::size_t nl = c.src.find( '\n', c.pos );
            moveTo( c, nl == std::string_view::npos ? size : std::uint32_t( nl ) );
        }
        else if( at( c, c.pos, "/*" ) )
        {
            skipBlockComment( c );
        }
        else
        {
            return;
        }
    }
}

// A `"…"` body from the opening quote at `quote`: escapes honoured, newlines allowed (Rust strings span lines).
inline void skipQuotedString( Cursor& c, std::uint32_t quote ) noexcept
{
    const auto    size = std::uint32_t( c.src.size() );
    std::uint32_t p    = quote + 1;
    while( p < size && c.src[ p ] != '"' )
    {
        p += ( c.src[ p ] == '\\' && p + 1 < size ) ? 2u : 1u;
    }
    moveTo( c, p < size ? p + 1 : size );
}

// A raw string `r#*"…"#*` whose `r` sits at `r` (any b/c prefix already stepped over): 0 when it is not one.
inline std::uint32_t rawStringEnd( const Cursor& c, std::uint32_t r ) noexcept
{
    std::uint32_t p = r + 1;
    while( p < c.src.size() && c.src[ p ] == '#' )
    {
        ++p;
    }
    if( p >= c.src.size() || c.src[ p ] != '"' )
    {
        return 0;
    }
    const std::uint32_t hashes = p - ( r + 1 );
    std::string         closer( 1, '"' );
    closer.append( hashes, '#' );
    const std::size_t close = c.src.find( closer, p + 1 );
    return close == std::string_view::npos ? std::uint32_t( c.src.size() ) : std::uint32_t( close + closer.size() );
}

// The byte length of the UTF-8 sequence led by `lead` (1 for ASCII and for a stray continuation byte).
inline std::uint32_t utf8Length( unsigned char lead ) noexcept
{
    return lead >= 0xF0 ? 4u : lead >= 0xE0 ? 3u : lead >= 0xC0 ? 2u : 1u;
}

// At a `'`: a char literal (`'a'`, `'\n'`, `'\u{1F600}'`, `'é'`) is skipped whole; a lifetime or label (`'a`,
// `'static`) is skipped as one word. Either way it is never a token the candidate rule reads.
inline void skipQuoteOrLifetime( Cursor& c ) noexcept
{
    const auto          size = std::uint32_t( c.src.size() );
    const std::uint32_t q    = c.pos;
    if( q + 1 < size && c.src[ q + 1 ] == '\\' )
    {
        std::uint32_t p = q + 2;
        while( p < size && c.src[ p ] != '\'' && c.src[ p ] != '\n' )
        {
            ++p;
        }
        moveTo( c, p < size && c.src[ p ] == '\'' ? p + 1 : p );
        return;
    }
    const std::uint32_t after = q + 1 + ( q + 1 < size ? utf8Length( (unsigned char)c.src[ q + 1 ] ) : 1u );
    if( after < size && c.src[ after ] == '\'' )
    {
        moveTo( c, after + 1 );   // a one-character literal
        return;
    }
    std::uint32_t p = q + 1;
    while( p < size && namesplit::isIdentChar( c.src[ p ] ) )
    {
        ++p;
    }
    moveTo( c, p );
}

// A word at c.pos — or the literal it prefixes (b"…", c"…", br"…", cr#"…"#, r"…", b'x'), or a raw identifier
// r#name, which is lexed as a Literal so `r#use` is never the keyword.
inline TokKind lexWord( Cursor& c ) noexcept
{
    const auto          size  = std::uint32_t( c.src.size() );
    const std::uint32_t start = c.pos;
    std::uint32_t       p     = start;
    while( p < size && namesplit::isIdentChar( c.src[ p ] ) )
    {
        ++p;
    }
    const std::string_view word = c.src.substr( start, p - start );
    if( p < size && c.src[ p ] == '"' && ( word == "b" || word == "c" ) )
    {
        skipQuotedString( c, p );
        return TokKind::Literal;
    }
    if( p < size && c.src[ p ] == '\'' && word == "b" )
    {
        moveTo( c, p );
        skipQuoteOrLifetime( c );
        return TokKind::Literal;
    }
    if( word == "r" || word == "br" || word == "cr" )
    {
        const std::uint32_t rawEnd = rawStringEnd( c, p - 1 );
        if( rawEnd != 0 )
        {
            moveTo( c, rawEnd );
            return TokKind::Literal;
        }
        if( word == "r" && p + 1 < size && c.src[ p ] == '#' && namesplit::isIdentStart( c.src[ p + 1 ] ) )
        {
            p += 1;
            while( p < size && namesplit::isIdentChar( c.src[ p ] ) )
            {
                ++p;
            }
            moveTo( c, p );
            return TokKind::Literal;   // r#name: a raw identifier, never a keyword
        }
    }
    moveTo( c, p );
    return TokKind::Word;
}

inline Tok nextTok( Cursor& c ) noexcept
{
    skipTrivia( c );
    Tok t;
    t.start = c.pos;
    t.line  = c.line;
    if( c.pos >= c.src.size() )
    {
        t.end = c.pos;
        return t;
    }
    const char ch = c.src[ c.pos ];
    if( ch == '\'' )
    {
        skipQuoteOrLifetime( c );   // never a candidate token: lexed as a Literal
        t.kind = TokKind::Literal;
    }
    else if( ch == '"' )
    {
        skipQuotedString( c, c.pos );
        t.kind = TokKind::Literal;
    }
    else if( namesplit::isIdentChar( ch ) )
    {
        t.kind = lexWord( c );
    }
    else
    {
        t.kind  = TokKind::Punct;
        t.punct = ch;
        moveTo( c, c.pos + 1 );
    }
    t.end = c.pos;
    return t;
}

inline bool isWord( const Tok& t, std::string_view src, std::string_view word ) noexcept
{
    return t.kind == TokKind::Word && src.substr( t.start, t.end - t.start ) == word;
}

inline bool isPunct( const Tok& t, char ch ) noexcept
{
    return t.kind == TokKind::Punct && t.punct == ch;
}

// `c` sits just past `use`. The next code token must be `<`, balanced before a structural byte; on a match `c` is left
// past the closing `>` and its token returned.
inline bool matchUseList( Cursor& c, Tok& close ) noexcept
{
    Cursor probe = c;
    if( !isPunct( nextTok( probe ), '<' ) )
    {
        return false;
    }
    std::uint32_t depth = 1;
    for( Tok t = nextTok( probe ); t.kind != TokKind::End; t = nextTok( probe ) )
    {
        if( t.kind == TokKind::Punct && ( t.punct == ';' || t.punct == '{' || t.punct == '}' || t.punct == '(' || t.punct == ')' || t.punct == '=' ) )
        {
            return false;
        }
        depth += isPunct( t, '<' ) ? 1u : 0u;
        depth -= isPunct( t, '>' ) ? 1u : 0u;
        if( depth == 0 )
        {
            close = t;
            c     = probe;
            return true;
        }
    }
    return false;
}

// Every precise-capturing bound in `src`, in source order (see THE CANDIDATE). Deterministic in the bytes alone.
inline void findPreciseCaptureBounds( std::string_view src, std::vector<macroreparse::BlankSpan>& out )
{
    out.clear();
    if( src.size() >= 0xFFFFFFFFull )
    {
        return;   // offsets are 32-bit; the crawl's size ceiling keeps every indexed file far below this
    }
    Cursor c{ src };
    Tok    prev;
    for( Tok t = nextTok( c ); t.kind != TokKind::End; t = nextTok( c ) )
    {
        const bool afterPlus = isPunct( prev, '+' );
        const bool afterImpl = isWord( prev, src, "impl" );
        Tok        close;
        if( isWord( t, src, "use" ) && ( afterPlus || afterImpl ) && matchUseList( c, close ) )
        {
            macroreparse::BlankSpan span{ afterPlus ? prev.start : t.start, close.end, t.end, afterPlus ? prev.line : t.line };
            if( afterImpl )
            {
                Cursor probe = c;
                const Tok next = nextTok( probe );
                if( isPunct( next, '+' ) )
                {
                    span.endByte = next.end;
                    c            = probe;
                }
            }
            out.push_back( span );
            prev = Tok{};   // the blanked bound is gone: what follows sees no `+` or `impl` before it
            continue;
        }
        prev = t;
    }
}

}   // namespace rw::rustusebounds
