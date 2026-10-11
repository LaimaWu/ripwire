class JRoot {
  run() {}
}
class JNode extends JRoot {
  static extend(name, body) { return class extends this {}; }
}
module.exports = { JRoot, JNode };
