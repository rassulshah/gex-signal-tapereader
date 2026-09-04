#!/usr/bin/env python3
"""recover-archive.py — rebuild archive/v15.53/ from git history.

WHY THIS EXISTS (2026-09-04). The v15.53 simplification archived ~4,300 lines into `archive/v15.53/` — every
retired block verbatim, grouped by reason, with `INDEX.md` and 26 retired tests under `tests/`. That directory
was committed in a cloud sandbox and NEVER reached GitHub: the installer's manifest had no `archive/` (nor
`roadmap/`), and the cloud cannot push. A fresh clone therefore failed `test_v1553.js` (no INDEX.md) and still
carried the 25 retired tests at the root, red — the SEVENTH directory the manifest lost (landmine L-Q).

The retired CODE was never lost: it is in git at the last pre-simplification release (v15.51, commit
bbd2f1a). This script diffs the top-level function/var names of that script against the live one and writes
every retired block into `archive/v15.53/<group>/<name>.js`, with `INDEX.md` listing all of them. The grouping
(A–L) is reconstructed from the v15.53 CHANGELOG entry where a name is listed there; everything else goes to
`unattributed/`. The per-block reasons and comments of the original archive are gone with the sandbox — this
file says so rather than inventing them.

  python3 tools/recover-archive.py [--base bbd2f1a]
"""
import os, re, subprocess, sys, io

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)
BASE = 'bbd2f1a'
if '--base' in sys.argv:
    BASE = sys.argv[sys.argv.index('--base') + 1]
LIVE = 'current/gex-signal-tapereader.user.js'
OUT = 'archive/v15.53'

# the CHANGELOG's own grouping of the retired blocks (names quoted in the v15.53 entry)
GROUPS = {
    'A-dead-else': ['readBlock44', 'kingHeaderBlock', 'briefBlockHtml', 'nextStopHtml', 'gexPathHtml', 'pbEntryHtml',
                    'accumBlock', 'nodeMapBlock', 'levelsHtmlV2', 'levelsCardHtml', 'lvlRow', 'lvlRows', 'lvlChart',
                    'lvlSvg', 'lvlTip', 'lvlLine', 'lvlBadge'],
    'B-hidden-em-rail': ['railLevelsLine', 'railRollLane', 'emPosRail', 'emRailHtml', 'emRailRows'],
    'C-flagged-off': ['secBias', 'secReact', 'secExec', 'stepState', 'rollDetect', 'nodeChip', 'esTick', 'velP',
                      'nodesListHtml', 'levelRowsHtml', 'nodeChartHtml'],
    'D-zero-callers': ['kingBlock', 'kingBadgeHtml', 'kingNarrativeHtml', 'kingPathChart', 'deflectionBlock',
                       'nodeMapSentence', 'trendBadgeHtml', 'outOfSyncBlock', 'ruleGet', 'driftLineHtml',
                       'kingPathSvg', 'projSvg'],
    'E-payload-probes': ['feedShape', 'callPutProbe'],
    'F-king-era-analytics': ['projScorecard', 'projScorecardHtml', 'kingAnalyticsHtml'],
    'G-hardcoded-prose': ['testingInsights', 'RECO_TESTS'],
    'H-write-only-state': ['NODEHIST', 'LAST_OK', 'SMA_CONT_FLAG', 'ACT_LAST'],
    'I-dead-css': [],
    'J-orphan-debug-hooks': [],
    'K-dark-pool-lifecycle': ['dpLifecycle', 'dpLifecycleHtml', 'dpEpisode', 'dpEpisodes'],
    'L-pip': ['pipToggle', 'pipOpen', 'pipRestore', 'pipCopyStyles'],
}

def src_at(ref, path):
    return subprocess.check_output(['git', 'show', '%s:%s' % (ref, path)], text=True, errors='replace')

FN = re.compile(r'^(?:function\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*\(|var\s+([A-Za-z_$][A-Za-z0-9_$]*)\s*=)', re.M)

