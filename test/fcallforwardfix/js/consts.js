// [B1b] ESM: an exported CONST object (notAFn) beside an exported function (realFn, the [P] twin).
export const notAFn = { call () { return 0 } }
export function realFn () { return 9 }
