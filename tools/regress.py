#!/usr/bin/env python3
"""
regress.py — the ONE regression runner for all four RTX indicators (and the panel rows that feed them).

    python3 tools/regress.py                       all four indicators
    python3 tools/regress.py daymodel              one (gamma | daymodel | daystats | kingtracker), several may be listed
    python3 tools/regress.py all --no-record       run but do not append to testing/RESULTS.md
    python3 tools/regress.py gamma --syntax        also syntax-check the plugin .cpp with g++ against the SDK shim (cloud only)
    regress.bat [indicator]                        the same on the operator's machine (python + node + MSVC or g++)

Every indicator has three gates (testing/REGRESSION.md):
    GATE A  panel -> CSV     node tests that EXECUTE the panel's builders / samplers with stubs (v10.js = current/ copy)
    GATE B  CSV -> chart     C++ tests that pin the plugin's decisions in plugin/<Name>Logic.h (no IRT SDK needed)
    GATE L  the live runner  tools/gp-regress.py --dry on the pinned CSV + audit fixtures (same-moment runs that must keep passing)
The LIVE same-moment check on TODAY's export (a fresh CSV + audit + screenshots) is tools/gp-regress.py, a manual step.

Prints one table, appends one dated block to testing/RESULTS.md, exit code = number of failing suites.
The suites' own summary lines are parsed ("N passed, M failed" / "PASS a/b" / "FAIL a/b"); anything else is judged
by the exit code alone.
"""
import argparse, os, re, subprocess, sys, datetime, shutil, platform

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLUGIN = os.path.join(ROOT, 'plugin')
WIN = platform.system() == 'Windows'

# ---- the map: which suites belong to which indicator, and which source files each indicator is built from.
#      A build that touches one of the `sources` MUST update the suites listed beside it (the standing rule).
INDICATORS = {
    'gamma': dict(
        title='lsGammaProfile',
        sources=['current/gex-signal-tapereader.user.js (gammaProfileBuild, gpRegime, gpLevelRows, gpWallDepth, gpSlopeWord, the IF builder)',
                 'current/gex-if-levels.user.js (gammaFlip, gexProf, windows)', 'plugin/GammaProfile.cpp', 'plugin/GammaProfileLogic.h', 'plugin/ContractOffsetLogic.h'],
        gateA=['test_gammaprofile_build.js', 'test_if_extras.js'],
        gateB=['test_gammaprofile_logic.cpp', 'test_contractoffset_logic.cpp'],
        # Gate A only on the pinned exports: the 1325/1426 Atlas + IRT transcriptions carry resolution/timing 'fails' documented in
        # testing/gamma-profile/RESULTS.md (a 3% cell's sign by colour, a 1.1-pt ES1 gap on a 100-pt/hour tape) — not regressions
        live=[('testing/gamma-profile/fixtures/fixtureA-0947', []), ('testing/gamma-profile/fixtures/fixtureIF-1020', []), ('testing/gamma-profile/fixtures/GammaProfile-1325', []), ('testing/gamma-profile/fixtures/GammaProfile-1426', []), ('testing/gamma-profile/fixtures/1118-pool', []), ('testing/gamma-profile/fixtures/1118-pool-SPY', [])],
        cpp='GammaProfile.cpp'),
    'daymodel': dict(
        title='lsDayModel',
        sources=['current/gex-signal-tapereader.user.js (gpDayModel, gpDayEmPin, gpDayRecord*, GP_EM_MODEL, gpOpenWindow, the DAYEXP/EXPMODEL/DAYSE rows, AU.day)', 'tools/day-derive.js',
                 'tools/study-hodlod.py + data/es-1min/BASERATES.json (the baked base rates)', 'plugin/DayModel.cpp', 'plugin/DayModelLogic.h'],
        gateA=['test_daymodel_em.js', 'test_day_export.js', 'test_hodlod.js'],
        gateB=['test_daymodel_logic.cpp'],
        live=[('testing/day-model/fixtures/synth-1033', [])],
        cpp='DayModel.cpp'),
    'daystats': dict(
        title='lsDayStats',
        sources=['current/gex-signal-tapereader.user.js (hodlodCondE, gpOpenWindow, the CONDE/DAYSA/DAYSE rows, hlBaseNormalise, AU.day)', 'tools/day-derive.js',
                 'tools/study-hodlod.py (condstats) + BASERATES.json', 'plugin/DayStats.cpp', 'plugin/DayStatsLogic.h'],
        gateA=['test_daystats_cond.js', 'test_day_export.js', 'test_hodlod.js'],
        gateB=['test_daystats_logic.cpp'],
        live=[('testing/day-stats/fixtures/synth-1033', [])],
        cpp='DayStats.cpp'),
    'kingtracker': dict(
        title='lsKingTracker',
        sources=['current/gex-signal-tapereader.user.js (ktrkSample, ktrkFresh, the KINGTRACK/KINGNOW rows)',
                 'plugin/KingTracker.cpp', 'plugin/KingTrackerLogic.h', 'plugin/ContractOffsetLogic.h'],
        gateA=['test_kingtracker_rows.js'],
        gateB=['test_kingtracker_logic.cpp', 'test_contractoffset_logic.cpp'],
        live=[('testing/king-tracker/fixtures/synth-1033', [])],
        cpp='KingTracker.cpp'),
}
ORDER = ['gamma', 'daymodel', 'daystats', 'kingtracker']

