import { Router } from "../router";

export class TrieRouter implements Router {
  match(method: string, path: string): string[] {
    return [method, path];
  }
}
