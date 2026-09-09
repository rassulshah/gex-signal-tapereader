#!/usr/bin/env python3
"""
Renders design/mockup-data-tab.html — the 🗄 DATA tab, proposed 2026-09-09 ("i want you to build a data tab and place it
before the analysis tab … a snapshot summary of the data we currently have, what we need for the studies, recommendations
and more. If there is missing data that we can obtain from yahoo, it should mention that"). Two variants on one page:
  A — the SOURCES table first, then the record, the studies' needs, the gaps (dense, one screen)
  B — the COVERAGE CALENDAR first (a cell per session since 08-17), then the same sections
Nothing here is illustrative: every number is read from the repo (the day files, the corpora, the registry, the logs) or
from his browser stores as probed on 2026-09-09 (localStorage 3.4 MB / 69 keys; IndexedDB snaps 2,699 · feat 63,597 · defl
149 · tape 610). Where the live panel would read a store, the mockup prints what the probe returned.
"""
import json, io, html, glob, os, sys, collections, csv
sys.path.insert(0, 'tools')
from importlib import import_module
CSS = import_module('mockup-from-studies').CSS
E = html.escape

# ---- the repo's numbers ----
STU = json.load(io.open('learning/studies.json', encoding='utf-8'))
studies = [x for sj in STU['subjects'] for ss in sj['subsections'] for x in ss['studies']]
by_status = collections.Counter(x['status'] for x in studies)
by_corpus = collections.Counter()
for x in studies:
    c = (x.get('corpus') or '?')
    key = ('tap record' if 'tap record' in c else 'price corpus (284d)' if '284d' in c else 'book (day files)' if 'book' in c else 'live / API' if 'API' in c or 'live' in c else c)
    by_corpus[key] += 1
REG = json.load(io.open('learning/register.json', encoding='utf-8'))
logs = sorted(f for f in glob.glob('learning/log/*.json') if 'catch-up' not in f)
LOG = json.load(io.open(logs[-1], encoding='utf-8'))
BR = json.load(io.open('data/es-1min/BASERATES.json', encoding='utf-8'))
BRN = json.load(io.open('data/futures/NQ/BASERATES.json', encoding='utf-8'))
SWP = json.load(io.open('data/es-1min/SWEEPS.json', encoding='utf-8'))
SWB = json.load(io.open('data/es-1min/SWEEPS-BOOK.json', encoding='utf-8'))
EX = json.load(io.open('learning/deflections/examples.json', encoding='utf-8'))
legs = sum(len(e['legs']) for e in EX['examples'])
REC = json.load(io.open('learning/recommendations.json', encoding='utf-8'))
ITEMS = json.load(io.open('learning/items.json', encoding='utf-8'))

days = sorted(glob.glob('data/20??-??-??.json'))
DAYS = []
for p in days:
    d = json.load(io.open(p, encoding='utf-8'))
    fb = d.get('futBars') or {}
    es = fb.get('ES') or {}; nq = fb.get('NQ') or {}
    snaps = d.get('snaps'); ns = sum(len(v) for v in snaps.values()) if isinstance(snaps, dict) else (len(snaps) if isinstance(snaps, list) else 0)
    DAYS.append(dict(day=p[5:15], mb=os.path.getsize(p)/1e6, snaps=ns, nev=len(d.get('nodeEvents') or []),
                     es=es.get('n'), esFull=bool(es.get('full')), nq=nq.get('n'), nqFull=bool(nq.get('full')),
                     feat=bool(d.get('feat')), defl=sum(len(v) for v in (d.get('defl') or {}).values()) if isinstance(d.get('defl'), dict) else 0))
CSV = {mk: {os.path.basename(p)[:10]: sum(1 for _ in open(p)) - 1 for p in glob.glob('data/futures/%s/20*.csv' % mk)} for mk in ('ES', 'NQ', 'GC', 'CL')}
LOGDAYS = {os.path.basename(p)[:10] for p in logs}
TAPE_ON_GITHUB = {'2026-09-07', '2026-09-08'}   # git ls-tree origin/main data/tape/ on 2026-09-09 (the clone has none — the sync pushed them)