def names(code):
    out = []
    for m in FN.finditer(code):
        out.append(m.group(1) or m.group(2))
    return out

def block(code, name):
    """the source of `function name(...)` {…} or `var name = …;` at top level"""
    m = re.search(r'^function\s+' + re.escape(name) + r'\s*\(', code, re.M)
    if m:
        i = code.index('{', m.end()); d = 0
        for k in range(i, len(code)):
            if code[k] == '{': d += 1
            elif code[k] == '}':
                d -= 1
                if d == 0:
                    return code[m.start():k + 1]
    m = re.search(r'^var\s+' + re.escape(name) + r'\s*=', code, re.M)
    if m:
        e = code.find('\n', m.end())
        # a multi-line literal: run to the first line that ends with ';'
        j = m.start()
        while True:
            e = code.find('\n', j)
            if e < 0: e = len(code)
            if code[j:e].rstrip().endswith(';'): return code[m.start():e]
            j = e + 1
            if j >= len(code): return code[m.start():]
    return None

base = src_at(BASE, LIVE)
live = io.open(LIVE, encoding='utf-8').read()
base_names, live_names = names(base), set(names(live))
retired = [n for n in base_names if n not in live_names]
# only names that are not referenced anywhere in the live file (comments excluded) — test_v1553 A3's rule
def strip_comments(s):
    s = re.sub(r'/\*[\s\S]*?\*/', '', s)
    return re.sub(r'(^|[^:\\])//[^\n]*', r'\1', s, flags=re.M)
live_nc = strip_comments(live)
retired = [n for n in retired if not re.search(r'(?<![A-Za-z0-9_$.\-])' + re.escape(n) + r'(?![A-Za-z0-9_$])', live_nc)]

group_of = {}
for g, lst in GROUPS.items():
    for n in lst: group_of[n] = g
rows, written = [], 0
for g in list(GROUPS.keys()) + ['unattributed']:
    os.makedirs(os.path.join(OUT, g), exist_ok=True)
for n in retired:
    g = group_of.get(n, 'unattributed')
    b = block(base, n)
    if b is None: continue
    p = os.path.join(OUT, g, n + '.js')
    io.open(p, 'w', encoding='utf-8').write('// archived at v15.53 (2026-09-03); recovered from git %s on 2026-09-04 by tools/recover-archive.py\n' % BASE + b + '\n')
    rows.append((n, g, b.count('\n') + 1)); written += 1

idx = ['# archive/v15.53 — INDEX (recovered 2026-09-04)', '',
       '⚠ **THIS IS A RECONSTRUCTION.** The v15.53 simplification (2026-09-03) wrote this directory in a cloud sandbox with',
       'every retired block, its reason and its pinning tests. It never reached GitHub: the installer manifest carried no',
       '`archive/` (nor `roadmap/`) and the cloud cannot push — landmine L-Q, the seventh directory lost that way. The code',
       'itself was never lost: it is in git at v15.51 (`%s`), and `tools/recover-archive.py` rewrote it here from that commit.' % BASE,
       'The grouping below is reconstructed from the v15.53 CHANGELOG entry; blocks the entry does not name are under',
       '`unattributed/`. The original per-block reasons are gone. `tests/` holds the 25 retired test files, moved there from',
       'the repo root on 2026-09-04 (they had stayed at the root on GitHub, red, because an installer can add files but never',
       'delete them — v15.64\'s installer removes them from the operator\'s tree).', '',
       '| block | group | lines |', '|---|---|---|']
for n, g, ln in rows:
    idx.append('| `%s` | %s | %d |' % (n, g, ln))
io.open(os.path.join(OUT, 'INDEX.md'), 'w', encoding='utf-8').write('\n'.join(idx) + '\n')
print('retired names in %s and absent from live: %d · written %d blocks · groups %d' % (BASE, len(retired), written, len(GROUPS) + 1))
