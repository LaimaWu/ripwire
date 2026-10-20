import { Router } from "../routers/router";

function clean(path: string): string {
  return path.trim();
}

export function dispatch(router: Router, method: string, path: string): string[] {
  return router.match(method, clean(path));
}
