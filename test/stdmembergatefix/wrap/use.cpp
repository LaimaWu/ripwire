#include "list.h"
#include <QList>
#include <QSharedPointer>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// ---- kept: the receiver's recorded written type names ns::List, so the ladder's via="name" hedge stays ----
int uniqueLocal() { std::unique_ptr<ns::List> up; return up->size(); }
int sharedParam( const std::shared_ptr<ns::List>& p ) { return p->size(); }
int optionalLocal() { std::optional<ns::List> o; return o->find( 1 ); }
int inTreeWrapper( SP<ns::List>& p ) { return p->size(); }
int inTreeWrapperLocal() { SP<ns::List> p; return p->front(); }
int qtWrapper( QSharedPointer<ns::List>& p ) { return p->front(); }
int boostWrapper( boost::shared_ptr<ns::List>& p ) { return p->find( 2 ); }
int smartRootField() { std::unique_ptr<Box2> b; return b->items_.size(); }
ns::List g_list;
int globalVar() { return g_list.size(); }
namespace app
{
ns::List nsVar;
int nsGlobalVar() { return nsVar.front(); }
}   // namespace app
static ns::List fileStatic;
int fileStaticVar() { return fileStatic.find( 3 ); }
int externVar() { return g_ext.size(); }

// ---- still declined: nothing written names a class that defines the callee ----
int uniqueStd() { std::unique_ptr<std::string> p; return int( p->size() ); }
int rawStdPtr( std::vector<int>* pv ) { return int( pv->size() ); }
int uniqueStdVec() { std::unique_ptr<std::vector<int>> up; return int( up->size() ); }
int vectorOfList( std::vector<ns::List>& v ) { return int( v.size() ); }
int rawVectorOfListPtr( std::vector<ns::List>* pv ) { return int( pv->size() ); }
int plainPtr( Plain* p ) { return p->size(); }
std::string g_str;
int globalStd() { return int( g_str.size() ); }
int shadowedGlobal() { std::string g_list; return int( g_list.size() ); }
struct Shelf2
{
    std::string g_list;
    int fieldHidesGlobal() { return int( g_list.size() ); }
};
struct Shelf3
{
    std::vector<int> g_list;
    int stdFieldHidesGlobal() { return int( g_list.size() ); }
};

// ---- STATED FLOOR: the index records no written type for these receivers ----
struct Holder2
{
    ns::List& list();
};
int callResult( Holder2& h ) { return h.list().size(); }
int frontOfVector( std::vector<ns::List>& v ) { return v.front().size(); }
int iteratorArrow( std::vector<ns::List>& v ) { auto it = v.begin(); return it->size(); }
int unqualifiedTemplate( QList<ns::List>& l ) { return l.size(); }

// ---- aliases of a std smart pointer: the alias's recorded target decides, one rule for typedef and using ----
int typedefSharedLocal() { ListSP p; return p->size(); }
int typedefSharedParam( const ListSP& p ) { return p->size(); }
int typedefUniqueLocal() { ListUP p; return p->front(); }
int typedefUniqueParam( ListUP& p ) { return p->find( 1 ); }
int usingSharedLocal() { ListUsingSP p; return p->size(); }
int typedefVecDot() { IntVec v; return int( v.size() ); }
int typedefVecPtrArrow( IntVec* p ) { return int( p->size() ); }
int usingVecPtrArrow( IntVecUsing* p ) { return int( p->size() ); }
int usingQtAliasLocal() { QtListSP p; return p->front(); }
struct Owner4
{
    typedef std::shared_ptr<ns::List> Ptr;
    Ptr p_;
    int typedefField() { return p_->size(); }
};
int classScopedTypedef( Owner4::Ptr p ) { return p->size(); }
int localTypedef() { typedef std::shared_ptr<ns::List> LocalSP; LocalSP p; return p->size(); }
int typedefStrUP() { StrUP p; return int( p->size() ); }
struct Holder3
{
    std::optional<ns::List> o_;
    int optionalField() { return o_->size(); }
};
