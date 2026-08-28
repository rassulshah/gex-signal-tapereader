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
                if not (RTH_A <= sec <= RTH_B):
                    continue
                ses[d].append((sec, float(x['High']), float(x['Low']), float(x['Close'])))
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

def survival(bars, low_side):
    """(stood_minutes, survived_to_close) for every running-extreme candidate in one session."""
    out, best, since = [], None, None
    for sec, hi, lo, _ in bars:
        v = lo if low_side else hi
        new = best is None or (v < best if low_side else v > best)
        if new:
            if best is not None:
                out.append(((sec - since)//60, False))
            best, since = v, sec
    out.append(((bars[-1][0] - since)//60, True))
    return out

def main(paths, out=None):
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
        b = ses[d]
        hi = max(b, key=lambda r: r[1]); lo = min(b, key=lambda r: r[2])
        hod_t, lod_t = hi[0], lo[0]
        first_low = lod_t < hod_t
        rng = hi[1] - lo[2]
        rows.append(dict(day=d, hod_t=hod_t, lod_t=lod_t, first='LOD' if first_low else 'HOD',
                         took=(min(hod_t, lod_t)-RTH_A)//60, gap=abs(hod_t-lod_t)//60,
                         rng_pts=round(rng, 2), rng_usd=round(rng*PT_USD, 2)))
        surv_lo += survival(b, True); surv_hi += survival(b, False)

    def ladder(sv):
        out = {}
        for w in WINDOWS:
            elig = [s for s in sv if s[0] >= w]
            hit = sum(1 for s in elig if s[1])
            out[w] = dict(n=len(elig), held=hit,
                          rate=round(100*hit/len(elig)) if elig else None)
        return out

    med = lambda xs: round(st.median(xs), 1) if xs else None
    firsts = collections.Counter(r['first'] for r in rows)
    res = dict(
        corpus=dict(sessions=len(days), first=days[0], last=days[-1],
                    min_bars=MIN_BARS, rth='08:30-15:00 CT', pt_usd=PT_USD),
        sequence=dict(LOD_first=firsts['LOD'], HOD_first=firsts['HOD'],
                      pct_LOD_first=round(100*firsts['LOD']/len(rows))),
        expected=dict(
            took_min=med([r['took'] for r in rows]),
            gap_min=med([r['gap'] for r in rows]),
            rng_pts=med([r['rng_pts'] for r in rows]),
            rng_usd=med([r['rng_usd'] for r in rows]),
            rng_p25=round(st.quantiles([r['rng_pts'] for r in rows], n=4)[0], 1),
            rng_p75=round(st.quantiles([r['rng_pts'] for r in rows], n=4)[2], 1),
            first_clock=med([min(r['hod_t'], r['lod_t']) for r in rows]),
            second_clock=med([max(r['hod_t'], r['lod_t']) for r in rows])),
        ladder=dict(low=ladder(surv_lo), high=ladder(surv_hi),
                    both=ladder(surv_lo + surv_hi)),
    )
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


def market_sources(market):
    """The vendor corpus (if present) FIRST, then every Yahoo daily for this market."""
    src = []
    if market == 'ES':
        for c in ('data/es-1min/EPM26-1min.csv.gz', 'data/es-1min/EPM26-1min.csv'):
            if os.path.exists(c):
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
        sys.exit(0 if main(srcs, outp) else 1)
    main(a or ['/tmp/es.csv'])
