#!/usr/bin/env python3
"""
HOD/LOD BASE RATES over the ES 1-min corpus.  Every figure the ⓪a section displays is derived here,
with its n, so nothing on the panel is a number we cannot reproduce.

⚠ DEFINITIONS ARE THE WHOLE BALLGAME and the mockup states them loosely, so they are pinned here:
  RTH            08:30-15:00 CT inclusive = 391 one-minute bars.  A COMPLETE session has >=386 of
                 them; that threshold is what reproduces the mockup header's "284d".  At >=391 the
                 count is 283.  Same corpus, different filter - the prior note warned about exactly
                 this, so the filter is stated rather than assumed.
  HOD / LOD      max(High) / min(Low) over RTH, FIRST occurrence if it repeats.
  1ST / 2ND      whichever extremity printed first, and the other.
  TOOK           minutes from the RTH open to the 1st extremity.
  HL GAP         minutes between the two extremities.
  HL RNG         HOD-LOD, in points and in dollars (ES = $50/point).
  THE LADDER     a SURVIVAL statistic, and the one thing on the section that is genuinely
                 predictive: walk the session, track the running extreme; every time a new one is
                 set, the previous candidate "died" having stood some number of minutes.  For a
                 holding window W, among all candidates that stood >= W, what fraction were still
                 the extreme at the close?  That is "the longer the low has stood, the likelier it
                 is the low of the day", as a measured rate rather than an intuition.
"""
import csv, collections, glob, gzip, io, json, os, sys, statistics as st

RTH_A, RTH_B = 8*3600+30*60, 15*3600
# (v15.87) THE TOOL GRID — operator, 2026-09-09: "tools". His tool reads the session on 3-minute bars stamped by their
# END; the bar labelled 08:30 is 08:27–08:30 and its open IS the session open (7715.00 on 09-08, where the first print
# at 08:30:00 was 7711.50 — that one number was the whole 6% vs 14% wick disagreement). So: the minutes are loaded
# from 08:27 up to 14:59 (the bar ending 15:00), folded into 3-minute bars stamped by end, and every clock in the wick
# family is a bar END; the reclaim bar is the first bar AFTER the extreme's bar. The survival ladder stays on minutes.
LOAD_A, LOAD_B = 8*3600+27*60, 15*3600          # minutes s with LOAD_A <= s < LOAD_B
BAR = 180
MIN_BARS = 386
PT_USD = 50.0
WINDOWS = [30, 60, 90, 120, 180]

def _open(path):
    """.gz or plain - the vendor corpus ships gzipped, the dailies do not."""
    if path.endswith('.gz'):
        return io.TextIOWrapper(gzip.open(path, 'rb'), encoding='utf-8')
    return io.open(path, encoding='utf-8')


def load(path, ses=None):
    """Accumulate sessions from one CSV. Columns are Symbol,Date,VOL,Open,High,Low,Close,Volume for
    BOTH the vendor corpus and the Yahoo dailies - tools/append-futures.py writes that layout
    deliberately so there is exactly one parser and one definition of a bar."""
    ses = collections.defaultdict(list) if ses is None else ses
    with _open(path) as f:
        for x in csv.DictReader(f):
            s = (x.get('Date') or '').strip()
            if ' ' not in s:
                continue
            d, t = s.split(' ', 1)
            p = t.split(':')
            try:
                sec = int(p[0])*3600 + int(p[1])*60
                if not (LOAD_A <= sec < LOAD_B):
                    continue
                ses[d].append((sec, float(x['High']), float(x['Low']), float(x['Close']),
                               float(x['Open'])))
            except (ValueError, IndexError):
                continue
    return ses


def complete(ses):
    """⚠ MIN_BARS IS LOAD-BEARING: >=386 gives 284 sessions on the vendor corpus, >=391 gives 283,
    and one 386-bar session is the entire difference. Changing it changes the filter, not the data."""
    return {d: sorted(v) for d, v in ses.items() if len(v) >= MIN_BARS}


