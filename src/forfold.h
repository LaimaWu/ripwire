#pragma once
// forfold.h — THE BODY-TAIL FOLD of a compact-route --for answer (lane body-fold, train 31): the whole bodies of its top ranked
// rows, appended after the unchanged answer under a fixed byte budget.
//
// WHY. A conceptual --for question routes to the compact bundle: ranked signatures, one-hop edges, the file tail, no bodies
// (bodies="0" reason="compact-route"). Its own continuation is "--expand the row you want". Measured over the round-5 rows, the
// one follow-up that ever lifted a row without names-only credit was reading one to three of the top ranked bodies; the existing
// knobs that serve bodies (--auto-bodies, --detail=N) REPLACE ranked rows and lost 2-5 complete answers. The fold APPENDS
// instead: every byte before it is the answer without it (RIPWIRE_NO_BODY_FOLD=1), the fold is a fixed stair-step on top of
// the answer's budget_bytes target, never a share of it.
//
// WHAT. The first kForBodyFoldRows ranked <d> rows (rank order) that are not already bodied (the body slot's owner) and have a
// body to slice (never a module scope). Each body is whole or not served: in rank order, a body whose rendered element fits
// what is left of kForBodyFoldBudgetBytes is served; one that does not is listed as <skip p= n= l= bytes= next="--expand=…">,
// the call that serves it, and the walk goes on to the next row (a later, smaller body can still fit). A body here is the
// --expand body element (<b t= l= p= n=> with its CDATA) WITHOUT the --expand file context (sibs=, inc=) and WITHOUT callee
// <calls> rows: the one-hop edges are the answer's <hops>, and the next-hop false adds a reader drew from callee rows of
// expanded bodies were the measured risk (dn-compound F2). The bodies are by rank, not a claim they are the answer.
// THE MEMBER SLICE (owner-approved variant V1, the STEP-0 pick): a CLASS/struct/interface row whose whole body does not fit what is
// left is not just skipped — its constructor (constructor, __init__, ctor, initialize, init, new, or the class's own name) and every
// member whose name or body text holds a question word (the query's subtokens less the stopword lists) are served, each whole or not
// at all, in source order, inside <slice p= n= l= members= of=>, and the class itself is still the <skip> that names the whole body.
// A member is the parser's (isParsedMemberOf: its scope names the class, or the class's span holds it), never one nested in another
// member (that text is already inside the outer member). Measured on the 53 round-5 --for rows: +1 Rule-C row over the plain fold
// (koa-02: the Application slice), 0 lost, +0.8 KB median.
// Funding: the DEFAULT regime only — an explicit --token-budget is a size the caller named, and the fold does not ride there
// (that answer is byte-identical to before); --json, --signatures-only, --detail=N, --auto-bodies and the name-exact route
// (which already serves the anchor's body) carry none either.
// RIPWIRE_NO_BODY_FOLD (any value) turns the fold off — the A/B handle, the RIPWIRE_NO_BODY_SLOT precedent.

