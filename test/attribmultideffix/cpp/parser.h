#pragma once
#include "lexer.h"
struct ManifestParser {
  Lexer lexer_;
  bool ParseDefault(std::string* err);
};
