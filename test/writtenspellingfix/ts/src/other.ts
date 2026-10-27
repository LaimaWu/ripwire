export class Other {
  beginTransaction(): number { return tsOtherHelper() }
}
function tsOtherHelper(): number { return 1 }
