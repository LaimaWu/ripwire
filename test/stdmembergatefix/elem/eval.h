#pragma once
// ELEMENT receivers (test/stdmembergatecheck.sh arm X): EvalString and Piece are the tree's only `empty` definitions in reach
// of elem/, so a call on an element of a container of EvalString keeps the ladder's two via="name" rows; a container whose
// element is written in std, or names a class with no `empty`, stays declined. Rule mirrors the shape that motivated it: a
// class-scoped typedef of a std::map field reached through a std::unique_ptr local.
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

struct EvalString
{
    bool empty() const { return text_.empty(); }
    std::string text_;
};
struct Piece
{
    bool empty() const { return len_ == 0; }
    int len_ = 0;
};
struct Other
{
    int n_ = 0;
};
typedef std::vector<EvalString> EvalList;
typedef EvalList EvalList2;

struct Rule
{
    typedef std::map<std::string, EvalString, std::less<>> Bindings;
    Bindings                              bindings_;
    std::map<std::string, EvalString>     direct_;
    std::vector<EvalString>               vec_;
    std::map<std::string, std::string>    strs_;
    std::vector<Other>                    others_;
    bool own( const std::string& k ) { return bindings_[ k ].empty(); }
    bool ownThis( const std::string& k ) { return this->bindings_[ k ].empty(); }
    bool localHidesField() { std::vector<std::string> bindings_; return bindings_[ 0 ].empty(); }
};

// an in-tree template whose own operator[] yields Other, whatever its argument: not a std container's element access
namespace ns
{
template<class T> struct Grid
{
    Other& operator[]( int i ) { return cells_[ i ]; }
    Other cells_[ 4 ];
};
}   // namespace ns

extern std::map<int, EvalString> g_table;
