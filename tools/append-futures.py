#!/usr/bin/env python3
"""
APPEND FUTURES BARS — day-file couriered bars -> the per-market 1-minute corpus.

    python3 tools/append-futures.py                 # every data/*.json into data/futures/
    python3 tools/append-futures.py data/2026-08-28.json

WHERE THIS SITS. The companion (v1.15) fetches Yahoo 1-minute bars and writes them to
localStorage; the panel carries them into data/YYYY-MM-DD.json under `futBars`; the installer or
push-data.bat commits that file. This script is the LAST leg: it turns those raw couriered bars into
per-session CSVs that tools/study-hodlod.py can read with no new parser.

⚠⚠ ALL SESSION LOGIC LIVES HERE AND NOWHERE ELSE. The courier is deliberately dumb - it does no
timezone conversion and no RTH classification - because a sandboxed userscript doing DST arithmetic
is how a corpus goes quietly wrong for half the year. (The companion's own ctToday() hardcodes -5h
and IS wrong in CST; it predates this rule.) Here we have a real tz database, so here is where the
clock is decided.

⚠ THE OUTPUT COLUMNS MATCH THE VENDOR CORPUS EXACTLY - Symbol,Date,VOL,Open,High,Low,Close,Volume -
so study-hodlod.py's existing load() reads a Yahoo daily and a vendor corpus file with the same code
path. One parser, one definition of a bar.

⚠ IDEMPOTENT BY CONSTRUCTION. The courier re-sends a 5-day window every poll, so the same minute
arrives many times. Rows are keyed by (market, minute) and re-running this over the same day files
changes nothing. That is what makes a missed day recoverable instead of fatal.

⚠ PROVENANCE IS NOT OPTIONAL. Yahoo rows are `ES=F`, the CONTINUOUS front-month quote; the vendor
corpus is `EPM26`, ONE contract. They differ by the calendar spread across a roll - points, not
ticks. Yahoo rows are therefore written under the Yahoo symbol and NEVER merged into the vendor file.
For HOD/LOD the statistics are a CLOCK and a RANGE, and a constant basis shifts neither - but that is
an argument, not a measurement, so --report prints the overlap and nothing pools until it is checked.
"""
import csv, glob, io, json, os, sys, collections
from datetime import datetime
from zoneinfo import ZoneInfo

CT = ZoneInfo('America/Chicago')
# (v15.87) THE TOOL GRID: the study folds minutes into 3-minute bars stamped by END, and the bar labelled 08:30 is
# 08:27-08:30 — its open is the session open his tool reads (operator, 2026-09-09: "tools"). So the harvest starts at
# 08:27 and stops before the 15:00 minute (which would belong to a bar ending 15:03). 393 minutes is a whole session.
RTH_A, RTH_B = 8*3600+27*60, 15*3600      # 08:27 <= s < 15:00 CT, the same window the study loads
# ⚠ OVERRIDABLE FOR TESTS ONLY. The first run of this tool during its own build wrote SYNTHETIC
# fixture prices straight into data/futures/ES/ - the exact path the real corpus lives in. Nothing
# would have flagged them: same columns, same filenames, plausible numbers. A test that writes into
# production storage is a corpus-poisoning bug waiting for the one run nobody watches.
OUT_ROOT = os.environ.get('GEX_FUTURES_OUT', 'data/futures')
HEADER = ['Symbol', 'Date', 'VOL', 'Open', 'High', 'Low', 'Close', 'Volume']
# (v15.90) THE NIGHT, TOO. The sweep study (study-sweeps.py) needs the overnight session — ONH / ONL, Asia, London — and
# its corpus had stopped on the vendor's last day (2026-08-21) because these CSVs carried RTH only: H7 on the register
# read n = 0 after thirteen sessions. The night bars (17:00 CT of the evening before → 08:27 of the session day) go to a
# second file per SESSION day, <day>-night.csv, same columns; the RTH files are untouched (study-hodlod reads those and
# only those — market_sources() skips the night files). The 15:00–17:00 post-close bars are dropped on purpose (F-23).
NIGHT_A = 17*3600
def session_day_of(dt):
    """the session a night bar belongs to: >= 17:00 -> the next weekday; < 08:27 -> the same day; else None."""
    from datetime import timedelta
    sec = dt.hour*3600 + dt.minute*60 + dt.second
    if sec >= NIGHT_A:
        nd = dt + timedelta(days=1)
        while nd.weekday() >= 5:      # a Friday-evening row does not exist on CME; a Sunday evening is Monday's night
            nd = nd + timedelta(days=1)
        return nd.strftime('%Y-%m-%d')
    if sec < RTH_A:
        return dt.strftime('%Y-%m-%d')
    return None


