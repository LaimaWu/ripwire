#pragma once
struct EvalString {
  int parsed_;
  void Clear() { parsed_ = 0; }
  bool empty() const { return parsed_ == 0; }
};