def version_of(path, key):
    try:
        with open(os.path.join(ROOT, path), encoding='utf-8', errors='replace') as f:
            for line in f:
                m = re.search(key, line)
                if m: return m.group(1)
    except OSError: pass
    return '?'

def versions():
    return {
        'panel': version_of('current/gex-signal-tapereader.user.js', r'@version\s+(\S+)'),
        'companion': version_of('current/gex-if-levels.user.js', r'@version\s+(\S+)'),
        'gamma': version_of('plugin/GammaProfile.cpp', r'setVersion\("([^"]+)"'),
        'daymodel': version_of('plugin/DayModel.cpp', r'setVersion\("([^"]+)"'),
        'daystats': version_of('plugin/DayStats.cpp', r'setVersion\("([^"]+)"'),
        'kingtracker': version_of('plugin/KingTracker.cpp', r'setVersion\("([^"]+)"'),
    }

SUMMARY_RES = [re.compile(r'(\d+)\s+passed,\s+(\d+)\s+failed'), re.compile(r'\b(?:PASS|FAIL)\s+(\d+)/(\d+)\b')]
def parse_summary(text, rc):
    """-> (passed, total, ok)"""
    for line in reversed(text.strip().splitlines()):
        for rx in SUMMARY_RES:
            m = rx.search(line)
            if m:
                a, b = int(m.group(1)), int(m.group(2))
                if rx is SUMMARY_RES[0]: return a, a + b, (b == 0 and rc == 0)
                return a, b, (a == b and rc == 0)
    return None, None, rc == 0

def run(cmd, cwd, timeout=600):
    try:
        p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=timeout, encoding='utf-8', errors='replace')
        return p.returncode, (p.stdout or '') + (p.stderr or '')
    except FileNotFoundError as e:
        return 127, 'not found: %s' % e
    except subprocess.TimeoutExpired:
        return 124, 'timeout'

def node_test(fn):
    rc, out = run(['node', fn], ROOT)
    p, t, ok = parse_summary(out, rc)
    return dict(kind='A', name=fn, ok=ok, passed=p, total=t, rc=rc, out=out)

def cxx():
    """the C++ compiler at hand: g++ (cloud, MinGW) else cl (MSVC, when vcvars is already in the environment)."""
    if shutil.which('g++'): return 'g++'
    if shutil.which('cl'): return 'cl'
    return None

