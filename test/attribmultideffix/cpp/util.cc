#include "util.h"
void CanonicalizePath(std::string* path, unsigned long* bits) {
  unsigned long len = path->size();
  CanonicalizePath(&(*path)[0], &len, bits);
}
void CanonicalizePath(char* path, unsigned long* len, unsigned long* bits) {
  *len = 0; *bits = 0; (void)path;
}
