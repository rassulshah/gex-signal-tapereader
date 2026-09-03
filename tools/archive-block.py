#!/usr/bin/env python3
"""
ARCHIVE A BLOCK — move a top-level function (or var) out of the userscript, verbatim, into archive/.

⚠⚠ THE RULE: A BLOCK IS REMOVED ONLY IF NOTHING ELSE REFERENCES IT. After the candidate's own text
is cut, the whole remaining file is grepped for the bare name (word-boundary). One hit = a live caller
the audit missed = the removal is ROLLED BACK and reported. Nothing is ever deleted; the source goes
to archive/<ver>/<group>/<name>.js with a header naming the reason and every test that pinned it.

Dead chains are handled by ITERATING TO A FIXED POINT: a callee still referenced by a dead caller
fails on pass 1 and succeeds on pass 2 once the caller is gone. Order in the spec does not matter.

USAGE
    python3 tools/archive-block.py spec.json            # apply
    python3 tools/archive-block.py spec.json --dry      # report only

spec.json:
    { "version": "v15.53",
      "groups": { "A-dead-else": { "reason": "…", "fn": ["readBlock44", …], "var": ["LAST_OK", …] } } }
"""
import io, json, os, re, sys, glob

SRC = 'current/gex-signal-tapereader.user.js'

def find_fn(src, name):
    m = re.search(r'\n(function\s+' + re.escape(name) + r'\s*\()', src)
    if not m: return None
    start = m.start() + 1
    # include contiguous // comment lines immediately above
    lead = start
    while True:
        prev_nl = src.rfind('\n', 0, lead - 1)
        line = src[prev_nl + 1:lead - 1]
        if line.strip().startswith('//'): lead = prev_nl + 1
        else: break
    i = src.index('{', m.end() - 1); d = 0
    for k in range(i, len(src)):
        c = src[k]
        if c == '{': d += 1
        elif c == '}':
            d -= 1
            if d == 0:
                end = k + 1
                # swallow a trailing ';' and the newline
                while end < len(src) and src[end] in ';': end += 1
                if end < len(src) and src[end] == '\n': end += 1
                return (lead, end)
    return None

def find_var(src, name):
    m = re.search(r'\n(var\s+' + re.escape(name) + r'\s*=)', src)
    if not m: return None
    a = m.start() + 1
    # a var statement may span lines ('…'+\n '…'); walk to the first ';' at bracket depth 0 outside quotes
    i = m.end(); d = 0; q = None
    while i < len(src):
        c = src[i]
        if q:
            if c == '\\': i += 2; continue
            if c == q: q = None
        elif c in '\'"': q = c
        elif c in '([{': d += 1
        elif c in ')]}': d -= 1
        elif c == ';' and d == 0:
            e = i + 1
            if e < len(src) and src[e] == '\n': e += 1
            return (a, e)
        i += 1
    return None

def code_only(src):
    # drop whole-line comments and trailing " // …" comments; a URL's "://" survives (no space before it)
    out = []
    for line in src.split('\n'):
        st = line.lstrip()
        if st.startswith('//'): continue
        out.append(re.sub(r'(\s|;)//.*$', '', line))
    return '\n'.join(out)

def refs(src, name):
    return len(re.findall(r'(?<![A-Za-z0-9_$.\-])' + re.escape(name) + r'(?![A-Za-z0-9_$])', code_only(src)))

def pinning_tests(name):
    out = []
    for t in sorted(glob.glob('test_*.js')):
        try:
            if re.search(r'(?<![A-Za-z0-9_$])' + re.escape(name) + r'(?![A-Za-z0-9_$])', io.open(t, encoding='utf-8').read()):
                out.append(t)
        except Exception: pass
    return out

def main():
    spec = json.load(open(sys.argv[1])); dry = '--dry' in sys.argv
    ver = spec['version']
    src = io.open(SRC, encoding='utf-8').read()
    pending = []
    for g, G in spec['groups'].items():
        for n in G.get('fn', []): pending.append((g, 'fn', n, G['reason']))
        for n in G.get('var', []): pending.append((g, 'var', n, G['reason']))
    done, index = [], []
    passno = 0
    while pending and passno < 12:
        passno += 1; progressed = False; still = []
        for (g, kind, name, reason) in pending:
            rng = find_fn(src, name) if kind == 'fn' else find_var(src, name)
            if not rng:
                print('  MISSING  %-28s (not found as top-level %s)' % (name, kind)); continue
            a, b = rng; body = src[a:b]; trial = src[:a] + src[b:]
            r = refs(trial, name)
            if r > 0:
                still.append((g, kind, name, reason)); continue
            # commit
            progressed = True
            line_from = src.count('\n', 0, a) + 1
            tests = pinning_tests(name)
            src = trial          # simulate the chain in dry mode too; only the writes are skipped
            if not dry:
                d = os.path.join('archive', ver, g); os.makedirs(d, exist_ok=True)
                hdr = ('// ARCHIVED %s from current/gex-signal-tapereader.user.js line %d (group %s)\n'
                       '// WHY: %s\n'
                       '// PINNED BY: %s\n'
                       '// This is the verbatim source. Restore by pasting it back and re-running the pinning tests.\n\n'
                       % (ver, line_from, g, reason, ', '.join(tests) if tests else 'nothing'))
                io.open(os.path.join(d, name + '.js'), 'w', encoding='utf-8').write(hdr + body)
            lines = body.count('\n')
            done.append((g, name, lines, tests))
            print('  archived %-28s %5d lines  pass %d  %s' % (name, lines, passno, ('pinned: ' + ' '.join(tests)) if tests else ''))
        pending = still
        if not progressed: break
    for (g, kind, name, reason) in pending:
        print('  REFUSED  %-28s still referenced %d time(s) — a LIVE caller, not archived' % (name, refs(src, name)))
    if not dry:
        io.open(SRC, 'w', encoding='utf-8').write(src)
        idx = os.path.join('archive', ver, 'INDEX.md')
        with io.open(idx, 'a', encoding='utf-8') as f:
            for (g, name, lines, tests) in done:
                f.write('| %s | `%s` | %d | %s |\n' % (g, name, lines, ', '.join(tests) if tests else '—'))
    total = sum(x[2] for x in done)
    print('\n%s %d blocks, %d lines; %d refused' % ('would archive' if dry else 'ARCHIVED', len(done), total, len(pending)))

if __name__ == '__main__': main()