#include "commentcoherence.h"   // isCommentStopword — the member slice's question words
#include "docparse.h"   // docparse::detail::readWholeFile — a sliced class's member text
#include "forowner.h"   // isParsedMemberOf — a class's members BY THE PARSER (the body slot's member reading)
#include "lexical.h"    // subtokens, isRouteStopword
#include "model.h"
#include "nextverb.h"   // nextFlag
#include "serialize.h"  // packBodies, chargeSection, redactTakeBack, tokensForEmittedBytes

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rw
{

// The fold's byte budget: the served body elements' bytes (markup and CDATA), never the envelope or the skip rows. A fixed step
// on top of the answer (owner, 2026-10-17: 4 KB). A content cap with a disclosed, recoverable cut: every body it leaves out is a
// <skip> row naming the call that serves it.
inline constexpr std::size_t kForBodyFoldBudgetBytes = 4000;
// How many ranked rows the fold considers (served or skipped): the top three (dn-compound C1, measured at 1 / 3 rows).
inline constexpr std::size_t kForBodyFoldRows = 3;

// The A/B switch: the fold rides unless RIPWIRE_NO_BODY_FOLD is set. The caller adds the posture (compact route, XML, default regime).
inline bool forBodyFoldOn()
{
    return std::getenv( "RIPWIRE_NO_BODY_FOLD" ) == nullptr;
}

// The candidates: the first kForBodyFoldRows rows of the ranked head (rank order; the first `headRows` entries of the surface
// with a positive score, the rows <sigs> prints) that are not `bodied` (the body slot's owner, already served whole) and have a
// span to slice (a module scope is bodyless by construction).
inline std::vector<NodeId> forBodyFoldCandidates( const IngestResult& ing, const std::vector<NodeId>& lensSurfaceIds, const std::vector<float>& lensRank,
                                                  std::size_t headRows, NodeId bodied )
{
    std::vector<NodeId> out;
    for( std::size_t i = 0; i < lensSurfaceIds.size() && i < headRows && out.size() < kForBodyFoldRows; ++i )
    {
        const NodeId id = lensSurfaceIds[i];
        if( id >= ing.symbols.size() || id >= lensRank.size() || lensRank[id] <= 0.0f || id == bodied )
        {
            continue;
        }
        const Symbol& s = ing.symbols[id];
        if( s.kind == SymKind::ModuleScope || s.endByte <= s.sigStartByte )
        {
            continue;
        }
        out.push_back( id );
    }
    return out;
}

// The fold's legend clause (both dialects), present-only beside the element. No "--": it rides inside an XML comment.
inline constexpr std::string_view kForBodyFoldLegend =
    "; fold kind=bodies: the whole bodies of the top ranked rows not already bodied, by rank, not a claim they are the answer "
    "(b t= n= p= l=, no sibs= and no callee rows); served= of total= rows, each whole or not at all within budget_bytes= "
    "(the served bodies' bytes, on top of the answer above, which is unchanged without it); slice p= n= l= members= of=: a class "
    "too big for what is left, its constructor and the members whose name or body holds a question word, each whole, members= "
    "served of its of= members; skip p= n= l= bytes= its source size, next= the call that serves it; sigs next= is then the first "
    "skip's next=, sigs_next= the signatures-only re-call";

struct BodyFoldInputs
{
    const IngestResult&     ing;
    std::string_view        task;         // the question: the member slice's words
    NodeId                  bodied;       // the body slot's owner (kNoNode: none) — already served, never repeated
    const std::vector<float>& lensRank;   // orders nothing here (no <calls>); threaded so a body renders as every --for body
    std::string_view        rootArg;
    RedactCounts*           redact;
    const notes::NoteIndex* notes;
    bool                    compress;     // --compress (the CLI only)
};

struct BodyFoldRender
{
    std::string xml;                 // <fold …>…</fold>; empty when nothing rides (no candidate, or a render buffer failed)
    std::size_t bodyTokens  = 0;     // the served body elements, at the body rate
    std::size_t markupBytes = 0;     // the envelope and the skip rows, priced with the rest of the markup
    std::size_t served      = 0;
    std::size_t total       = 0;
    std::size_t slices = 0;   // classes served as a member slice (each also has its skip row)
    std::string firstSkipNext;       // the first skip's next= value, XML-escaped; empty when every candidate was served
};

namespace fold_detail
{
// the member slice's question words: the query's subtokens (the BM25 split), less the general-English and route stopwords, deduped
inline std::vector<std::string> sliceWords( std::string_view task )
{
    std::vector<std::string> toks;
    subtokens( task, toks );
    std::vector<std::string> out;
    for( std::string& t : toks )
    {
        if( !isCommentStopword( t ) && !isRouteStopword( t ) && std::find( out.begin(), out.end(), t ) == out.end() )
        {
            out.push_back( std::move( t ) );
        }
    }
    return out;
}
inline bool isSliceableKind( SymKind k ) noexcept
{
    return k == SymKind::Class || k == SymKind::Struct || k == SymKind::Interface;
}
// a constructor by name: the spellings the languages use, or the class's own name (C++, Java, C#)
inline bool isConstructorName( std::string_view member, std::string_view cls ) noexcept
{
    static constexpr std::string_view kCtorNames[] = { "__init__", "constructor", "ctor", "init", "initialize", "new" };
    return member == cls || std::ranges::find( kCtorNames, member ) != std::end( kCtorNames );
}
// does `text` (a member's name, or its body) carry one of `words` as a subtoken
inline bool holdsWord( std::string_view text, const std::vector<std::string>& words )
{
    std::vector<std::string> toks;
    subtokens( text, toks );
    return std::ranges::any_of( toks, [ & ]( const std::string& t ) { return std::ranges::find( words, t ) != words.end(); } );
}
// One body through packBodies (the --expand body shape, no callee rows: the empty CSR has no out-edge for any id), unwrapped from
// its <bodies> envelope: the legend comments packBodies writes ahead of the envelope (an extent-suspect body's reading) stay in
// front of the <b>. nullopt when the buffer failed or the span rendered nothing.
inline std::optional<std::string> renderOneBody( const BodyFoldInputs& in, NodeId id )
{
    static const std::vector<std::uint32_t> kNoOff;
    static const std::vector<NodeId>        kNoTargets;
    static const std::vector<std::uint8_t>  kNoNameOnly;
    ChargedSection sec = chargeSection( [ & ]( std::FILE* f )
        { packBodies( f, in.ing, { id }, /*budgetBytes=*/0, kNoOff, kNoTargets, CalleeEdgeBytes( kNoNameOnly ), in.compress, in.redact,
                      /*ranges=*/nullptr, in.notes, /*outEmitted=*/nullptr, /*truncateOversizedFirst=*/false,
                      /*withFileContext=*/false, in.rootArg, &in.lensRank, {} ); },
        kBytesPerTokenBody );
    if( !sec.isRendered )
    {
        return std::nullopt;
    }
    std::string&      x     = sec.xml;
    const std::size_t open  = x.find( "<bodies shown=" );
    const std::size_t close = open == std::string::npos ? std::string::npos : x.find( '>', open );
    constexpr std::string_view kClose = "</bodies>";
    if( close == std::string::npos || !x.ends_with( kClose ) || x.find( "<b t=\"", close ) == std::string::npos )
    {
        return std::nullopt;   // an unreadable span renders an empty envelope: nothing to serve
    }
    return x.substr( 0, open ) + x.substr( close + 1, x.size() - kClose.size() - close - 1 );
}

inline std::string skipRow( const BodyFoldInputs& in, NodeId id, std::string& escapedNext )
{
    const Symbol&     s          = in.ing.symbols[id];
    const std::string rootPrefix = in.rootArg.empty() ? std::string() : rw::sarif::rootPrefixOf( in.rootArg );
    const std::string path       = in.rootArg.empty() ? std::string( in.ing.files[s.fileId] )
                                                      : std::string( rw::sarif::rootRelativeUri( in.ing.files[s.fileId], rootPrefix ) );
    std::vector<char> esc;
    escapedNext = std::string( escapeXml( nextFlag( "--expand=", path + ":" + std::to_string( s.line ) + ":" + s.name ), esc ) );
    std::string x = "<skip p=\"";
    x += escapeXml( path, esc );
    x += "\" n=\"";
    x += escapeXml( s.name, esc );
    x += "\" l=\"" + std::to_string( s.line ) + "\" bytes=\"" + std::to_string( s.endByte - s.sigStartByte ) + "\" next=\"";
    x += escapedNext;
    x += "\"/>";
    return x;
}

// THE MEMBER SLICE of one class row that did not fit: its constructor first, then the members whose name or body holds a question
// word, each whole within `room` bytes (a member that does not fit what is left is passed over, the walk goes on), emitted in source
// order. Empty xml when no member fits (the class's skip row alone then stands for it).
struct SliceRender
{
    std::string xml;
    std::size_t bodyBytes = 0;   // the served member elements' bytes (charged to the fold budget)
};
inline SliceRender renderSlice( const BodyFoldInputs& in, NodeId clsId, const std::vector<std::string>& words, std::size_t room,
                                std::vector<NodeId>& served )   // in: the bodies the fold (and the slot) already carry; out: + this slice's
{
    SliceRender   out;
    const Symbol& c = in.ing.symbols[clsId];
    std::vector<NodeId> members;
    for( NodeId m = 0; m < in.ing.symbols.size(); ++m )
    {
        const Symbol& ms = in.ing.symbols[m];
        if( ms.kind != SymKind::ModuleScope && ms.endByte > ms.sigStartByte && isParsedMemberOf( ms, c ) )
        {
            members.push_back( m );
        }
    }
    const auto inside = [ & ]( NodeId a, NodeId b )   // a's text lies in member b's
    {
        const Symbol& x = in.ing.symbols[a];
        const Symbol& y = in.ing.symbols[b];
        return a != b && x.fileId == y.fileId && y.sigStartByte <= x.sigStartByte && x.endByte <= y.endByte
            && ( y.sigStartByte < x.sigStartByte || x.endByte < y.endByte || b < a );
    };
    std::erase_if( members, [ & ]( NodeId a ) { return std::ranges::any_of( members, [ & ]( NodeId b ) { return inside( a, b ); } ); } );
    std::ranges::sort( members, [ & ]( NodeId a, NodeId b )
    {
        const Symbol& x = in.ing.symbols[a];
        const Symbol& y = in.ing.symbols[b];
        return x.fileId != y.fileId ? x.fileId < y.fileId : ( x.sigStartByte != y.sigStartByte ? x.sigStartByte < y.sigStartByte : a < b );
    } );
    // the member texts, one read per file (a C++ out-of-line member lives in another file than its class)
    std::vector<std::pair<std::uint32_t, std::string>> files;
    const auto textOf = [ & ]( const Symbol& ms ) -> std::string_view
    {
        auto it = std::ranges::find_if( files, [ & ]( const auto& f ) { return f.first == ms.fileId; } );
        if( it == files.end() )
        {
            files.emplace_back( ms.fileId, docparse::detail::readWholeFile( diskPath( in.ing, ms.fileId ) ).value_or( std::string() ) );
            it = files.end() - 1;
        }
        const std::string& src = it->second;
        return ms.endByte <= src.size() ? std::string_view( src ).substr( ms.sigStartByte, ms.endByte - ms.sigStartByte ) : std::string_view();
    };
    std::vector<NodeId> chosen;
    for( const NodeId m : members )
    {
        if( isConstructorName( in.ing.symbols[m].name, c.name ) )
        {
            chosen.push_back( m );
        }
    }
    for( const NodeId m : members )
    {
        const Symbol& ms = in.ing.symbols[m];
        if( !isConstructorName( ms.name, c.name ) && ( holdsWord( ms.name, words ) || holdsWord( textOf( ms ), words ) ) )
        {
            chosen.push_back( m );
        }
    }
    std::vector<std::pair<std::size_t, std::string>> kept;   // (source-order index, element)
    for( const NodeId m : chosen )
    {
        const Symbol& ms = in.ing.symbols[m];
        if( std::ranges::find( served, m ) != served.end() )
        {
            continue;   // already a body of this answer (a ranked row the fold served, or the slot's owner): never twice
        }
        if( out.bodyBytes + ( ms.endByte - ms.sigStartByte ) > room )
        {
            continue;   // the span alone is over what is left: never rendered
        }
        const std::optional<RedactCounts> before = in.redact != nullptr ? std::optional<RedactCounts>( *in.redact ) : std::nullopt;
        const std::optional<std::string>  piece  = renderOneBody( in, m );
        if( piece && out.bodyBytes + piece->size() <= room )
        {
            out.bodyBytes += piece->size();
            kept.emplace_back( std::size_t( std::ranges::find( members, m ) - members.begin() ), *piece );
            served.push_back( m );
            continue;
        }
        rw::redactTakeBack( in.redact, before, in.redact != nullptr ? std::optional<RedactCounts>( *in.redact ) : std::nullopt );
    }
    if( kept.empty() )
    {
        return out;
    }
    std::ranges::sort( kept, {}, &std::pair<std::size_t, std::string>::first );
    std::vector<char> esc;
    const std::string rootPrefix = in.rootArg.empty() ? std::string() : rw::sarif::rootPrefixOf( in.rootArg );
    const std::string path       = in.rootArg.empty() ? std::string( in.ing.files[c.fileId] )
                                                      : std::string( rw::sarif::rootRelativeUri( in.ing.files[c.fileId], rootPrefix ) );
    out.xml = "<slice p=\"";
    out.xml += escapeXml( path, esc );
    out.xml += "\" n=\"";
    out.xml += escapeXml( c.name, esc );
    out.xml += "\" l=\"" + std::to_string( c.line ) + "\" members=\"" + std::to_string( kept.size() ) + "\" of=\"" + std::to_string( members.size() ) + "\">";
    for( const auto& [ idx, x ] : kept )
    {
        out.xml += x;
    }
    out.xml += "</slice>";
    return out;
}
}   // namespace fold_detail

// THE FOLD'S RENDER: the walk the header comment states. A body that does not fit is never rendered into the answer, and the
// redaction count its render added is taken back (it is not served).
inline BodyFoldRender renderForBodyFold( const BodyFoldInputs& in, const std::vector<NodeId>& candidates )
{
    BodyFoldRender out;
    out.total = candidates.size();
    if( candidates.empty() )
    {
        return out;
    }
    std::string bodies, skips;
    std::size_t used = 0;
    std::optional<std::vector<std::string>> words;   // the member slice's question words, split on the first slice
    std::vector<NodeId> served;                       // every body this answer carries: the slot's owner, then the fold's
    if( in.bodied != kNoNode )
    {
        served.push_back( in.bodied );
    }
    for( const NodeId id : candidates )
    {
        if( std::ranges::find( served, id ) != served.end() )
        {
            ++out.served;   // a ranked row an earlier slice already served whole: counted, never repeated
            continue;
        }
        EXPECTS( id < in.ing.symbols.size(), "a fold candidate is an indexed symbol (forBodyFoldCandidates)" );
        const Symbol&     s       = in.ing.symbols[id];
        const std::size_t span    = s.endByte - s.sigStartByte;
        std::string       next;
        // the source span is a lower bound of the element (markup only adds bytes), so a span over what is left skips unrendered
        if( used + span <= kForBodyFoldBudgetBytes )
        {
            const std::optional<RedactCounts> before = in.redact != nullptr ? std::optional<RedactCounts>( *in.redact ) : std::nullopt;
            const std::optional<std::string>  piece  = fold_detail::renderOneBody( in, id );
            if( piece && used + piece->size() <= kForBodyFoldBudgetBytes )
            {
                bodies += *piece;
                used += piece->size();
                ++out.served;
                served.push_back( id );
                continue;
            }
            rw::redactTakeBack( in.redact, before, in.redact != nullptr ? std::optional<RedactCounts>( *in.redact ) : std::nullopt );
        }
        if( fold_detail::isSliceableKind( s.kind ) )
        {
            if( !words )
            {
                words = fold_detail::sliceWords( in.task );
            }
            const fold_detail::SliceRender slice = fold_detail::renderSlice( in, id, *words, kForBodyFoldBudgetBytes - used, served );
            bodies += slice.xml;
            used += slice.bodyBytes;
            out.slices += slice.bodyBytes > 0 ? 1u : 0u;
        }
        skips += fold_detail::skipRow( in, id, next );
        if( out.firstSkipNext.empty() )
        {
            out.firstSkipNext = next;
        }
    }
    const std::string open = "<fold kind=\"bodies\" budget_bytes=\"" + std::to_string( kForBodyFoldBudgetBytes ) + "\" served=\""
                           + std::to_string( out.served ) + "\" total=\"" + std::to_string( out.total ) + "\">";
    constexpr std::string_view kClose = "</fold>";
    out.xml         = open + bodies + skips + std::string( kClose );
    out.bodyTokens  = tokensForEmittedBytes( used, kBytesPerTokenBody );
    out.markupBytes = out.xml.size() - used;
    ENSURES( used <= kForBodyFoldBudgetBytes, "the served bodies fit the fold's budget" );
    return out;
}

// C3 NEXT ORDER (dn-compound): on an answer that carries the fold, the <sigs> continuation an agent meets first is the call that
// ever lifted a row — the first skipped body's --expand — and the signatures-only re-call of the same rows (0 items lifted in 20
// executions) moves to sigs_next=, its reading renamed with it. Only the <sigs> open tag is touched; nothing when it has no next=
// and no body was skipped.
inline constexpr std::string_view kForFoldSigsNextNote = " [sigs sigs_next=: one call serving all total= rows uncut]";
inline void spliceFoldSigsNext( std::string& sigs, std::string& sigsNote, std::string_view firstSkipNext )
{
    std::size_t open = sigs.find( "<sigs" );
    while( open != std::string::npos && open + 5 < sigs.size() && sigs[open + 5] != ' ' && sigs[open + 5] != '>' )
    {
        open = sigs.find( "<sigs", open + 5 );   // the open tag itself, never a longer element name
    }
    std::size_t close = open == std::string::npos ? std::string::npos : sigs.find( '>', open );
    if( close == std::string::npos )
    {
        return;
    }
    constexpr std::string_view kNext = " next=\"";
    if( const std::size_t at = sigs.find( kNext, open ); at != std::string::npos && at < close )
    {
        sigs.replace( at, kNext.size(), " sigs_next=\"" );
        close += 5;   // "sigs_"
        if( const std::size_t n = sigsNote.find( kForSigsNextNote ); n != std::string::npos )
        {
            sigsNote.replace( n, kForSigsNextNote.size(), kForFoldSigsNextNote );
        }
    }
    if( !firstSkipNext.empty() )
    {
        sigs.insert( close, " next=\"" + std::string( firstSkipNext ) + "\"" );
    }
}

}   // namespace rw
