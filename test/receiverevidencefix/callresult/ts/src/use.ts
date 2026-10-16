import { Config, makeConfig, makeUntyped, loadConfig } from "./model";

// useConst: `const x = f()` with an annotated return type.
export function useConst(): number {
  const c = makeConfig();
  return c.get();
}

// useAwait: `const x = await f()` through Promise<T>.
export async function useAwait(): Promise<number> {
  const c = await loadConfig();
  return c.get();
}

// nearUntyped: no written return type (TS inference is not modelled).
export function nearUntyped(): number {
  const c = makeUntyped();
  return c.get();
}

// nearMap: an outside callee whose result has a same-named member.
export function nearMap(): number | undefined {
  const m = Object.freeze(new Map<string, number>());
  return m.get("a");
}

// useWritten (twin): the same call with the type written.
export function useWritten(): number {
  const c: Config = makeConfig();
  return c.get();
}
