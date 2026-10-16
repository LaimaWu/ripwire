#include "model.h"

namespace cfg
{
int Config::get() const { return 0; }
int Config::size() const { return 0; }
int Other::get() const { return 1; }
int Other::size() const { return 1; }
Config makeConfig() { return {}; }
const Config& configRef() { static Config c; return c; }
Config* configPtr() { return nullptr; }
std::unique_ptr<Config> ownConfig() { return {}; }
ConfigAlias makeAlias() { return {}; }
Config pick( int ) { return {}; }
Other pick( const char* ) { return {}; }
std::reference_wrapper<Config> wrapRef() { static Config c; return c; }
Config Factory::make() const { return {}; }
}
