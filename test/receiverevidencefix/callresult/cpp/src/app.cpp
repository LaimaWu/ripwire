#include "model.h"

// usePlain: `auto x = f()` with a value result.
int usePlain()
{
    auto c = cfg::makeConfig();
    return c.get();
}

// useRef: `T&` result.
int useRef()
{
    const auto& c = cfg::configRef();
    return c.get();
}

// usePtr: `T*` result through `->`.
int usePtr()
{
    auto p = cfg::configPtr();
    return p->get();
}

// useUnique: std::unique_ptr<T> through `->`.
int useUnique()
{
    auto u = cfg::ownConfig();
    return u->get();
}

// useAlias: the result names a `using` alias of an in-tree class.
int useAlias()
{
    auto a = cfg::makeAlias();
    return a.get();
}

// useMethod: the callee is a method on a typed local.
int useMethod()
{
    cfg::Factory f;
    auto c = f.make();
    return c.get();
}

// nearOverload: the callee is overloaded (two definitions).
int nearOverload()
{
    auto x = cfg::pick( 1 );
    return x.get();
}

// nearExternal: an outside callee whose result has a same-named member.
int nearExternal()
{
    auto fut = std::async( [] { return 1; } );
    return fut.get();
}

// nearWrapper: std::reference_wrapper is outside the list; its own get() runs.
int nearWrapper()
{
    auto r = cfg::wrapRef();
    return r.get().size();
}

// nearGeneric: a template-parameter result.
int nearGeneric()
{
    auto g = cfg::create<cfg::Config>();
    return g.get();
}

// nearShadow: an inner `c` hides the typed outer one.
int nearShadow()
{
    auto c = cfg::makeConfig();
    {
        auto c = std::async( [] { return 2; } );
        return c.get();
    }
}

// useWritten (twin): the same call with the type written.
int useWritten()
{
    cfg::Config c = cfg::makeConfig();
    return c.get();
}
