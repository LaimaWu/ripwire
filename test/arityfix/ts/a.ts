interface Foo { n: number }
export function t(a: number, b?: number): number { return a + (b ?? 0); }
export function d(a: number, b = 2): number { return a + b; }
export function w(this: Foo, a: number): number { return this.n + a; }
