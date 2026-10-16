import { Router } from "../router";

export class RegExpRouter implements Router {
  match(method: string, path: string): string[] {
    return [path, method];
  }
}
