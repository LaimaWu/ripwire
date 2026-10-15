# softbudget.py — the soft --token-budget readers' shared helpers (lane forbudget-soft; CHECKLIST 32: one copy, not five).
#
# Five gates read a soft-target answer's ST invariant (fornotesbudgetcheck, forsoftbudgetcheck, compactlegendcheck,
# estchargecheck, forsigspancheck). Each keeps its own row reader (their dialects and row charges differ); what they share
# lives here. The gates export RIPWIRE_TESTLIB="$ROOT/test/lib" and import this file (sys.path insert), as crashsweepcheck
# does shardmatch.py. No state; pure functions of the answer text.
import json, re


def tail_size( t ):
    """Bytes of the answer's tail: XML, the whole <tail>…</tail> element; JSON, the compact "tail" object.

    FIX1 R1: the refill reserves the tail at its HEAD-scoped size (the tail of the no-budget answer) when that is what binds,
    so maximality counts the tail at no less than the reference tail (an upper bound on the reserve: the check stays a bound).
    """
    if t.lstrip().startswith( "{" ):
        try: return len( json.dumps( json.loads( t ).get( "tail", {} ), separators=( ",", ":" ), ensure_ascii=False ).encode() )
        except ValueError: return 0
    m = re.search( r"<tail\b.*?</tail>", t, re.S )
    return len( m.group( 0 ).encode() ) if m else 0
