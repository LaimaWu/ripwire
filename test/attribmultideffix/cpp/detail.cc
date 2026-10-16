#include "detail.h"
namespace b { namespace detail { int tally(int x) { return x + 1; } } }
int CountA() { return a::detail::tally(1); }
int CountB() { return b::detail::tally(2); }
