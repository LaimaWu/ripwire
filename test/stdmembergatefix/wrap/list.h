#pragma once
// ns::List is the tree's only class defining size / find / front, so before the gate every call below bound to it by name
// (via="name"). SP<T> is an in-tree wrapper with its own operator->; Plain is an in-tree class with neither.
namespace ns
{
class List
{
public:
    int size() const { return 0; }
    int find( int k ) const { return k; }
    int front() const { return 1; }
};
}   // namespace ns

template<class T> class SP
{
public:
    T* operator->() const { return p_; }
    T* p_ = nullptr;
};

struct Plain : ext::Base
{
    int other() const { return 2; }
};

struct Box2
{
    ns::List items_;
};

extern ns::List g_ext;

// type aliases (test/stdmembergatecheck.sh arm W): a typedef is also a Struct symbol at its name, a `using` is not; the gate
// reads both through their recorded alias target
typedef std::shared_ptr<ns::List> ListSP;
typedef std::unique_ptr<ns::List> ListUP;
using ListUsingSP = std::shared_ptr<ns::List>;
typedef std::vector<int> IntVec;
using IntVecUsing = std::vector<int>;
typedef std::unique_ptr<std::string> StrUP;
using QtListSP = QSharedPointer<ns::List>;
