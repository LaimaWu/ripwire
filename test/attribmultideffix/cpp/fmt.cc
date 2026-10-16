#include "fmt.h"
void Fmt::put(int v) { (void)v; }
void Fmt::put(const char* s) { (void)s; }
void outer::run() {}
void outer::inner::run() {}
void PutInt(Fmt& f) { f.put(1); }
void PutStr(Fmt& f) { f.put("x"); }
void RunOuter() { outer::run(); }
void RunInner() { outer::inner::run(); }
int GetInt(const Box<int>& b) { return b.get(); }
double GetDouble(const Box<double>& b) { return b.get(); }