# ---- his browser, as probed 2026-09-09 (the live panel reads these itself) ----
LS = dict(totalKB=3417, keys=69, big=[('gpts_recorder_v7', 1618), ('gpts_futbars_v1', 482), ('gpts_nodeevents_v1', 419), ('gpts_slices_v7', 316), ('gpts_nodehist_v1', 79)])
IDB = dict(snaps=2699, feat=63597, defl=149, tape=610, kv=16)
FUT = [('ES', 4265, True, 8), ('NQ', 1439, False, 8), ('GC', 1440, False, 8), ('CL', 1439, False, 8)]   # rows, full night?, age min


def sec(n, title, dc, body, ct=''):
    return ('<div class="sec"><div class="sech"><span class="n">%s</span><span class="t">%s</span><span class="dc">%s</span>'
            '<span class="ct">%s</span></div><div class="secb">%s</div></div>' % (n, E(title), E(dc), E(ct), body))


def dot(state):
    return {'ok': '<span class="gr">●</span>', 'warn': '<span class="am">●</span>', 'bad': '<span class="rd">●</span>', 'off': '<span class="dm">○</span>'}[state]


def sources():
    h = ['<table><tr><th></th><th>source</th><th>what</th><th class="r">coverage</th><th class="r">fresh</th><th>feeds</th></tr>']
    rows = [
        ('ok', 'Skylit Atlas', 'the tape — SPXW 271 · SPY 112 · QQQ 104 · VIX 70 strikes per 3-min bar; the 0DTE chain (F-22 guard: 0 rejects today)', '127 bars today', 'live', '⓪ the King, the ladder, the nodes, the taps, the tape store'),
        ('ok', 'InsiderFinance', 'the chains — SPX / SPY / QQQ open interest × gamma, three windows (dte0 · toFri · all)', '3 windows', '4 min', '① FRAME: CW0 / PW0 / FLIP, the walls, the EM band'),
        ('ok', 'Yahoo · ES=F 1-min', 'the whole Globex day (ONH / ONL · Asia · London · the session)', '4,265 rows · 5 days', '8 min', '⓪a the candle, the A row, the levels; the corpus tap'),
        ('warn', 'Yahoo · NQ=F 1-min', 'RTH only until companion v1.19 installs — then the whole night', '1,439 rows · 5 days', '8 min', 'the NQ chart’s ⓪a (v15.88); the NQ corpus'),
        ('ok', 'Yahoo · GC=F · CL=F 1-min', 'RTH only (nothing reads their night)', '1,440 · 1,439 rows', '8 min', 'the corpus (D5)'),
        ('off', 'Yahoo · ES / NQ 5-min', 'one month of RTH 5-minute bars — the prior week (companion v1.19: not fetched yet)', '—', '—', 'PWH / PWL / WPOC on the candle'),
        ('ok', 'Yahoo · ^VIX daily', 'two years of closes', '503 rows', '11 h', 'the far-side table (VIX bucket)'),
        ('ok', 'GitHub raw · the repo', 'studies · results · register · SWEEPS · BASERATES ES + NQ · the log · examples · recommendations · items', 'asOf 2026-09-08', '9 min', 'Analysis · Testing · Learn · Rec · Open Items'),
        ('ok', 'The nightly (his machine)', 'last run %s %s CT — %d sessions · %d episodes · %d deflection events; %s' % (LOG['date'], LOG['ranAt'][11:16], LOG['sessions'], LOG['episodes'], LOG['deflEvents'], 'H1–H10 judged'), 'log %s' % LOG['date'], '1 run', 'results.json → the registry; the Learn rules’ verdicts; Rec'),
    ]
    for st, name, what, cov, fresh, feeds in rows:
        h.append('<tr><td>%s</td><td><b>%s</b></td><td style="white-space:normal">%s</td><td class="r">%s</td><td class="r %s">%s</td><td class="dm" style="white-space:normal">%s</td></tr>'
                 % (dot(st), E(name), E(what), E(cov), 'gr' if fresh in ('live',) else 'dm', E(fresh), E(feeds)))
    h.append('</table>')
    return ''.join(h)


