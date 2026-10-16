/* a static (internal-linkage) definition: no prototype in another file declares it */
static int helper(int x) { return x + 1; }
int UseA(void) { return helper(1); }
