#include "parser.h"
#include "util.h"
#include "string_piece.h"
bool ManifestParser::ParseDefault(std::string* err) {
  EvalString eval;
  lexer_.ReadPath(&eval);
  if (eval.empty()) return false;
  std::string path = "x";
  unsigned long bits;
  CanonicalizePath(&path, &bits);
  return true;
}
bool UsesPiece(StringPiece sp) {
  return sp.empty();
}
bool UsesEval(EvalString ev) {
  return ev.empty();
}
bool UsesBoth(StringPiece sp, EvalString ev) {
  return sp.empty() && ev.empty();
}