def record_here():
    q = 10 * 1024
    used = LS['totalKB']; pct = used / q
    bar = ('<div style="height:5px;background:#1e2530;border-radius:3px;overflow:hidden;margin:2px 0 3px"><div style="width:%d%%;height:100%%;background:%s"></div></div>'
           % (round(100 * pct), '#2ec27e' if pct < .6 else '#f2b45a'))
    big = ' · '.join('%s %s KB' % (k.replace('gpts_', '').replace('_v7', '').replace('_v1', ''), v) for k, v in LS['big'])
    h = ['<table style="table-layout:fixed"><colgroup><col style="width:88px"><col style="width:74px"><col><col style="width:58px"><col style="width:150px"></colgroup><tr><th>store</th><th class="r">size</th><th>holds</th><th>since</th><th>note</th></tr>']
    h.append('<tr><td><b>localStorage</b></td><td class="r" style="white-space:normal">%s / 10 MB · %d keys</td><td style="white-space:normal">%s</td><td>today</td><td class="dm" style="white-space:normal">the quota that collapsed the feature record once (F-10); bounded writes since</td></tr>' % (round(used / 1024, 1), LS['keys'], E(big)))
    h.append('<tr><td colspan="5" style="padding:0 4px">%s</td></tr>' % bar)
    h.append('<tr><td><b>IndexedDB · snaps</b></td><td class="r">{:,}</td><td style="white-space:normal">one row per 3-min bar per book — the ranked lists, the King, the levels, the projection</td><td>08-25</td><td class="dm" style="white-space:normal">the window-proof source the circles were read from</td></tr>'.format(IDB['snaps']))
    h.append('<tr><td><b>IndexedDB · feat</b></td><td class="r">{:,}</td><td style="white-space:normal">the feature records with outcomes (the archive; LS is a window)</td><td>08-2x</td><td class="dm" style="white-space:normal">exported as the day file’s feat (v15.82)</td></tr>'.format(IDB['feat']))
    h.append('<tr><td><b>IndexedDB · defl</b></td><td class="r">%d</td><td style="white-space:normal">the deflection ledger — stamped taps with CONTINUED / STALLED</td><td>09-04</td><td class="dm" style="white-space:normal">reads ±0.50 SPY wobbles; the tap record (v15.89) replaces it at his scale</td></tr>' % IDB['defl'])
    h.append('<tr><td><b>IndexedDB · tape</b></td><td class="r">%d bars</td><td style="white-space:normal">the whole book per bar (SPXW velocity, SPY / QQQ / VIX ladders)</td><td>09-04</td><td class="dm" style="white-space:normal">written to data/tape/&lt;day&gt;/ at the close — 2 days on GitHub (09-07, 09-08)</td></tr>' % IDB['tape'])
    h.append('</table>')
    return ''.join(h)


def coverage_table():
    h = ['<table><tr><th>session</th><th class="r">day file</th><th class="r">snaps</th><th class="r">node ev.</th><th class="r">ES bars</th><th class="r">NQ bars</th><th class="r">corpus CSV</th><th class="r">tape</th><th class="r">nightly</th></tr>']
    for d in DAYS[-9:]:
        k = d['day']; es = CSV['ES'].get(k); nq = CSV['NQ'].get(k)
        def bars(n, full):
            if not n: return '<span class="rd">—</span>'
            return '%s%s' % ('{:,}'.format(n), ' <span class="gr">night</span>' if full else ' <span class="dm">RTH</span>')
        def csvc(n):
            if n is None: return '<span class="dm">—</span>'
            return ('<span class="gr">%d ✓</span>' % n) if n >= 386 else ('<span class="am">%d ⚠</span>' % n)
        h.append('<tr><td><b>%s</b></td><td class="r">%.1f MB</td><td class="r">%d</td><td class="r">%d</td><td class="r">%s</td><td class="r">%s</td><td class="r">%s</td><td class="r">%s</td><td class="r">%s</td></tr>'
                 % (k, d['mb'], d['snaps'], d['nev'], bars(d['es'], d['esFull']), bars(d['nq'], d['nqFull']), csvc(es), ('<span class="gr">✓</span>' if k in TAPE_ON_GITHUB else '<span class="dm">—</span>'), ('<span class="gr">✓</span>' if k in LOGDAYS else '<span class="dm">—</span>')))
    h.append('</table>')
    h.append('<div class="note">%d day files since %s (08-29 / 08-30 are weekend files with no bars — the old auto-export; kept). The corpora: ES %d sessions (%d vendor + %d Yahoo, %s → %s) · NQ %d (%d + %d) · the sweep corpus %d · the book corpus %d sessions · the Learn corpus %d examples, %d legs.</div>'
             % (len(DAYS), DAYS[0]['day'], BR['corpus']['sessions'], 284, BR['corpus']['sessions'] - 284, BR['corpus']['first'], BR['corpus']['last'], BRN['corpus']['sessions'], 188, BRN['corpus']['sessions'] - 188, SWP['corpus']['sessions'], SWB['corpus'].get('sessions', 0), len(EX['examples']), legs))
    return ''.join(h)


