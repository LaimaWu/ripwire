namespace ca {
struct CBase { virtual void run() = 0; };
struct CMid : CBase { void run() override {} };
}
namespace cb {
struct CMid { };
struct CLeaf : CMid { };
}
namespace ca {
struct CTrue : CMid { };
struct CQual : ca::CMid { };
}