def logic_test(fn, compiler, exe_dir):
    stem = os.path.splitext(fn)[0]
    if compiler is None:
        return dict(kind='B', name=fn, ok=False, passed=None, total=None, rc=-1, out='no C++ compiler on PATH (cloud: g++; Windows: run plugin\\run-logic-tests.bat, which finds MSVC itself)', skipped=True)
    exe = os.path.join(exe_dir, stem + ('.exe' if WIN else ''))
    if compiler == 'g++':
        rc, out = run(['g++', '-std=c++14', '-O0', '-o', exe, fn], PLUGIN)
    else:
        rc, out = run(['cl', '/nologo', '/EHsc', '/W3', '/Fe:' + exe, '/Fo' + os.path.join(exe_dir, stem + '.obj'), fn], PLUGIN)
    if rc != 0:
        return dict(kind='B', name=fn, ok=False, passed=None, total=None, rc=rc, out='COMPILE FAILED\n' + out)
    rc, out = run([exe], PLUGIN)
    p, t, ok = parse_summary(out, rc)
    return dict(kind='B', name=fn, ok=ok, passed=p, total=t, rc=rc, out=out)

def syntax_check(cpp):
    """cloud-only: g++ -fsyntax-only against Linn's SDK header + tools/shim/shim.h (the MSVC-isms). Needs the SDK header
    at $IRTSDK_INC (a directory holding irtsdk.h); it is NOT in the repo (Linn's copyright)."""
    inc = os.environ.get('IRTSDK_INC') or '/tmp/claude-0/shim/inc'
    shim = os.path.join(ROOT, 'tools', 'shim', 'shim.h')
    if not (shutil.which('g++') and os.path.exists(os.path.join(inc, 'irtsdk.h')) and os.path.exists(shim)):
        return dict(kind='S', name=cpp, ok=False, passed=None, total=None, rc=-1, out='syntax check skipped: needs g++, tools/shim/shim.h and irtsdk.h at $IRTSDK_INC', skipped=True)
    rc, out = run(['g++', '-std=c++14', '-fsyntax-only', '-fpermissive', '-w', '-include', shim, '-I', inc, '-I', PLUGIN, cpp], PLUGIN)
    return dict(kind='S', name=cpp, ok=(rc == 0), passed=None, total=None, rc=rc, out=out)

LIVE_RX = re.compile(r'(\d+) checks, (\d+) failed')
def live_test(ind, stem, extra):
    """Gate L: tools/gp-regress.py --dry on a pinned CSV + audit pair (the live runner's own regression — a same-moment
    run that must still pass on the fixtures it passed on the day they were taken)."""
    csv, audit = stem + '.csv', stem + '.audit.json'
    name = '%s (%s)' % (os.path.basename(stem), 'live runner')
    if not (os.path.exists(os.path.join(ROOT, csv)) and os.path.exists(os.path.join(ROOT, audit))):
        return dict(kind='L', name=name, ok=False, passed=None, total=None, rc=-1, out='fixture missing: ' + stem, skipped=True)
    rc, out = run([sys.executable, os.path.join('tools', 'gp-regress.py'), '--indicator', ind, '--csv', csv, '--audit', audit, '--dry'] + list(extra), ROOT)
    m = LIVE_RX.search(out)
    if m:
        t, f = int(m.group(1)), int(m.group(2))
        return dict(kind='L', name=name, ok=(f == 0 and rc == 0), passed=t - f, total=t, rc=rc, out=out)
    return dict(kind='L', name=name, ok=(rc == 0), passed=None, total=None, rc=rc, out=out)

