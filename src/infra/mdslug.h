#pragma once

// mdslug.h — the ONE GitHub-style anchor slug of a markdown heading name.
//
// Two stages need it and neither can include the other: ingest_docs.h resolves an in-file [text](#slug) at
// extraction, and graph.h resolves a cross-file [text](other.md#slug) once every heading in the repo is a
// symbol. A leaf header keeps them on one definition (a second copy is the clone --quality-delta flags).

#include <string>
#include <string_view>

namespace rw
{

// alnum lowered, spaces→'-', '-'/'_' kept, everything else dropped. First match wins on collision
// (GitHub's -1/-2 suffixes and percent-encoded slugs are not modelled; such a link falls back to the
// target file's own edge — disclosed here rather than guessed).
inline std::string mdSlugOf( std::string_view name )
{
    std::string slug;
    slug.reserve( name.size() );
    for( const char c : name )
    {
        if( ( c >= 'a' && c <= 'z' ) || ( c >= '0' && c <= '9' ) || c == '_' || c == '-' ) { slug += c; }
        else if( c >= 'A' && c <= 'Z' ) { slug += char( c - 'A' + 'a' ); }
        else if( c == ' ' || c == '\t' ) { slug += '-'; }
    }
    return slug;
}

}   // namespace rw