def coverage_calendar():
    # a cell per session since 08-17: five signals stacked as tiny squares
    h = ['<div style="display:flex;gap:3px;flex-wrap:wrap;align-items:flex-end">']
    for d in DAYS:
        k = d['day']; es = CSV['ES'].get(k)
        sig = [('day file', True), ('ES night', bool(d['esFull'])), ('NQ night', bool(d['nqFull'])), ('CSV', es is not None and es >= 386), ('tape', k in TAPE_ON_GITHUB), ('nightly', k in LOGDAYS)]
        cells = ''.join('<div title="%s" style="width:9px;height:4px;margin-top:1px;background:%s;border-radius:1px"></div>' % (E(n), '#2ec27e' if v else '#2a3140') for n, v in sig)
        h.append('<div style="text-align:center"><div style="font-size:5.6px;color:var(--g-dim)">%s</div>%s</div>' % (k[5:], cells))
    h.append('</div><div class="note">rows, top to bottom: the day file · ES night · NQ night · the corpus CSV complete · the tape on disk · the nightly’s log. Green = have. The NQ night and the weekly bars begin the day companion v1.19 is installed; the tape on disk since 09-07 (v15.66); the corpus CSVs since 08-24 (the courier).</div>')
    return ''.join(h)


def needs():
    open_tap = sum(1 for x in studies if x['status'] == 'OPEN' and 'tap record' in (x.get('corpus') or ''))
    h = ['<table><tr><th>the studies wait on</th><th class="r">studies</th><th>what unlocks them</th><th class="r">have</th><th class="r">need</th></tr>']
    rows = [
        ('THE TAP RECORD (v15.89, next)', open_tap, 'one row per tap of an exported line + two controls (design/TAP-RECORD.md)', '0 taps', 'the recorder built, then n ≥ 15 per cell'),
        ('the price corpus (284 sessions)', by_corpus.get('price corpus (284d)', 0), 'read — the sweep tables (F-14 / F-23); grows by one session a night', '%d' % SWP['corpus']['sessions'], 'H7 re-reads after 2026-08-21: %d of 60' % 0),
        ('the book corpus (day files)', by_corpus.get('book (day files)', 0), 'the SPY 3-min book per bar; sweeps at a node vs not', '%d sessions' % SWB['corpus'].get('sessions', 0), 'H6: sweep-at-node events %s of 40' % ((LOG.get('hypotheses') or [{}])[0].get('n', '?') if False else '2')),
        ('the Learn gauge (blind reads)', 1, 'blind calls on screenshots before the answer', '0 blind', '5 to start, 15 to trust'),
        ('the King roll (H8 / H9)', 2, 'kingRoll stamped on every tap since v15.72', '%d taps' % 0, '30 per class'),
        ('the deflection ledger (F-19 / F-21)', sum(1 for x in studies if 'defl' in (x.get('corpus') or '')), 'turn / resume outcomes per tap', '%d events' % LOG['deflEvents'], 'the tap record at his scale'),
    ]
    for name, n, what, have, need in rows:
        h.append('<tr><td><b>%s</b></td><td class="r">%d</td><td style="white-space:normal">%s</td><td class="r">%s</td><td class="r dm">%s</td></tr>' % (E(name), n, E(what), E(have), E(need)))
    h.append('</table>')
    h.append('<div class="note">%d studies: %s. The Rec tab holds %d proposals (%d implemented); the Open Items file %d items.</div>'
             % (len(studies), ' · '.join('%s %d' % (k, v) for k, v in sorted(by_status.items(), key=lambda kv: -kv[1])), len(REC['rows']), REC['counts']['implemented'], len(ITEMS['items'])))
    return ''.join(h)


