#include <utility>
template <class... T> int call_p(T&&... t) { return p(std::forward<T>(t)...); }
template <class... T> int call_p2(T... t) { return p(t...); }
