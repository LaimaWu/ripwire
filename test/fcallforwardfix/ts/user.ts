import { target as tgt } from './lib';
import defaultFn from './lib2';
import * as ns from './lib';
import type { TypeOnly } from './lib';
import { type typeOnlyFn } from './lib';

// [T1] `.call` on an aliased named import; [T2] `.apply` on a default import
export function tsUser(): number {
  const a = tgt.call(null, 1);
  const b = defaultFn.apply(null, []);
  return a + b;
}

// [T3] a module namespace member, never the bare name
export function tsNs(): number {
  return ns.target.call(null, 2);
}

// [T4] a typed parameter shadows the namesake in other.ts
export function tsParam(handler: () => number, t: TypeOnly): number {
  return handler.call(null) + t.n;
}

// [T5] `.bind` stored in a local: a value, not a call
export function tsBind(): () => number {
  const bound = defaultFn.bind(null);
  return bound;
}

// [T6] a local arrow function: not forwarded (disclosed floor)
export function tsLocalArrow(): number {
  const run = (): number => 5;
  return run.call(null);
}

// [M4] an inline `type` specifier binds no value: no forwarded call
export function tsTypeSpec(): number {
  return typeOnlyFn.call(null);
}
