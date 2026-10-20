interface SetH { (name: string, value?: string, options?: object): void }
type Fn = (a: string, b?: string) => void;
export class Ctx {
  header: SetH = (name, value, options): void => {}
}
export const f: Fn = (a: string, b: string): void => {};
export const g = ((a: string, b: string): void => {}) as Fn;
export const u = (a: string, b: string): void => {};
