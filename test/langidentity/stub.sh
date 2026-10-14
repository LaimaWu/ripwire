#!/usr/bin/env bash
# stub.sh VARIANT ARGV... -- a stand-in ripwire for selftest.sh. The harness sees only bytes and the exit code, so a
# stub whose output is a fixed function of its argv tests it the way a rebuilt binary would, in milliseconds.
#   base    the reference behaviour; every notice ends with a provenance tag "(FILE:LINE, SIGNATURE — logged ...)"
#   moved   the same behaviour; only the tag's line number and signature and the TMPDIR differ (must read SAME)
#   stderr  base plus one extra stderr line (must read DIFF)
#   msg     base with the notice text changed, tag untouched (must read DIFF)
#   stdout  base with one stdout byte changed on every --callers= argv (must read DIFF)
#   rc      base with the same output but exit code 1 on every argv but the --json map (must read ERROR)
v="$1"; shift
case " $* " in
    *" --json "*)
        printf '%s\n' '{"r":[{"p":"test/gdscriptfix/stub.gd","s":[{"n":"<file-scope>","t":"modscope","k":9},{"n":"alpha","t":"fn","k":5},{"n":"beta","t":"fn","k":4}]}]}'
        exit 0 ;;
    *" --regex="*) printf '<grep schema="stub"><f p="src/stub.h"></f></grep>\n'; exit 0 ;;
esac
line=100 sig='void f(int)' msg='the stub walk could not read every tree' mark='o'
case "$v" in
    moved) line=250 sig='int g(const std::string &)' ;;
    msg) msg='the stub walk could not read some trees' ;;
esac
case "$v:$*" in stdout:*--callers=*) mark='0' ;; esac
case " $* " in
    *" --dead-code "*) printf '<dead-code schema="stub" count="0"></dead-code>\n' ;;
    *) printf '<%sut schema="stub" argv="%s"><row/></out>\n' "$mark" "$*" ;;
esac
printf '[math degraded] %s  (crossref.h:%d, %s — logged once per site)\n' "$msg" "$line" "$sig" >&2
printf 'scratch: %s/stub.tmp\n' "${TMPDIR:-/tmp}" >&2
[ "$v" = stderr ] && printf 'warning: injected stderr line\n' >&2
[ "$v" = rc ] && exit 1
exit 0
