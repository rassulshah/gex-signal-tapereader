#!/usr/bin/env python3
"""splice-seed.py — copy a seed JSON file into its `var NAME=…;` line in the userscript.

The panel renders the plan / the learning doc / the registry before its first fetch from a baked-in copy
(PLAN_SEED, LEARN_SEED, STUDIES_SEED). test_v1559 / test_v1562 pin each copy equal to its file, byte for byte
under JSON.stringify. This is the one way to update them — never by hand.

  python3 tools/splice-seed.py PLAN_SEED learning/plan.json
  python3 tools/splice-seed.py LEARN_SEED learning/deflections/examples.json
"""
import io, json, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)
SCRIPT = 'current/gex-signal-tapereader.user.js'

def main():
    name, path = sys.argv[1], sys.argv[2]
    data = json.load(io.open(path, encoding='utf-8'))
    js = json.dumps(data, ensure_ascii=False, separators=(',', ':'))
    src = io.open(SCRIPT, encoding='utf-8').read()
    m = re.search(r'^var ' + re.escape(name) + r'=.*?;\s*$', src, re.M)
    if not m:
        raise SystemExit('no single-line `var %s=…;` in the script' % name)
    out = src[:m.start()] + 'var ' + name + '=' + js + ';' + src[m.end():]
    io.open(SCRIPT, 'w', encoding='utf-8').write(out)
    print('spliced %s from %s (%d bytes)' % (name, path, len(js)))

if __name__ == '__main__':
    main()
