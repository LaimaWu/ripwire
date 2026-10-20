import { UnsupportedPathError } from './errors'
import type { Result, Router } from './types'

export class SmartRouter<T> implements Router<T> {
  name: string = 'SmartRouter'
  #routers: Router<T>[] = []
  #routes?: [string, string, T][] = []

  constructor(init: { routers: Router<T>[] }) {
    this.#routers = init.routers
  }

  add(method: string, path: string, handler: T) {
    if (!this.#routes) {
      throw new Error('router already built')
    }
    this.#routes.push([method, path, handler])
  }

  match(method: string, path: string): Result<T> {
    const routers = this.#routers
    const routes = this.#routes ?? []
    let res
    for (let i = 0; i < routers.length; i++) {
      const router = routers[i]
      try {
        for (const r of routes) {
          router.add(...r)
        }
        res = router.match(method, path)
      } catch (e) {
        if (e instanceof UnsupportedPathError) {
          continue
        }
        throw e
      }
      this.match = router.match.bind(router)
      this.#routers = [router]
      this.#routes = undefined
      break
    }
    if (!res) {
      throw new Error('no router accepted the routes')
    }
    this.name = `SmartRouter + ${this.activeRouter.name}`
    return res
  }

  get activeRouter(): Router<T> {
    if (this.#routes || this.#routers.length !== 1) {
      throw new Error('no active router until the first match')
    }
    return this.#routers[0]
  }
}
