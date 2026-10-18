#include <utility>
struct Wrap
{
    template <class... T> static int fwd( T&&... t )
    {
        const int got = p( std::forward<T>( t )... );   // a forwarded pack: any number of arguments
        return got;
    }
};
template <class... T> int call_p2( T... t ) { return p( t... ); }
