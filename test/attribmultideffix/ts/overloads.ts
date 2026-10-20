// overload signatures of DIFFERING arity before one implementation: one function, one definition group
export function pick(a: string): string;
export function pick(a: string, b: number): string;
export function pick(a: string, b?: number): string { return b === undefined ? a : a + b; }
export function usePick(): string { return pick("x") + pick("y", 2); }
