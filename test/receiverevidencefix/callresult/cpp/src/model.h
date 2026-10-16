#pragma once
#include <functional>
#include <future>
#include <memory>

namespace cfg
{
struct Config
{
    int get() const;
    int size() const;
};

struct Other
{
    int get() const;
    int size() const;
};

using ConfigAlias = Config;

Config                  makeConfig();
const Config&           configRef();
Config*                 configPtr();
std::unique_ptr<Config> ownConfig();
ConfigAlias             makeAlias();
Config                  pick( int which );
Other                   pick( const char* name );
std::reference_wrapper<Config> wrapRef();
template<class T> T     create();

struct Factory
{
    Config make() const;
};
}

namespace cfg
{
Config                        async( int which );   // an in-tree namesake of std::async
template<class Config> Config create2();            // a template parameter named after the in-tree Config
}
