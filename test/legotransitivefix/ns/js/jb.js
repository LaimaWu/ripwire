// Unrelated namesake JNode: does NOT extend JRoot.
class JNode {
  static extend(name) { return class extends this {}; }
}
const JLeaf = JNode.extend('JLeaf');
class JLeaf2 extends JNode {}
module.exports = { JNode, JLeaf, JLeaf2 };
