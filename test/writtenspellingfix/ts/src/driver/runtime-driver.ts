export class RuntimeDriver {
  async beginTransaction(x: number): Promise<number> { return tsBeginHelper(x) }
  commitTransaction(x: number): number { return tsCommitHelper(x) }
}
export const AliasDriver = RuntimeDriver
function tsBeginHelper(x: number): number { return x }
function tsCommitHelper(x: number): number { return x }
