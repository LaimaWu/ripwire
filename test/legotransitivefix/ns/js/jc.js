const { JNode } = require('./ja');
class JTrue2 extends JNode {}
const JTrue = JNode.extend('JTrue');                      // the receiver is imported: the parser cannot show it is a class
const JBody = JNode.extend('JBody', { render() { return 1; } });   // a class BODY: the parser shows the class
module.exports = { JTrue2, JTrue, JBody };