def gaps():
    h = ['<table><tr><th></th><th>gap</th><th>what it costs</th><th>from Yahoo?</th><th>the fix</th></tr>']
    rows = [
        ('warn', 'GC / CL 2026-09-08 incomplete (379 of 393 bars)', 'two sessions dropped from D5', 'yes — the next day’s 5-day window carries them', 'automatic: tonight’s append completes them'),
        ('warn', 'the NQ night — none before v1.19', 'ONH / ONL / Asia / London on the NQ chart from 09-09 only; E006 read without them', 'yes — NQ=F 1-min, whole day (v1.19)', 'install companion v1.19; the first fetch back-fills 5 days'),
        ('warn', 'the prior week (WH / WL / WPOC) — never fetched', 'three of his key levels never drawn', 'yes — ES=F / NQ=F 5-min, 1 month (v1.19)', 'install companion v1.19'),
        ('bad', 'DAILY bars — none held', 'his seasonality charts (range by weekday vs the 10-week average, red / green counts) stand on 57 sessions per weekday; gap statistics unmeasured', 'YES — ES=F / NQ=F 1-day, full history (years)', 'RECOMMENDED: a daily courier (one fetch a day, ~20 KB a year)'),
        ('bad', 'HOURLY bars — none held', 'the HOD / LOD clock could stand on ~500 sessions (2 years) instead of 295; the E row per weekday on 100+ each', 'YES — ES=F 1-hour, 730 days', 'RECOMMENDED: a one-time backfill into the corpus (hour resolution — the ladder, not the wick family)'),
        ('bad', '^VIX1D — the one-day implied move', 'the EM band is built from the ATM straddle read live; a daily implied series would let the band be backtested on the corpus', 'yes — ^VIX1D daily', 'OPTIONAL: extend the VIX courier'),
        ('off', 'the option chains’ history', 'every book study is forward-only (11–13 sessions)', 'no — Skylit / IF only, live capture', 'keep capturing; the tape on disk since 09-07'),
        ('off', 'QQQ node dollars', 'E006 read by %King only', 'no — Skylit', 'the tap record carries QQQ $ from the ladder’s $K'),
        ('off', 'bid / ask volume (absorption)', 'the footprint he named', 'no — IRT / CQG', 'the RTX SDK path (design notes 2026-09-07)'),
    ]
    for st, gap, cost, yahoo, fix in rows:
        h.append('<tr><td>%s</td><td style="white-space:normal"><b>%s</b></td><td style="white-space:normal">%s</td><td style="white-space:normal" class="%s">%s</td><td style="white-space:normal" class="dm">%s</td></tr>'
                 % (dot(st), E(gap), E(cost), 'gr' if yahoo.lower().startswith('yes') else 'dm', E(yahoo), E(fix)))
    h.append('</table>')
    return ''.join(h)


def clock():
    return ('<div class="sc"><span class="id">15:01</span><span class="qq">the panel writes data/2026-09-08.json (10.8 MB) + data/tape/2026-09-08/</span><span class="cl gr">SAVED</span></div>'
            '<div class="sc"><span class="id">15:03</span><span class="qq">the sync task pushes it</span><span class="cl gr">PUSHED</span></div>'
            '<div class="sc"><span class="id">15:05</span><span class="qq">the nightly runs on his machine — the log, the registry, the Learn verdicts, Rec, the corpus append</span><span class="cl gr">RAN</span></div>'
            '<div class="sc"><span class="id">21:35</span><span class="qq">re-run after the v15.87 install (the tick’s check) — 295 ES sessions, NQ 195</span><span class="cl am">RE-RAN</span></div>'
            '<div class="sc"><span class="id">09:1x</span><span class="qq">the panel fetched the registry, the log, SWEEPS, BASERATES (ES + NQ) from GitHub</span><span class="cl gr">FRESH</span></div>')


