#!/usr/bin/env python3
"""runner.py -- the engine of test/langidentitycheck.sh (read that file's header for the contract).

Runs one fixed argv set per corpus with a BASE and a HEAD ripwire binary, compares stdout bytes and exit
code, and classifies every argv SAME | DIFF | EMPTY | ERROR. Never spawns a shell: every argv is a list.

usage: runner.py [--base BIN] [--ratchet-only] --head BIN --root REPO --scratch DIR --manifest FILE --ratchet FILE
                 [--langs a,b] [--syms FILE] [--intended FILE] [--repo LANG:DIR]... [--jobs N]
                 [--timeout S] [--keep DIR] [--list]
(--ratchet-only runs just the ratchet line against --head, which needs no --base.)
Exit 0 = every argv SAME (or DIFF with an --intended row), the self-test fired, the ratchet holds.
"""
import argparse
import fnmatch
import json
import os
import re
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

# The verbs that need no symbol (run once per corpus), and those that take one (run per symbol).
CORPUS_VERBS = [[], ['--deps', '--limit=100000'], ['--skipped'], ['--clones', '--limit=100000'], ['--dead-code'],
                ['--hotspots'], ['--tree'], ['--nonlocal-state']]
SYMBOL_VERBS = ['--callers', '--callees', '--impact', '--uses', '--whereis', '--expand']
TOP_PER_LANG = 5

# Map rows are grouped by file with no language attribute, so the "top 5 per language" filter is by extension.
EXTS = {
    'cpp': ('.cpp', '.cc', '.cxx', '.hpp', '.hh', '.hxx', '.h'), 'c': ('.c', '.h'), 'py': ('.py', '.pyi'),
    'ts': ('.ts', '.tsx'), 'js': ('.js', '.jsx', '.mjs', '.cjs'), 'go': ('.go',), 'rs': ('.rs',),
    'swift': ('.swift',), 'objc': ('.m', '.mm', '.h'), 'sh': ('.sh', '.bash'), 'java': ('.java',),
    'rb': ('.rb',), 'cs': ('.cs',), 'php': ('.php',), 'lua': ('.lua',), 'ex': ('.ex', '.exs'),
    'dart': ('.dart',), 'kt': ('.kt', '.kts'), 'gd': ('.gd',), 'clj': ('.clj', '.cljs', '.cljc'),
}


def run(binary, argv, cwd, tmpdir, timeout):
    """-> (rc, stdout, stderr, wall_seconds, error_text). error_text is None when the process ran to exit."""
    env = dict(os.environ, TMPDIR=tmpdir)
    t0 = time.monotonic()
    try:
        r = subprocess.run([binary] + argv, cwd=cwd, env=env, stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE, timeout=timeout)
    except subprocess.TimeoutExpired:
        return None, b'', b'', time.monotonic() - t0, 'timeout after %ss' % timeout
    except OSError as e:
        return None, b'', b'', time.monotonic() - t0, 'cannot start: %s' % e
    return r.returncode, r.stdout, r.stderr, time.monotonic() - t0, None


def classify(b, h):
    """b, h = run() tuples -> (status, detail). ERROR beats EMPTY beats DIFF beats SAME."""
    if b[4] or h[4]:
        return 'ERROR', 'base: %s; head: %s' % (b[4] or 'ran', h[4] or 'ran')
    if b[0] != 0 or h[0] != 0:
        return 'ERROR', 'rc base=%s head=%s' % (b[0], h[0])
    if not b[1] or not h[1]:
        return 'EMPTY', 'stdout bytes base=%d head=%d (a vacuous comparison)' % (len(b[1]), len(h[1]))
    if b[1] != h[1]:
        n = 0
        lim = min(len(b[1]), len(h[1]))
        while n < lim and b[1][n] == h[1][n]:
            n += 1
        line = b[1].count(b'\n', 0, n) + 1
        return 'DIFF', 'bytes base=%d head=%d, first difference at byte %d (line %d)' % (len(b[1]), len(h[1]), n, line)
    return 'SAME', ''


