// lang-facts pins: a Java interface method is a contract, a definition of its own, never an overload signature.
interface Doer { void doIt(); }
class Doing implements Doer { public void doIt() { } }
class UseDoer { void run( Doer d ) { d.doIt(); } }