def panel(variant):
    h = ['<div class="pan"><div class="tabs"><span>Dashboard</span><span class="on">🗄 Data</span><span>📊 Analysis</span><span>🧪 Testing</span><span>📚 Learn</span><span>💡 Rec</span><span>⚙</span><span>🗺</span><span>📌</span></div>']
    h.append('<div class="hd"><b>THE DATA — what we hold, what the studies need, what is missing</b> · 5 sources live · %d day files · ES %d / NQ %d sessions · %s studies · the nightly %s</div>'
             % (len(DAYS), BR['corpus']['sessions'], BRN['corpus']['sessions'], len(studies), LOG['date']))
    if variant == 'A':
        h.append(sec('①', 'THE SOURCES — live', 'SKYLIT · IF · YAHOO · GITHUB · THE NIGHTLY', sources(), 'green = fresh · amber = partial · grey = not yet'))
        h.append(sec('②', 'THE RECORD ON THIS MACHINE', 'localStorage · IndexedDB', record_here()))
        h.append(sec('③', 'THE RECORD ON GITHUB — coverage by session', 'day files · bars · corpus · tape · nightly', coverage_table(), 'last 9 sessions'))
    else:
        h.append(sec('①', 'COVERAGE — every session since 08-17', 'a cell per day', coverage_calendar()))
        h.append(sec('②', 'THE SOURCES — live', 'SKYLIT · IF · YAHOO · GITHUB · THE NIGHTLY', sources()))
        h.append(sec('③', 'THE RECORD ON THIS MACHINE', 'localStorage · IndexedDB', record_here()))
    h.append(sec('④', 'WHAT THE STUDIES ARE WAITING FOR', 'the registry, by corpus', needs()))
    h.append(sec('⑤', 'THE GAPS — AND WHAT YAHOO CAN FILL', 'have · partial · missing', gaps(), 'green = obtainable from Yahoo'))
    h.append(sec('⑥', 'THE PIPELINE’S CLOCK — today', 'export → sync → nightly → fetch', clock()))
    h.append('<div class="foot"><span>🗄 DATA</span><span>·</span><span>read-only: every number is a store or a file; nothing here is a claim</span><span>·</span><span>fetched 09:1x · reloads on the 10-minute check</span></div></div>')
    return ''.join(h)


def page():
    out = ['<!doctype html><meta charset="utf-8"><title>Mockup — the 🗄 DATA tab (proposed 2026-09-09)</title><style>%s .stage{padding:10px} .scaler{--sh:1700px}</style>' % CSS]
    out.append('<div class="wrap"><h1>The 🗄 DATA tab — two mockups</h1><p class="lede">His ask, 2026-09-09: <i>"a snapshot summary of the data we currently have, what we need for the studies, recommendations and more. If there is missing data that we can obtain from yahoo, it should mention that."</i> The tab sits before Analysis. Every number below is the repo’s or his browser’s as of 2026-09-09; nothing is illustrative.</p>')
    out.append('<section><h2><span>A</span> the sources first — one screen, tables</h2><div class="stage"><div class="scaler">%s</div></div></section>' % panel('A'))
    out.append('<section><h2><span>B</span> the coverage calendar first — a cell per session</h2><div class="stage"><div class="scaler">%s</div></div></section>' % panel('B'))
    out.append('<section><h2>what is the same in both</h2><div class="pros"><div><b>read-only, no claims</b>The tab reads stores and files and prints counts, ages and dates. It never rates anything — that is Analysis. The one judgement it makes is a colour: green have, amber partial, red missing.</div><div><b>the Yahoo column</b>Every gap says whether Yahoo can fill it (green) or cannot (grey — Skylit, IF, IRT), and what the fix is. Two are recommended now: DAILY bars (years) for his seasonality charts, and HOURLY bars (2 years) to put the HOD / LOD clock on ~500 sessions.</div><div><b>what the studies wait on</b>195 studies by the corpus they need, with have / need beside each — the tap record (86 studies) is the biggest line and it is the next build.</div><div><b>the pipeline’s clock</b>Today’s export → sync → nightly → fetch with the time each happened, so a stale step is visible here as well as on the day line.</div></div></section></div>')
    return ''.join(out)


if __name__ == '__main__':
    io.open('design/mockup-data-tab.html', 'w', encoding='utf-8').write(page())
    print('wrote design/mockup-data-tab.html')