def materialize(root, dirs, dest):
    """Copy each fixture directory (same relative path) into a fresh git repository with one commit at a fixed date.

    A fixture run inside the host checkout pays for that checkout's whole history and every branch it carries
    (--hotspots, --whereis), and its answer would move with that state. In its own repository the answer depends on
    the fixture alone, and the path (test/xfix) still classifies it the way the real tree does."""
    os.makedirs(dest, exist_ok=True)
    for d in dirs:
        target = os.path.join(dest, d)
        os.makedirs(os.path.dirname(target), exist_ok=True)
        if subprocess.run(['cp', '-R', os.path.join(root, d), target], stdout=subprocess.DEVNULL).returncode != 0:
            return None
    env = dict(os.environ, GIT_AUTHOR_DATE='2026-01-01T00:00:00Z', GIT_COMMITTER_DATE='2026-01-01T00:00:00Z',
               GIT_CONFIG_GLOBAL=os.devnull, GIT_CONFIG_SYSTEM=os.devnull)
    for cmd in (['git', 'init', '-q'], ['git', 'add', '-A', '-f'],
                ['git', '-c', 'user.name=fixture', '-c', 'user.email=fixture@example.invalid', 'commit', '-q', '-m', 'fixtures']):
        try:
            if subprocess.run(cmd, cwd=dest, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode != 0:
                return None
        except OSError:
            return None
    return dest


def read_tsv(path):
    rows = []
    with open(path, encoding='utf-8', errors='replace') as fh:
        for n, raw in enumerate(fh, 1):
            s = raw.rstrip('\n')
            if not s.strip() or s.lstrip().startswith('#'):
                continue
            rows.append((n, s.split('\t')))
    return rows


def symbols_of(binary, root, cwd, tmpdir, timeout):
    """Every symbol of a corpus, ranked: [(file, name, kind, rank)]; None when the map could not be read."""
    rc, out, _e, _w, err = run(binary, [root, '--top-k=100000', '--json', '--no-cache'], cwd, tmpdir, timeout)
    if err or rc != 0:
        return None
    try:
        doc = json.loads(out.decode('utf-8', 'replace'))
    except ValueError:
        return None
    syms = []
    for row in doc.get('r', []):
        for s in row.get('s', []):
            syms.append((row.get('p', ''), s.get('n', ''), s.get('t', ''), float(s.get('k', 0))))
    return syms


def top_names(syms, lang, count):
    """The `count` highest-ranked distinct names whose file extension belongs to `lang`."""
    exts = EXTS.get(lang)
    seen, picked = set(), []
    for p, n, _t, _k in sorted(syms, key=lambda s: -s[3]):
        if not n or n in seen or (exts and not p.lower().endswith(exts)):
            continue
        seen.add(n)
        picked.append(n)
        if len(picked) == count:
            break
    return picked


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--base', default='')
    ap.add_argument('--head', required=True)
    ap.add_argument('--root', required=True)
    ap.add_argument('--scratch', required=True)
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--ratchet', required=True)
    ap.add_argument('--langs', default='')
    ap.add_argument('--syms', default='')
    ap.add_argument('--intended', default='')
    ap.add_argument('--repo', action='append', default=[])
    ap.add_argument('--jobs', type=int, default=1)
    ap.add_argument('--timeout', type=int, default=600)
    ap.add_argument('--keep', default='')
    ap.add_argument('--list', action='store_true')
    ap.add_argument('--ratchet-only', action='store_true')
    a = ap.parse_args()
    fails = []

    tmp = {}
    for label in ('base', 'head'):
        tmp[label] = os.path.join(a.scratch, 'tmp-' + label)
        os.makedirs(tmp[label], exist_ok=True)
    binary = {'base': a.base, 'head': a.head}
    if a.ratchet_only:
        return 1 if ratchet(a, a.head, tmp['head']) else 0
    want = [x for x in a.langs.split(',') if x]

    # -- corpora: manifest fixtures, then caller-supplied repos --
    corpora = []
    for _n, cols in read_tsv(a.manifest):
        if len(cols) < 2 or cols[1] == '-':
            continue
        if want and cols[0] not in want:
            continue
        corpora.append({'id': '%s/%s' % (cols[0], os.path.basename(cols[1].rstrip('/'))), 'lang': cols[0],
                        'root': cols[1], 'kind': 'fixture', 'cwd': ''})
    for spec in a.repo:
        lang, _sep, path = spec.partition(':')
        if not path or not os.path.isdir(path):
            print('  FAIL  --repo=%s: expected LANG:DIR with an existing directory' % spec)
            return 1
        if want and lang not in want:
            continue
        corpora.append({'id': '%s/%s' % (lang, os.path.basename(path.rstrip('/'))), 'lang': lang,
                        'root': os.path.abspath(path), 'kind': 'repo', 'cwd': a.root})
    if not corpora:
        print('  FAIL  no corpus selected (--langs=%s matched no manifest row and no --repo)' % a.langs)
        return 1
    if a.list:
        for c in corpora:
            print('%s\t%s\t%s' % (c['id'], c['kind'], c['root']))
        return 0

    fixtures = [c for c in corpora if c['kind'] == 'fixture']
    if fixtures:
        fixrepo = materialize(a.root, sorted({c['root'] for c in fixtures}), os.path.join(a.scratch, 'fixrepo'))
        if not fixrepo:
            print('  FAIL  could not copy the fixture directories into a scratch git repository (is git installed?)')
            return 1
        for c in fixtures:
            c['cwd'] = fixrepo

    # -- touched symbols: NAME or LANG:NAME per line --
    touched = []
    if a.syms:
        for _n, cols in read_tsv(a.syms):
            word = cols[0].strip()
            lang, sep, name = word.partition(':')
            touched.append((lang, name) if sep and lang in EXTS else ('', word))

    # -- intended DIFF rows: CORPUS-GLOB <TAB> ARGV-GLOB <TAB> ARM --
    intended, bad_intended = [], False
    if a.intended:
        for n, cols in read_tsv(a.intended):
            if len(cols) < 3 or not all(c.strip() for c in cols[:3]):
                print('  FAIL  %s line %d: an intended row is CORPUS<TAB>ARGV<TAB>ARM, all three non-empty' % (a.intended, n))
                bad_intended = True
                continue
            intended.append({'corpus': cols[0].strip(), 'argv': cols[1].strip(), 'arm': cols[2].strip(), 'hits': 0})
    if bad_intended:
        return 1

    counts = {'SAME': 0, 'DIFF': 0, 'EMPTY': 0, 'ERROR': 0, 'intended': 0}
    wall = {'base': 0.0, 'head': 0.0}
    biggest = (0, None)  # (symbol count, corpus) for the self-test

    def one(c, verb):
        cwd = c['cwd']
        argv = [c['root']] + verb + ['--no-cache']
        # BASE and HEAD overlap: the two answers are independent, and a run is mostly process start and git time
        with ThreadPoolExecutor(max_workers=2) as pair:
            fb = pair.submit(run, binary['base'], argv, cwd, tmp['base'], a.timeout)
            fh = pair.submit(run, binary['head'], argv, cwd, tmp['head'], a.timeout)
            return verb, fb.result(), fh.result()

    for c in corpora:
        base_syms = symbols_of(binary['base'], c['root'], c['cwd'], tmp['base'], a.timeout)
        head_syms = symbols_of(binary['head'], c['root'], c['cwd'], tmp['head'], a.timeout)
        if base_syms is None or head_syms is None:
            print('corpus %s (%s)' % (c['id'], c['root']))
            print('  ERROR  (symbol list)  the %s map could not be read as JSON' % ('base' if base_syms is None else 'head'))
            counts['ERROR'] += 1
            fails.append('%s: symbol list unreadable' % c['id'])
            continue
        if len(base_syms) > biggest[0]:
            biggest = (len(base_syms), c)
        names = top_names(base_syms, c['lang'], TOP_PER_LANG)
        known = {s[1] for s in base_syms} | {s[1] for s in head_syms}
        extra, absent = [], 0
        for lang, name in touched:
            if lang and lang != c['lang']:
                continue
            if name in known and name not in names and name not in extra:
                extra.append(name)
            elif name not in known:
                absent += 1
        verbs = list(CORPUS_VERBS)
        for name in names + extra:
            verbs += [[v + '=' + name] for v in SYMBOL_VERBS]
        print('corpus %s (%s): %d symbols, %d top + %d touched named, %d argv%s' % (
            c['id'], c['root'], len(base_syms), len(names), len(extra), len(verbs),
            ('; %d touched name(s) absent from both maps' % absent) if absent else ''))
        with ThreadPoolExecutor(max_workers=max(1, a.jobs)) as ex:
            results = list(ex.map(lambda v: one(c, v), verbs))
        for verb, b, h in results:
            status, detail = classify(b, h)
            text = ' '.join(verb) if verb else '(map)'
            wall['base'] += b[3]
            wall['head'] += h[3]
            note = ''
            if status == 'DIFF':
                for row in intended:
                    if fnmatch.fnmatchcase(c['id'], row['corpus']) and fnmatch.fnmatchcase(text, row['argv']):
                        row['hits'] += 1
                        note = '  [intended: %s]' % row['arm']
                        break
                if note:
                    counts['intended'] += 1
                else:
                    fails.append('%s %s: unexplained DIFF' % (c['id'], text))
            elif status != 'SAME':
                fails.append('%s %s: %s' % (c['id'], text, status))
            counts[status] += 1
            print('  %-5s %-36s base=%.2fs head=%.2fs%s%s' % (status, text, b[3], h[3],
                                                           ('  ' + detail) if detail else '', note))
            if status != 'SAME' and a.keep:
                d = os.path.join(a.keep, c['id'].replace('/', '_'), re.sub(r'[^A-Za-z0-9._=-]', '_', text))
                os.makedirs(d, exist_ok=True)
                for lab, r in (('base', b), ('head', h)):
                    with open(os.path.join(d, lab + '.out'), 'wb') as fh:
                        fh.write(r[1])
                    with open(os.path.join(d, lab + '.err'), 'wb') as fh:
                        fh.write(r[2])

    total = counts['SAME'] + counts['DIFF'] + counts['EMPTY'] + counts['ERROR']
    for row in intended:
        if not row['hits']:
            print('  NOTE  intended row matched no DIFF (stale, or the corpus was not selected): %s | %s | %s' % (
                row['corpus'], row['argv'], row['arm']))
    summary = '%d argv over %d corpora: %d SAME, %d DIFF (%d intended), %d EMPTY, %d ERROR; wall base=%.1fs head=%.1fs' % (
        total, len(corpora), counts['SAME'], counts['DIFF'], counts['intended'], counts['EMPTY'], counts['ERROR'],
        wall['base'], wall['head'])
    if fails:
        print('  FAIL  ' + summary)
        for f in fails[:30]:
            print('        - ' + f)
        if len(fails) > 30:
            print('        ... %d more' % (len(fails) - 30))
    else:
        print('  PASS  ' + summary)

    # -- self-test: an arm that cannot fail proves nothing. HEAD with a smaller --top-k MUST read as DIFF --
    st_fail = False
    n_syms, corpus = biggest
    if corpus is None or n_syms < 2:
        print('  FAIL  self-test cannot fire: no selected corpus has a map of at least 2 symbols')
        st_fail = True
    else:
        k = 199 if n_syms > 199 else n_syms - 1
        argv = [corpus['root'], '--no-cache']
        b = run(binary['base'], argv, corpus['cwd'], tmp['base'], a.timeout)
        h = run(binary['head'], argv + ['--top-k=%d' % k], corpus['cwd'], tmp['head'], a.timeout)
        status, detail = classify(b, h)
        if status == 'DIFF':
            print('  PASS  self-test: head with --top-k=%d on %s reads as DIFF (%s)' % (k, corpus['id'], detail))
        else:
            print('  FAIL  self-test: head with --top-k=%d on %s read as %s, so a real change could read as SAME' % (
                k, corpus['id'], status))
            st_fail = True

    # -- ratchet: shared-file `== Lang::X` predicates may not rise (printed last) --
    ratchet_fail = ratchet(a, binary['head'], tmp['head'])
    return 1 if (fails or st_fail or ratchet_fail) else 0


def ratchet(a, head, tmpdir):
    """Count `==`/`!=` predicates against a Lang:: enumerator in src/ files that are not per-language files."""
    ceiling, perlang = None, set()
    for _n, cols in read_tsv(a.ratchet):
        if cols[0] == 'ceiling' and len(cols) > 1 and cols[1].isdigit():
            ceiling = int(cols[1])
        elif cols[0] == 'perlang' and len(cols) > 1:
            perlang.add(cols[1])
    if ceiling is None:
        print('  FAIL  ratchet: %s has no "ceiling<TAB>N" row' % a.ratchet)
        return True
    rc, out, _e, _w, err = run(head, [os.path.join(a.root, 'src'), '--regex=Lang::[A-Z][A-Za-z]+', '--limit=100000',
                                      '--no-cache'], a.root, tmpdir, a.timeout)
    if err or rc != 0 or not out:
        print('  FAIL  ratchet: the HEAD binary could not list Lang:: mentions (%s, rc=%s)' % (err or 'ran', rc))
        return True
    text = out.decode('utf-8', 'replace')
    seen, count = set(), 0
    pred = re.compile(r'[=!]=\s*(?<![A-Za-z0-9_])Lang::|(?<![A-Za-z0-9_])Lang::\w+\s*[=!]=')
    for path, body in re.findall(r'<f p="([^"]+)"[^>]*>(.*?)</f>', text, re.S):
        if os.path.basename(path) in perlang:
            continue
        for line, code in re.findall(r'<hit l="(\d+)" in="[^"]*"><!\[CDATA\[(.*?)\]\]></hit>', body, re.S):
            if (path, line) in seen:
                continue
            seen.add((path, line))
            if re.search(r'\bcase\s+Lang::', code):
                continue
            if pred.search(code):
                count += 1
    if count > ceiling:
        print('  FAIL  ratchet: shared-file "== Lang::" predicates %d > ceiling %d (move the new one into a LangFacts column or a per-language file)' % (count, ceiling))
        return True
    print('  PASS  ratchet: shared-file "== Lang::" predicates %d (ceiling %d)%s' % (
        count, ceiling, '' if count == ceiling else ' -- lower the ceiling in %s to %d' % (os.path.basename(a.ratchet), count)))
    return False


if __name__ == '__main__':
    sys.stdout.reconfigure(line_buffering=True)  # a long run must show progress through a pipe
    sys.exit(main())
