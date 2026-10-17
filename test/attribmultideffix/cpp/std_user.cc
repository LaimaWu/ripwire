// ninja-07 confounder: a std::string member call named like the selector — never a proven row of either empty
#include <string>
bool UsesStd(const std::string& s) { return s.empty(); }