def load_sources(paths):
    """Merge every source into one session map, and report which day came from where.

    ⚠⚠ PROVENANCE IS RECORDED, NOT ASSUMED. The vendor corpus is EPM26 - ONE contract. The Yahoo
    dailies are ES=F - the CONTINUOUS front-month quote. They differ by the calendar spread across a
    roll: points, not ticks. For HOD/LOD the statistics are a CLOCK and a RANGE and a constant basis
    shifts neither, but that is an ARGUMENT, not a measurement - so every day carries its source and
    `sources` lands in BASERATES.json where the panel and any later study can see the mix.
    ⚠ A day present in BOTH wins from the FIRST source listed (the vendor corpus), because a
    single-contract print is the more authoritative of the two. Never averaged."""
    ses, prov = collections.defaultdict(list), {}
    for path in paths:
        one = load(path)
        for d, bars in one.items():
            if d in ses:
                continue                      # first source wins; see above
            ses[d] = bars
            prov[d] = os.path.basename(path)
    return ses, prov

# ---------------------------------------------------------------------------------------------
# THE WICK FAMILY — operator-defined 2026-08-28, then CONFIRMED against the live ES tape.
#
# His mockup prints six fields this study did not compute. They were derived from his own printed
# numbers and then verified bar-by-bar on 2026-08-27, where the two fields that do not depend on
# extremity timing landed EXACTLY: Wick% 26 vs 26, W.End 8:42am vs 8:42am.
#
#   TOOK    open -> the first extremity
#   W.END   the first bar to CLOSE back through the SESSION OPEN after that extremity.
#           ⚠ CLOSE, not touch. On 2026-08-27 the first touch was 8:41 and the first close 8:42;
#           his panel prints 8:42. One session decided this - if a later day disagrees, revisit.
#   BOP     first extremity -> W.END (the recovery leg)
#   WICK    TOOK + BOP, i.e. open -> W.END (the whole opening excursion)
#   WICK%   |open - first extremity| / total range. ⚠ A PRICE ratio, NOT a duration: no duration
#           ratio can produce his printed 26% (wick/session = 3.1%, wick/gap = 5.6%).
#   MUD     W.END -> the second extremity (the middle, after the open resolves)
#
# ⚠⚠ HIS EXCLUSION RULE, VERBATIM 2026-08-28: "if there is no wick, then its 0 ... these days
# should not be averaged. also crazy outliers should not be averaged."
# So: a zero-wick session REPORTS 0 and is EXCLUDED from the wick-family medians. It is still a
# perfectly good session for TOOK, HL GAP and HL RNG, so it is excluded from those medians NOWHERE.
# Outliers are fenced by TUKEY 1.5xIQR, which is computed FROM THE CORPUS rather than chosen by me,
# and every exclusion is COUNTED into the output so nothing is dropped invisibly.
def tool_bars(mins):
    """(v15.87) minutes (sec, high, low, close, open) sorted -> 3-minute bars (END sec, high, low, close, open) stamped
    by their end, the first one ending 08:30 (its open = the session open, his tool's way)."""
    out = []
    for m in mins:
        end = ((m[0] - LOAD_A) // BAR + 1) * BAR + LOAD_A
        if out and out[-1][0] == end:
            b = out[-1]; out[-1] = (end, max(b[1], m[1]), min(b[2], m[2]), m[3], b[4])
        else:
            out.append((end, m[1], m[2], m[3], m[4]))
    return out


def wick_fields(bars):
    """bars: 3-minute TOOL bars (end_sec, high, low, close, open) sorted. Returns the six fields for one session.
    TOOK is from 08:30 (RTH_A) to the first extreme's bar END; the reclaim bar is the first bar AFTER the extreme's."""
    o0, OPEN = RTH_A, bars[0][4]
    hi = max(bars, key=lambda r: r[1]); lo = min(bars, key=lambda r: r[2])
    hod_t, lod_t = hi[0], lo[0]
    low_first = lod_t < hod_t
    first_t = lod_t if low_first else hod_t
    second_t = hod_t if low_first else lod_t
    ext = lo[2] if low_first else hi[1]
    rng = hi[1] - lo[2]
    wick_pts = abs(OPEN - ext)
    wend = None
    for b in bars:
        if b[0] <= first_t:                      # (v15.87) the extreme's own bar never reclaims — his tool's 8:36, not 8:33
            continue
        if (b[3] >= OPEN) if low_first else (b[3] <= OPEN):
            wend = b[0]
            break
    took = (first_t - o0) // 60
    out = dict(first='LOD' if low_first else 'HOD', took=took,
               wick_pts=round(wick_pts, 2),
               wick_pct=round(100 * wick_pts / rng) if rng > 0 else None,
               reclaimed=wend is not None)
    if wend is None:
        # never closed back through the open: there is no completed wick. NOT zero - UNKNOWN.
        # Reporting 0 here would say "the excursion ended instantly", the opposite of what happened.
        out.update(wend=None, bop=None, wick=None, mud=None)
    else:
        out.update(wend=wend, bop=(wend - first_t) // 60, wick=(wend - o0) // 60,
                   mud=max(0, (second_t - wend) // 60))
    return out


def tukey_keep(vals):
    """Tukey 1.5xIQR fence. Returns (kept, n_dropped). The fence comes from the data, not from me."""
    xs = sorted(v for v in vals if v is not None)
    if len(xs) < 8:
        return xs, 0
    def pc(p):
        i = (len(xs) - 1) * p
        a, b = int(i), min(int(i) + 1, len(xs) - 1)
        return xs[a] + (xs[b] - xs[a]) * (i - a)
    q1, q3 = pc(0.25), pc(0.75)
    iqr = q3 - q1
    lo_f, hi_f = q1 - 1.5 * iqr, q3 + 1.5 * iqr
    kept = [v for v in xs if lo_f <= v <= hi_f]
    return kept, len(xs) - len(kept)


def survival(bars, low_side):
    """(stood_minutes, survived_to_close) for every running-extreme candidate in one session."""
    out, best, since = [], None, None
    for sec, hi, lo, _c, _o in bars:
        v = lo if low_side else hi
        new = best is None or (v < best if low_side else v > best)
        if new:
            if best is not None:
                out.append(((sec - since)//60, False))
            best, since = v, sec
    out.append(((bars[-1][0] - since)//60, True))
    return out

def main(paths, out=None, market='ES'):
    global PT_USD
    PT_USD = {'ES': 50.0, 'NQ': 20.0, 'GC': 100.0, 'CL': 1000.0}.get(market, 50.0)   # (v15.87) the contract multiplier, by market
    if isinstance(paths, str):
        paths = [paths]
    raw, prov = load_sources(paths)
    ses = complete(raw)
    dropped = sorted(set(raw) - set(ses))
    days = sorted(ses)
    if not days:
        print('NO COMPLETE SESSIONS in %s.' % ', '.join(paths), file=sys.stderr)
        print('Refusing to emit base rates from nothing - an empty ladder would render as a', file=sys.stderr)
        print('confident 0%%. Check the inputs; do not lower MIN_BARS to make this pass.', file=sys.stderr)
        return None
    rows, surv_lo, surv_hi = [], [], []
    for d in days:
        mins = ses[d]
        b = tool_bars(mins)                      # (v15.87) the tool grid: 3-minute bars stamped by end, from 08:27
        hi = max(b, key=lambda r: r[1]); lo = min(b, key=lambda r: r[2])
        hod_t, lod_t = hi[0], lo[0]              # bar ENDS — his tool's clocks (HOD 8:33 = the 08:30–08:33 bar)
        first_low = lod_t < hod_t
        rng = hi[1] - lo[2]
        rec = dict(day=d, hod_t=hod_t, lod_t=lod_t, first='LOD' if first_low else 'HOD',
                   took=max(0, min(hod_t, lod_t)-RTH_A)//60, gap=abs(hod_t-lod_t)//60,
                   rng_pts=round(rng, 2), rng_usd=round(rng*PT_USD, 2),
                   # (v15.77) the session's colour, the close against the (tool) open — his tool's "Red Day / Green Day"
                   green=(b[-1][3] > b[0][4]))
        rec.update(wick_fields(b))
        rows.append(rec)
        rth = [m for m in mins if m[0] >= RTH_A]          # the ladder keeps counting from 08:30, on minutes
        surv_lo += survival(rth, True); surv_hi += survival(rth, False)

    def ladder(sv):
        out = {}
        for w in WINDOWS:
            elig = [s for s in sv if s[0] >= w]
            hit = sum(1 for s in elig if s[1])
            out[w] = dict(n=len(elig), held=hit,
                          rate=round(100*hit/len(elig)) if elig else None)
        return out

    # ⚠⚠ EVERY E FIELD IS A TRIMMED MEAN. Operator, 2026-08-28: "the e row is the expected result
    # based on averages" and, when I had switched only the wick columns, "i thought they were all
    # averages." He is right and the split was my invention: TOOK / HL GAP / HL RNG / the clocks
    # were still medians because v14.57 had verified them that way against an older mockup. One row,
    # one statistic - a table where two columns are means and three are medians is a table nobody
    # can reason about.
    # ⚠ "also crazy outliers should not be averaged", so the SAME Tukey 1.5xIQR fence applies here
    # as to the wick family, and the count of what it removed is reported per field.
    # ⚠ p25/p75 stay TRUE PERCENTILES. They are explicitly a spread, not an average, and trimming a
    # quantile would make it describe a range it no longer covers.
    trim_drop = {}
    def med(xs, _key=None):
        if not xs:
            return None
        kept, n_out = tukey_keep(xs)
        if _key:
            trim_drop[_key] = n_out
        return round(sum(kept) / len(kept), 1) if kept else None
    med_true = lambda xs: round(st.median(xs), 1) if xs else None
    firsts = collections.Counter(r['first'] for r in rows)
    res = dict(
        corpus=dict(sessions=len(days), first=days[0], last=days[-1],
                    min_bars=MIN_BARS, rth='08:30-15:00 CT', pt_usd=PT_USD,
                    definition='tool grid (v15.87): 3-minute bars stamped by END, the session = the bars ending 08:30-15:00 (minutes 08:27-14:59), the open = the bar ending 08:30; clocks are bar ends; the reclaim bar is after the extreme\'s bar'),
        sequence=dict(LOD_first=firsts['LOD'], HOD_first=firsts['HOD'],
                      pct_LOD_first=round(100*firsts['LOD']/len(rows))),
        expected=dict(
            took_min=med([r['took'] for r in rows], 'took'),
            gap_min=med([r['gap'] for r in rows], 'gap'),
            rng_pts=med([r['rng_pts'] for r in rows], 'rng_pts'),
            rng_usd=med([r['rng_usd'] for r in rows], 'rng_usd'),
            rng_p25=round(st.quantiles([r['rng_pts'] for r in rows], n=4)[0], 1),
            rng_p75=round(st.quantiles([r['rng_pts'] for r in rows], n=4)[2], 1),
            first_clock=med([min(r['hod_t'], r['lod_t']) for r in rows], 'first_clock'),
            second_clock=med([max(r['hod_t'], r['lod_t']) for r in rows], 'second_clock'),
            statistic='trimmed mean (Tukey 1.5xIQR outliers excluded), operator-specified 2026-08-28',
            outliers_excluded=trim_drop,
            median_for_contrast=dict(
                took_min=med_true([r['took'] for r in rows]),
                gap_min=med_true([r['gap'] for r in rows]),
                rng_pts=med_true([r['rng_pts'] for r in rows]),
                first_clock=med_true([min(r['hod_t'], r['lod_t']) for r in rows]),
                second_clock=med_true([max(r['hod_t'], r['lod_t']) for r in rows]))),
        ladder=dict(low=ladder(surv_lo), high=ladder(surv_hi),
                    both=ladder(surv_lo + surv_hi)),
    )
    # ---- THE WICK FAMILY, with his two exclusions applied and COUNTED ------------------------
    # "if there is no wick, then its 0 ... these days should not be averaged. also crazy outliers
    # should not be averaged."  Zero-wick and never-reclaimed sessions leave the wick medians;
    # Tukey fences the rest. Every drop is reported so the n on the face is the n behind the number.
    wick_rows = [r for r in rows if r.get('reclaimed')]
    n_never = len(rows) - len(wick_rows)
    n_zero = sum(1 for r in wick_rows if r['wick'] == 0)
    pool = [r for r in wick_rows if r['wick'] not in (None, 0)]
    wick_stats, wick_drop = {}, {}
    for key in ('wick', 'bop', 'mud', 'wick_pct', 'wick_pts'):
        # ⚠ NOT `dropped` — that name already holds the incomplete-session list built above, and
        # shadowing it made len(dropped) explode 40 lines later. GREP BEFORE NAMING (5th collision).
        kept, n_out = tukey_keep([r[key] for r in pool])
        # ⚠ THE MEAN, NOT THE MEDIAN. Operator, 2026-08-28: "the e row is the expected result based
        # on AVERAGES." Combined with "no wick days should not be averaged. also crazy outliers
        # should not be averaged", that is a TRIMMED MEAN: exclude the zero-wick days and the Tukey
        # outliers first, then average what is left. The median is kept alongside it because the two
        # differ a lot on this right-skewed data (BOP mean 26m raw vs median 8m) and a reader should
        # be able to see that rather than take one on faith.
        wick_stats[key] = round(sum(kept) / len(kept), 1) if kept else None
        wick_stats[key + '_median'] = round(st.median(kept), 1) if kept else None
        wick_drop[key] = n_out
        wick_stats[key + '_n'] = len(kept)
    res['wickFamily'] = dict(
        median=wick_stats,
        excluded=dict(never_reclaimed=n_never, zero_wick=n_zero,
                      outliers_tukey_1_5_iqr=wick_drop,
                      rule="operator 2026-08-28: no-wick days and crazy outliers are not averaged; "
                           "a zero wick still PRINTS 0 on the day's own row"),
        wend_definition="first bar to CLOSE back through the session open after the first extremity")
    # ---- (v15.77) THE SAME ROW, PER WEEKDAY — his seasonality ----------------------------------
    # Operator, 2026-09-08: "it compares friday with fridays in the past and mondays with past
    # mondays ... mondays and fridays have beginning and end of week characteristics which are
    # different than mid week characteristics. this is a type of seasonality." Every E field is
    # re-derived over the sessions of ONE weekday with the SAME statistic and the SAME exclusions
    # as the pooled block above (trimmed mean, zero-wick and never-reclaimed days out, Tukey fence),
    # so the weekday row and the pooled row can be read against each other. Each carries its n — a
    # weekday is ~55-60 sessions here, a fifth of the corpus, and the face must say so.
    # ⚠ The pooled block above is UNCHANGED: the ladder (hold rates by age) stays pooled — split five
    # ways its rungs go thin — and the panel reads it from the same place it always did.
    res['byWeekday'] = weekday_blocks(rows)
    # the mix, so a consumer can see a pooled corpus rather than discover it
    mix = collections.Counter(prov[d] for d in days)
    res['corpus']['sources'] = dict(mix)
    res['corpus']['incomplete_dropped'] = len(dropped)
    res['generatedAt'] = __import__('datetime').datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ')
    if out:
        with io.open(out, 'w', encoding='utf-8') as f:
            f.write(json.dumps(res, indent=1))
        print('wrote %s  (%d sessions, %s -> %s, sources: %s)'
              % (out, res['corpus']['sessions'], res['corpus']['first'], res['corpus']['last'],
                 ', '.join('%s x%d' % kv for kv in sorted(mix.items()))))
    else:
        print(json.dumps(res, indent=1))
    return res


WEEKDAYS = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri']


def expected_block(rows):
    """The E row over a set of session rows: expected + sequence + wickFamily, the pooled block's own
    statistics (trimmed mean, Tukey 1.5xIQR) and exclusions, with every n. Returns None under 8
    sessions - a fence needs eight points, and a row over fewer is a number, not an expectation."""
    if len(rows) < 8:
        return None
    drop = {}
    def tmean(xs, key=None):
        xs = [x for x in xs if x is not None]
        if not xs:
            return None
        kept, n_out = tukey_keep(xs)
        if key:
            drop[key] = n_out
        return round(sum(kept) / len(kept), 1) if kept else None
    med_true = lambda xs: round(st.median(xs), 1) if xs else None
    firsts = collections.Counter(r['first'] for r in rows)
    rng_all = [r['rng_pts'] for r in rows]
    out = dict(
        sessions=len(rows), first=min(r['day'] for r in rows), last=max(r['day'] for r in rows),
        sequence=dict(LOD_first=firsts['LOD'], HOD_first=firsts['HOD'],
                      pct_LOD_first=round(100 * firsts['LOD'] / len(rows))),
        expected=dict(
            took_min=tmean([r['took'] for r in rows], 'took'),
            gap_min=tmean([r['gap'] for r in rows], 'gap'),
            rng_pts=tmean(rng_all, 'rng_pts'),
            rng_usd=tmean([r['rng_usd'] for r in rows], 'rng_usd'),
            rng_p25=round(st.quantiles(rng_all, n=4)[0], 1),
            rng_p75=round(st.quantiles(rng_all, n=4)[2], 1),
            first_clock=tmean([min(r['hod_t'], r['lod_t']) for r in rows], 'first_clock'),
            second_clock=tmean([max(r['hod_t'], r['lod_t']) for r in rows], 'second_clock'),
            statistic='trimmed mean (Tukey 1.5xIQR outliers excluded), operator-specified 2026-08-28',
            outliers_excluded=drop,
            median_for_contrast=dict(
                took_min=med_true([r['took'] for r in rows]),
                gap_min=med_true([r['gap'] for r in rows]),
                rng_pts=med_true(rng_all),
                first_clock=med_true([min(r['hod_t'], r['lod_t']) for r in rows]),
                second_clock=med_true([max(r['hod_t'], r['lod_t']) for r in rows]))))
    wick_rows = [r for r in rows if r.get('reclaimed')]
    n_never = len(rows) - len(wick_rows)
    n_zero = sum(1 for r in wick_rows if r['wick'] == 0)
    pool = [r for r in wick_rows if r['wick'] not in (None, 0)]
    wick_stats, wick_drop = {}, {}
    for key in ('wick', 'bop', 'mud', 'wick_pct', 'wick_pts'):
        kept, n_out = tukey_keep([r[key] for r in pool])
        wick_stats[key] = round(sum(kept) / len(kept), 1) if kept else None
        wick_stats[key + '_median'] = round(st.median(kept), 1) if kept else None
        wick_drop[key] = n_out
        wick_stats[key + '_n'] = len(kept)
    out['wickFamily'] = dict(median=wick_stats,
                             excluded=dict(never_reclaimed=n_never, zero_wick=n_zero,
                                           outliers_tukey_1_5_iqr=wick_drop))
    # (v15.77) THE RECENT SIX — his tool's "Last 6 per Weekday": the colour of the last six sessions
    # of this weekday (RTH close vs RTH open), a count, never a rate, and the days it stands on.
    last = sorted(rows, key=lambda r: r['day'])[-6:]
    g = sum(1 for r in last if r.get('green'))
    out['recent'] = dict(n=len(last), green=g, red=len(last) - g,
                         days=[r['day'] for r in last],
                         rng_pts=round(sum(r['rng_pts'] for r in last) / len(last), 1) if last else None)
    return out


def weekday_blocks(rows):
    """One expected_block per weekday, keyed Mon..Fri (a weekend session, if a corpus ever carried one,
    is left out rather than filed under a weekday it is not)."""
    import datetime as _dt
    by = collections.defaultdict(list)
    for r in rows:
        try:
            wd = _dt.date.fromisoformat(r['day']).weekday()
        except ValueError:
            continue
        if wd < 5:
            by[WEEKDAYS[wd]].append(r)
    out = {}
    for name in WEEKDAYS:
        blk = expected_block(by.get(name, []))
        if blk:
            out[name] = blk
    return out


def market_sources(market):
    """The vendor corpus (if present) FIRST, then every Yahoo daily for this market."""
    src = []
    if market == 'ES':
        # ⚠ THE CORPUS HAS ARRIVED UNDER MORE THAN ONE NAME. The README calls it
        # EPM26-1min.csv.gz; what actually reached GitHub on 2026-08-28 was the operator's raw
        # export, "ES TestingData.txt" - same columns, 406,155 rows, uncompressed. Looking for one
        # spelling and reporting "no sources" would have been the fourth time in this project that a
        # file was declared absent because the search was too narrow. Try every name it has worn.
        for c in ('data/es-1min/EPM26-1min.csv.gz', 'data/es-1min/EPM26-1min.csv',
                  'data/es-1min/ES TestingData.txt', 'data/es-1min/ES_TestingData.txt'):
            if os.path.exists(c):
                src.append(c)
                break
        if not src:
            import glob as _g
            for c in sorted(_g.glob('data/es-1min/*.txt') + _g.glob('data/es-1min/*.csv*')):
                if not c.endswith('BASERATES.json'):
                    src.append(c)
                    break
    src += sorted(glob.glob(os.path.join('data/futures', market, '*.csv')))
    return src


if __name__ == '__main__':
    a = sys.argv[1:]
    if a and a[0] == '--market':
        mk = a[1] if len(a) > 1 else 'ES'
        outp = a[3] if len(a) > 3 and a[2] == '--out' else (
            'data/es-1min/BASERATES.json' if mk == 'ES' else 'data/futures/%s/BASERATES.json' % mk)
        srcs = market_sources(mk)
        if not srcs:
            print('no sources for %s - looked for the vendor corpus and data/futures/%s/*.csv' % (mk, mk),
                  file=sys.stderr)
            sys.exit(1)
        print('sources: %s' % ', '.join(srcs))
        sys.exit(0 if main(srcs, outp, market=mk) else 1)
    main(a or ['/tmp/es.csv'])