def day_files(argv):
    if argv:
        return argv
    return sorted(glob.glob('data/*.json'))


def harvest(paths, night=False):
    """(market, yahoo_symbol) -> { 'YYYY-MM-DD': { 'HH:MM:SS': row } }, RTH CT only — or, with night=True, the overnight
    bars keyed by the SESSION day they precede (17:00 the evening before -> 08:27), stamped 'YYYY-MM-DD HH:MM:SS'."""
    out = collections.defaultdict(lambda: collections.defaultdict(dict))
    seen_files = 0
    for p in paths:
        try:
            with io.open(p, encoding='utf-8') as f:
                d = json.load(f)
        except Exception as e:
            print('  skip %s (%s)' % (p, e))
            continue
        fb = d.get('futBars')
        if not isinstance(fb, dict):
            continue
        seen_files += 1
        for mk, m in fb.items():
            if mk.startswith('_') or not isinstance(m, dict):
                continue
            if m.get('err') or not m.get('rows'):
                continue
            ysym = m.get('sym') or mk
            for r in m['rows']:
                try:
                    t = int(r[0])
                    dt = datetime.fromtimestamp(t, tz=CT)
                    sec = dt.hour*3600 + dt.minute*60 + dt.second
                    if night:
                        sday = session_day_of(dt)
                        if sday is None or dt.second:
                            continue
                        out[(mk, ysym)][sday][dt.strftime('%Y-%m-%d %H:%M:%S')] = [
                            ysym, dt.strftime('%Y-%m-%d %H:%M:%S'), '',
                            r[1], r[2], r[3], r[4], r[5] if len(r) > 5 else 0]
                        continue
                    if not (RTH_A <= sec < RTH_B):
                        continue
                    # (v15.87) THE LIVE QUOTE IS NOT A BAR. Yahoo's last row is the in-progress minute stamped at the
                    # quote's own second (14:49:41, o=h=l=c, volume 0); it is not on the minute grid and it is
                    # superseded by the real bar on the next poll. Dropped here so no session ends on a phantom row.
                    if dt.second:
                        continue
                    out[(mk, ysym)][dt.strftime('%Y-%m-%d')][dt.strftime('%H:%M:%S')] = [
                        ysym, dt.strftime('%Y-%m-%d %H:%M:%S'), '',
                        r[1], r[2], r[3], r[4], r[5] if len(r) > 5 else 0]
                except Exception:
                    continue
    return out, seen_files


NIGHT_MIN = 200     # (v15.90) a night file is written only when the harvest holds a real night (the sweep study's MIN_ON) — a
                    # RTH-only courier (GC / CL, and NQ before companion v1.19) yields a 27-minute pre-market stub, not a night
def merge_write(market, ysym, day, rows_by_min, suffix=''):
    """Merge into any existing file for this market/day. Never destructive. suffix '-night' (v15.90): the overnight file,
    keyed by the full stamp (two calendar dates share one session night); a stub under NIGHT_MIN minutes is not written
    unless the file already exists (a partial night completes from the next day file's window)."""
    d = os.path.join(OUT_ROOT, market)
    os.makedirs(d, exist_ok=True)
    path = os.path.join(d, day + suffix + '.csv')
    if suffix and len(rows_by_min) < NIGHT_MIN and not os.path.exists(path):
        return path, 0, 0
    merged = {}
    if os.path.exists(path):
        with io.open(path, encoding='utf-8') as f:
            for x in csv.DictReader(f):
                s = (x.get('Date') or '').strip()
                if ' ' in s and s.endswith(':00'):        # (v15.87) a live-quote row written before this rule is dropped on rewrite
                    merged[(s if suffix else s.split(' ', 1)[1])] = [x.get(c, '') for c in HEADER]
    before = len(merged)
    merged.update(rows_by_min)
    if len(merged) == before and os.path.exists(path):
        return path, before, 0
    with io.open(path, 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f)
        w.writerow(HEADER)
        for k in sorted(merged):
            w.writerow(merged[k])
    return path, len(merged), len(merged) - before


