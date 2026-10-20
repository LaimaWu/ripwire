#pragma once
// a foreign declaration: same name, same leaf scope `detail`, same parameter types — another scope chain
namespace a { namespace detail { int tally(int x); } }
