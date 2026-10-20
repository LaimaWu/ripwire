#pragma once
struct Fmt {
  void put(int v);
  void put(const char* s);
};
namespace outer {
void run();
namespace inner {
void run();
}
}
template <typename T> struct Box {
  T value;
  T get() const { return value; }
};
template <> struct Box<int> {
  int value;
  int get() const { return value + 1; }
};
