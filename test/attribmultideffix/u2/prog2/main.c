/* a prototype with no #include that ties it to prog1/util.c: the identity rule can only say Unproven */
int scan_tok(const char *s);
int use_tok2(void) { return scan_tok("b"); }
