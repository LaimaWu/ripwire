/* R1 (final review): a definition, an untied prototype in another program, a static namesake in a third */
int scan_tok(const char *s) { return s[0]; }
int use_tok1(void) { return scan_tok("a"); }
