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
RTH_A, RTH_B = 8*3600+30*60, 15*3600      # 08:30-15:00 CT, the same window the study uses
# ⚠ OVERRIDABLE FOR TESTS ONLY. The first run of this tool during its own build wrote SYNTHETIC
# fixture prices straight into data/futures/ES/ - the exact path the real corpus lives in. Nothing
# would have flagged them: same columns, same filenames, plausible numbers. A test that writes into
# production storage is a corpus-poisoning bug waiting for the one run nobody watches.
OUT_ROOT = os.environ.get('GEX_FUTURES_OUT', 'data/futures')
HEADER = ['Symbol', 'Date', 'VOL', 'Open', 'High', 'Low', 'Close', 'Volume']


def day_files(argv):
    if argv:
        return argv
    return sorted(glob.glob('data/*.json'))


def harvest(paths):
    """(market, yahoo_symbol) -> { 'YYYY-MM-DD': { 'HH:MM:SS': row } }, RTH CT only."""
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
                    if not (RTH_A <= sec <= RTH_B):
                        continue
                    out[(mk, ysym)][dt.strftime('%Y-%m-%d')][dt.strftime('%H:%M:%S')] = [
                        ysym, dt.strftime('%Y-%m-%d %H:%M:%S'), '',
                        r[1], r[2], r[3], r[4], r[5] if len(r) > 5 else 0]
                except Exception:
                    continue
    return out, seen_files


def merge_write(market, ysym, day, rows_by_min):
    """Merge into any existing file for this market/day. Never destructive."""
    d = os.path.join(OUT_ROOT, market)
    os.makedirs(d, exist_ok=True)
    path = os.path.join(d, day + '.csv')
    merged = {}
    if os.path.exists(path):
        with io.open(path, encoding='utf-8') as f:
            for x in csv.DictReader(f):
                s = (x.get('Date') or '').strip()
                if ' ' in s:
                    merged[s.split(' ', 1)[1]] = [x.get(c, '') for c in HEADER]
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
    print('\n%d new minute(s) written under %s/' % (total_new, OUT_ROOT))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
