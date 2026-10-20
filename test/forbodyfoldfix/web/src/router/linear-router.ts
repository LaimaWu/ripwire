import { UnsupportedPathError } from './errors'
import type { Result, Router } from './types'

export class LinearRouter<T> implements Router<T> {
  name: string = 'LinearRouter'
  #routes: [string, string, T][] = []

  add(method: string, path: string, handler: T) {
    this.#routes.push([method, path, handler])
  }

  match(method: string, path: string): Result<T> {
    const out: Result<T> = []
    for (const [m, p, h] of this.#routes) {
      if (p.includes('{')) {
        throw new UnsupportedPathError()
      }
      if (m === method && p === path) {
        out.push([h, {}])
      }
    }
    return out
  }
}
