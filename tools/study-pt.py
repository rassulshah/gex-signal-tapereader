#!/usr/bin/env python3
"""
PT — THE PROFIT-TAKING LEG, as the operator defines it (2026-08-29):

  "lets say the market make a lod and then an hod. pt is hod to the lowest point prior to the close
   that doesn't take out the lod, and pt time is the time that it took."

So PT runs from the SECOND extreme to the most adverse point after it, before the close.

⚠⚠ THIS IS NOT WHAT study-secondleg.py MEASURED. That one took the distance from the second extreme
to the CLOSING PRICE. PT is the full EXCURSION — how far price actually travelled back, whether or
not it was still there at the bell. On a day that retraces 20 points and closes back at the highs,
the close-based number is ~0 and PT is 20. PT is the one that answers "was there a trade here".

⚠ "that doesn't take out the lod" is guaranteed by construction, and it is worth writing down why:
if the low after the HOD had broken the LOD, THAT would be the day's LOD and the HOD would not be
the second extreme. The clause describes the invariant rather than adding a filter — but the code
asserts it anyway, because a silent violation would mean the extremes were mislabelled.

⚠ Corpus times are ET (24h globex file). RTH = 09:30-15:59 ET = 08:30-15:00 CT. ES = $50/pt.
"""
import csv, io, collections, statistics

PT_USD = 50.0
rows = collections.defaultdict(list)
r = csv.DictReader(io.open('data/es-1min/ES TestingData.txt', encoding='utf-8'))
for row in r:
    v = row['Date'].strip().strip('"')
    if ' ' not in v: continue
    d, t = v.split(' ', 1)
    hh, mm = t.split(':')[0:2]
    m = int(hh)*60 + int(mm)
    if m < 9*60+30 or m > 15*60+59: continue
    try: rows[d].append((m, float(row['High']), float(row['Low']), float(row['Close'])))
    except Exception: continue

legs = []; violations = 0
for d, bars in rows.items():
    if len(bars) < 300: continue
    bars.sort()
    hi = max(bars, key=lambda b: b[1]); lo = min(bars, key=lambda b: b[2])
    hiT, loT, hiP, loP = hi[0], lo[0], hi[1], lo[2]
    if hiT == loT: continue
    secondIsHOD = hiT > loT
    secT  = hiT if secondIsHOD else loT
    secP  = hiP if secondIsHOD else loP
    after = [b for b in bars if b[0] >= secT]
    if len(after) < 2: continue
    if secondIsHOD:
        adv = min(after, key=lambda b: b[2]); advT, advP = adv[0], adv[2]
        if advP < loP - 1e-9: violations += 1        # would mean the LOD was mislabelled
    else:
        adv = max(after, key=lambda b: b[1]); advT, advP = adv[0], adv[1]
        if advP > hiP + 1e-9: violations += 1
    legs.append({'d': d, 'second': 'HOD' if secondIsHOD else 'LOD',
                 'secT': secT, 'ptMin': advT - secT, 'ptPts': abs(secP - advP),
                 'toClose': bars[-1][0] - secT, 'rng': hiP - loP})

def q(v, p):
    v = sorted(v); return v[min(len(v)-1, max(0, int(round(p*(len(v)-1)))))] if v else 0
