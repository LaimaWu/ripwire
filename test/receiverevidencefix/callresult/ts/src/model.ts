export class Config {
  get(): number { return 0; }
}

export class Other {
  get(): number { return 1; }
}

export function makeConfig(): Config { return new Config(); }

export function makeUntyped() { return new Config(); }

export async function loadConfig(): Promise<Config> { return new Config(); }

export function freeze(): Config { return new Config(); }
