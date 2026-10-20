// lang-facts pins (train 29 langfacts-fold): the TypeScript overload-signature rule and its near misses.
export class Box {
    stash(a: string): string;
    stash(a: string, b: number): string;
    stash(a: string, b?: number): string { return b === undefined ? a : a + b; }
}
export abstract class Shape { abstract measure(): number; }
export class Sq extends Shape { measure(): number { return 4; } }
declare class Amb { ring(): void; }
export class Bell { ring(): void { return; } }
export function useBox(b: Box, s: Shape, a: Amb, l: Bell): number { return b.stash("x").length + s.measure() + (a.ring(), l.ring(), 1); }
