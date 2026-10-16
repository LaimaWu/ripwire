/* a same-named prototype in another file: a foreign declaration of the static helper in helper_a.c */
int helper(int x);
int UseB(void) { return helper(2); }
