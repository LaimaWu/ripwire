// impact depth 2: reaches StringPiece::empty only, through UsesPiece
#include "string_piece.h"
bool UsesPiece(StringPiece sp);
bool RunPiece() {
  StringPiece sp{"", 0};
  return UsesPiece(sp);
}