def main(argv):
    paths = day_files(argv)
    print('reading %d day file(s)' % len(paths))
    got, seen = harvest(paths)
    if not got:
        print('\nNO futBars IN ANY DAY FILE.')
        print('That is expected until a panel running v14.59+ and a companion running v1.15+ have')
        print('both been live for a session. It is NOT a failure of this script - and it is not a')
        print('reason to invent rows. Check __gptsDebug.futBars() on the live tab.')
        return 1
    total_new = 0
    for (market, ysym), days in sorted(got.items()):
        print('\n%s  (%s)' % (market, ysym))
        for day in sorted(days):
            rows = days[day]
            path, n, added = merge_write(market, ysym, day, rows)
            total_new += added
            # 391 one-minute bars is a complete 08:30-15:00 CT session; say so rather than
            # letting a half day quietly enter the corpus looking whole.
            flag = '' if n >= 386 else '   <- INCOMPLETE (%d bars; the study needs >=386)' % n
            print('  %s  %4d bars  (+%d)%s' % (day, n, added, flag))
    # (v15.90) the nights, one file per session day
    nights, _ = harvest(paths, night=True)
    total_night = 0
    for (market, ysym), days in sorted(nights.items()):
        print('\n%s  (%s)  the nights' % (market, ysym))
        for day in sorted(days):
            path, n, added = merge_write(market, ysym, day, days[day], suffix='-night')
            total_night += added
            flag = '' if n >= 200 else '   <- SHORT (%d bars; the sweep study needs >=200)' % n
            print('  %s-night  %4d bars  (+%d)%s' % (day, n, added, flag))
    print('\n%d new minute(s) written under %s/ · %d night minute(s)' % (total_new, OUT_ROOT, total_night))
    return 0


def selftest():
    """(v15.90) the nights: keyed by the SESSION day, the post-close dropped, a Sunday evening is Monday's night."""
    import tempfile, shutil, time as _t
    global OUT_ROOT
    keep = OUT_ROOT; root = tempfile.mkdtemp(); OUT_ROOT = os.path.join(root, 'futures')
    try:
        def ts(s):
            return int(datetime.strptime(s, '%Y-%m-%d %H:%M:%S').replace(tzinfo=CT).timestamp())
        rows = [[ts('2026-09-04 15:30:00'), 1, 2, 0, 1, 5],      # post-close: dropped
                [ts('2026-09-06 17:00:00'), 1, 2, 0, 1, 5],      # Sunday evening -> Monday 09-07's night
                [ts('2026-09-07 23:30:00'), 1, 2, 0, 1, 5],      # Monday evening -> 09-08's night
                [ts('2026-09-08 02:00:00'), 1, 2, 0, 1, 5],      # 09-08's night
                [ts('2026-09-08 08:26:00'), 1, 2, 0, 1, 5],      # 09-08's night (the last minute before the RTH harvest)
                [ts('2026-09-08 08:27:00'), 1, 2, 0, 1, 5],      # RTH
                [ts('2026-09-08 08:27:41'), 1, 2, 0, 1, 0]]      # the live quote: dropped on both paths
        day = os.path.join(root, '2026-09-08.json')
        io.open(day, 'w').write(json.dumps(dict(futBars=dict(ES=dict(sym='ES=F', rows=rows)))))
        got, _ = harvest([day]); night, _ = harvest([day], night=True)
        assert list(got[('ES', 'ES=F')].keys()) == ['2026-09-08'] and list(got[('ES', 'ES=F')]['2026-09-08'].keys()) == ['08:27:00'], got
        nd = night[('ES', 'ES=F')]
        assert sorted(nd.keys()) == ['2026-09-07', '2026-09-08'], sorted(nd.keys())
        assert sorted(nd['2026-09-08'].keys()) == ['2026-09-07 23:30:00', '2026-09-08 02:00:00', '2026-09-08 08:26:00'], sorted(nd['2026-09-08'].keys())
        assert list(nd['2026-09-07'].keys()) == ['2026-09-06 17:00:00']
        p0, n0, a0 = merge_write('ES', 'ES=F', '2026-09-08', nd['2026-09-08'], suffix='-night')
        assert n0 == 0 and a0 == 0 and not os.path.exists(p0), 'a 3-minute stub is not a night: nothing written'
        big = dict(('2026-09-08 %02d:%02d:00' % (h, m), ['ES=F', '2026-09-08 %02d:%02d:00' % (h, m), '', 1, 2, 0, 1, 5]) for h in range(0, 8) for m in range(0, 60))
        big.update(nd['2026-09-08'])
        p1, n1, a1 = merge_write('ES', 'ES=F', '2026-09-08', big, suffix='-night')
        assert p1.endswith('2026-09-08-night.csv') and n1 == len(big) and a1 == len(big)
        p2, n2, a2 = merge_write('ES', 'ES=F', '2026-09-08', nd['2026-09-08'], suffix='-night')
        assert n2 == len(big) and a2 == 0, (n2, a2)                # idempotent; an existing file keeps merging
        txt = io.open(p1).read()
        assert txt.startswith('Symbol,Date,VOL,Open,High,Low,Close,Volume') and 'ES=F,2026-09-07 23:30:00,' in txt
        print('append-futures selftest ok')
    finally:
        OUT_ROOT = keep; shutil.rmtree(root, ignore_errors=True)


if __name__ == '__main__':
    if '--selftest' in sys.argv:
        selftest(); sys.exit(0)
    sys.exit(main(sys.argv[1:]))