def hm(m): return '%dh%02d' % (m//60, m%60)
def clk(m):
    h=m//60; ap='pm' if h>=12 else 'am'; return '%d:%02d%s' % (h%12 or 12, m%60, ap)

print('sessions: %d   invariant violations: %d  (must be 0 — a violation means the extremes were mislabelled)'
      % (len(legs), violations))
for lab, sel in (('ALL', legs),
                 ('second = HOD  (PT is the pullback DOWN)', [l for l in legs if l['second']=='HOD']),
                 ('second = LOD  (PT is the bounce UP)',     [l for l in legs if l['second']=='LOD'])):
    if not sel: continue
    T=[l['ptMin'] for l in sel]; P=[l['ptPts'] for l in sel]
    print('\n--- %s   n=%d ---' % (lab, len(sel)))
    print('  PT TIME   median %s   middle half %s .. %s' % (hm(q(T,.5)), hm(q(T,.25)), hm(q(T,.75))))
    print('  PT DIST   median %.1f pts ($%s)   middle half %.1f .. %.1f pts'
          % (q(P,.5), format(int(q(P,.5)*PT_USD), ','), q(P,.25), q(P,.75)))
    print('  PT as %% of the day range   median %d%%' % round(100*statistics.median([l['ptPts']/l['rng'] for l in sel if l['rng']>0])))

print('\n--- how often PT clears a size (ALL) ---')
for thr in (5,10,15,20,30,40):
    n = sum(1 for l in legs if l['ptPts'] >= thr)
    print('   >= %2d pts ($%-6s)  %5.1f%%  (n=%d)' % (thr, format(int(thr*PT_USD),','), 100.0*n/len(legs), n))

# what the close-based number would have said instead
import statistics as st
print('\n--- PT vs the close-based figure study-secondleg.py reported ---')
print('   PT median            %.1f pts' % q([l['ptPts'] for l in legs], .5))
print('   close-based median   12.5 pts   <- understates the excursion by ~%.0f%%'
      % (100*(q([l['ptPts'] for l in legs], .5) - 12.5)/12.5))

# ============================================================================================
# THE PT LEG'S OWN WICK FAMILY — mirroring the operator's first-extreme definitions
#
#   WICK%  = |open - extreme| / day range      ->  PTWick% = PT distance / day range
#   MUD    = reclaim -> second extreme          ->  PTMUD   = PT extreme -> the close
#
# ⚠⚠ PTWICK IS DELIBERATELY NOT DERIVED HERE. WICK is "the session open to the bar that RECLAIMS
# the open" — it needs an ANCHOR that the move started from and later took back. The PT leg's anchor
# is the second extreme itself, so "reclaim" would mean price returning TO that extreme, which is a
# different event from anything the first-extreme family measures. Inventing a definition and
# printing it beside measured columns is precisely what made the wick family untrustworthy in
# v14.57; the operator supplied those definitions himself when asked. Ask again.
# ============================================================================================
print('\n' + '='*70)
print('THE PT LEG WICK FAMILY  (PTWick% and PTMUD only — PTWICK needs a definition)')
print('='*70)
wpct=[]; mud=[]
for d, bars in rows.items():
    if len(bars) < 300: continue
    bars.sort()
    hi = max(bars, key=lambda b: b[1]); lo = min(bars, key=lambda b: b[2])
    hiT, loT, hiP, loP = hi[0], lo[0], hi[1], lo[2]
    if hiT == loT: continue
    secondIsHOD = hiT > loT
    secT = hiT if secondIsHOD else loT
    secP = hiP if secondIsHOD else loP
    after = [b for b in bars if b[0] >= secT]
    if len(after) < 2: continue
    if secondIsHOD: adv = min(after, key=lambda b: b[2]); advT, advP = adv[0], adv[2]
    else:           adv = max(after, key=lambda b: b[1]); advT, advP = adv[0], adv[1]
    rng = hiP - loP
    if rng <= 0: continue
    wpct.append(100.0*abs(secP-advP)/rng)
    mud.append(bars[-1][0] - advT)          # PT extreme -> the close
def qq(v,p):
    v=sorted(v); return v[min(len(v)-1,max(0,int(round(p*(len(v)-1)))))] if v else 0
print('  PTWick%%   median %d%%   middle half %d%% .. %d%%   n=%d'
      % (round(qq(wpct,.5)), round(qq(wpct,.25)), round(qq(wpct,.75)), len(wpct)))
print('  PTMUD     median %dh%02d  middle half %dh%02d .. %dh%02d   n=%d'
      % (qq(mud,.5)//60, qq(mud,.5)%60, qq(mud,.25)//60, qq(mud,.25)%60,
         qq(mud,.75)//60, qq(mud,.75)%60, len(mud)))
# split by side, since PT itself is ~40% asymmetric
for lab, want in (('second = HOD', True), ('second = LOD', False)):
    W=[];M=[]
    for d, bars in rows.items():
        if len(bars) < 300: continue
        bars.sort()
        hi=max(bars,key=lambda b:b[1]); lo=min(bars,key=lambda b:b[2])
        if hi[0]==lo[0]: continue
        sH = hi[0] > lo[0]
        if sH != want: continue
        secT = hi[0] if sH else lo[0]; secP = hi[1] if sH else lo[2]
        after=[b for b in bars if b[0]>=secT]
        if len(after)<2: continue
        if sH: adv=min(after,key=lambda b:b[2]); advT,advP=adv[0],adv[2]
        else:  adv=max(after,key=lambda b:b[1]); advT,advP=adv[0],adv[1]
        rng=hi[1]-lo[2]
        if rng<=0: continue
        W.append(100.0*abs(secP-advP)/rng); M.append(bars[-1][0]-advT)
    print('    %s  PTWick%% ~%d%%   PTMUD ~%dh%02d   n=%d'
          % (lab, round(qq(W,.5)), qq(M,.5)//60, qq(M,.5)%60, len(W)))
