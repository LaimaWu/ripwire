export interface Router {
  match(method: string, path: string): string[];
}