def fmt_count(r):
    if r.get('skipped'): return 'skipped'
    if r['passed'] is None: return 'ok' if r['ok'] else 'rc=%s' % r['rc']
    return '%d/%d' % (r['passed'], r['total'])

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('which', nargs='*', default=['all'], help='all | ' + ' | '.join(ORDER))
    ap.add_argument('--no-record', action='store_true', help='do not append to testing/RESULTS.md')
    ap.add_argument('--syntax', action='store_true', help='also g++ -fsyntax-only the plugin .cpp (cloud, needs the SDK header)')
    ap.add_argument('--verbose', '-v', action='store_true', help='print every suite\'s output')
    ap.add_argument('--note', default='', help='a note for the RESULTS.md block')
    a = ap.parse_args()
    which = ORDER if 'all' in a.which else [w.lower() for w in a.which]
    bad = [w for w in which if w not in INDICATORS]
    if bad:
        print('unknown indicator(s): %s   (choose from: all %s)' % (' '.join(bad), ' '.join(ORDER))); return 2

    # Gate A reads v10.js — ALWAYS regenerated from current/ (a stale-but-green v10.js is the dangerous state; tools/run-tests.sh)
    shutil.copyfile(os.path.join(ROOT, 'current', 'gex-signal-tapereader.user.js'), os.path.join(ROOT, 'v10.js'))
    V = versions()
    comp = cxx()
    exe_dir = os.path.join(ROOT, 'plugin', '.regress-bin'); os.makedirs(exe_dir, exist_ok=True)
    now = datetime.datetime.now()
    print('regress.py  %s   panel %s  companion %s  GP %s  DM %s  DS %s  KT %s   C++: %s' % (
        now.strftime('%Y-%m-%d %H:%M'), V['panel'], V['companion'], V['gamma'], V['daymodel'], V['daystats'], V['kingtracker'], comp or 'none'))

    rows = []       # (indicator, result)
    done_A, done_B = {}, {}   # a suite shared by two indicators (test_hodlod.js, test_contractoffset_logic.cpp) runs once
    for w in which:
        I = INDICATORS[w]
        for fn in I['gateA']:
            if fn not in done_A: done_A[fn] = node_test(fn)
            rows.append((w, done_A[fn]))
        for fn in I['gateB']:
            if fn not in done_B: done_B[fn] = logic_test(fn, comp, exe_dir)
            rows.append((w, done_B[fn]))
        for stem, extra in I.get('live', []):
            rows.append((w, live_test(w, stem, extra)))
        if a.syntax:
            rows.append((w, syntax_check(I['cpp'])))

    # ---- the table
    print()
    print('%-12s %-5s %-34s %-9s %s' % ('indicator', 'gate', 'suite', 'result', 'status'))
    print('-' * 78)
    fails = 0; skipped = 0
    for w, r in rows:
        st = 'SKIP' if r.get('skipped') else ('ok' if r['ok'] else 'FAIL')
        if st == 'FAIL': fails += 1
        if st == 'SKIP': skipped += 1
        print('%-12s %-5s %-34s %-9s %s' % (w, r['kind'], r['name'], fmt_count(r), st))
        if a.verbose or st == 'FAIL':
            for line in r['out'].strip().splitlines()[-40:]: print('        | ' + line)
    print('-' * 78)
    verdict = ('%d suite(s) FAILED' % fails) if fails else ('ALL GREEN' + (' (%d skipped)' % skipped if skipped else ''))
    print(verdict)

    # ---- the trail
    if not a.no_record:
        res = os.path.join(ROOT, 'testing', 'RESULTS.md')
        os.makedirs(os.path.dirname(res), exist_ok=True)
        new = not os.path.exists(res)
        with open(res, 'a', encoding='utf-8') as f:
            if new:
                f.write('# Regression results — all indicators\n\n_Appended by `tools/regress.py` on every run (one block per run, newest at the bottom). '
                        'Per-indicator detail and the live same-moment runs: `testing/REGRESSION.md`._\n')
            f.write('\n## %s  ·  %s  ·  panel %s / companion %s / GP %s / DM %s / DS %s / KT %s  ·  %s\n\n' % (
                now.strftime('%Y-%m-%d %H:%M'), ' '.join(which), V['panel'], V['companion'], V['gamma'], V['daymodel'], V['daystats'], V['kingtracker'], verdict))
            if a.note: f.write('_%s_\n\n' % a.note)
            f.write('| indicator | gate | suite | result | status |\n|---|---|---|---|---|\n')
            for w, r in rows:
                st = 'skipped' if r.get('skipped') else ('ok' if r['ok'] else '**FAIL**')
                f.write('| %s | %s | `%s` | %s | %s |\n' % (w, r['kind'], r['name'], fmt_count(r), st))
            f.write('\n_host: %s, C++: %s_\n' % (platform.node() or platform.system(), comp or 'none'))
        print('recorded -> testing/RESULTS.md')
    return fails

if __name__ == '__main__':
    sys.exit(main())
