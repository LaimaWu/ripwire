import { SmartRouter } from './router/smart-router'
import { LinearRouter } from './router/linear-router'

export class App {
  router: SmartRouter<string>

  // the app picks its router: a SmartRouter over the candidate routers
  constructor() {
    this.router = new SmartRouter({ routers: [new LinearRouter()] })
  }

  route(method: string, path: string, handler: string) {
    this.router.add(method, path, handler)
  }
}
