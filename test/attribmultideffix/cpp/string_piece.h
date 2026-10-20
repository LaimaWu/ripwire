#pragma once
#include <cstddef>
struct StringPiece {
  const char* str_;
  std::size_t len_;
  bool empty() const { return len_ == 0; }
};
