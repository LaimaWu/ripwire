// impact depth 2: reaches EvalString::empty only, through UsesEval
#include "eval_env.h"
bool UsesEval(EvalString ev);
bool RunEval() {
  EvalString ev{0};
  return UsesEval(ev);
}
