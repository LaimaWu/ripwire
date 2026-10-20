import { notAFn, realFn } from './consts.js'

// [B1b] `.call` on an ESM named import of a const object: no forwarded call; [P] the same shape on a function: an edge
export function esmUser () {
  notAFn.call(null)
  return realFn.call(null)
}
