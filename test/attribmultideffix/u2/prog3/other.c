/* a static (internal-linkage) namesake: another function */
static int scan_tok(const char *s) { return 7; }
int use_tok3(void) { return scan_tok("c"); }
