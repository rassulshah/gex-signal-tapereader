#!/usr/bin/env python3
"""
WHERE AND WHEN DOES THE OPPOSITE EXTREMITY PRINT, ONCE THE FIRST ONE IS CALLED IN?

    "I want it to say something like LOD IN -74%, HOD expected around 7772-7792 in 3.5 Hrs
     between 1:30pm and 2pm - 80%."   - operator, 2026-08-28

⚠ ONE ROW PER SESSION, at the FIRST bar the shipped table crosses its 70% threshold. Bars inside a
session are not independent; the decision is taken once.
⚠ EVERY WINDOW IS SCORED BY OUT-OF-SAMPLE COVERAGE, split by date. A quantile fitted and scored on
the same rows will look perfect and mean nothing.
⚠ THE BASELINE IS THE UNCONDITIONAL WINDOW. If conditioning does not beat "the same window every
day", it has learned nothing - the F-1/F-6 lesson.
"""
import sys, json, collections
import numpy as np
sys.path.insert(0, 'tools')
from importlib.machinery import SourceFileLoader
M = SourceFileLoader('m', 'tools/model-lodhod.py').load_module()

OPEN = 8*3600+30*60
CLOSE = 15*3600
MINBAR = 5
# the SHIPPED table (v14.70, 72 cells) - rows = posr octile, cols = 45-min blocks from 08:30
HLTAB = [
 [4,7,8,9,12,15,20,28,47],[10,18,21,33,36,39,57,66,84],[16,26,34,49,56,65,68,84,96],
 [22,33,50,55,65,78,85,95,99],[27,45,62,67,73,81,91,97,100],[32,58,72,75,84,88,95,98,100],
 [40,59,76,86,89,95,98,99,100],[47,74,84,90,96,99,99,100,100]]
THRESH = 70

def cell(posr, mins):
    pi = max(0, min(7, int(posr*8)))
    ti = max(0, min(8, int(mins//45)))
    return HLTAB[pi][ti]

def rows_for(sessions):
    out = []
    for d in sorted(sessions):
        bars = sessions[d]
        if bars[0][0] > OPEN + 60:      # session must actually start at the open
            continue
        # FINAL session extremes
        fh = max(b[2] for b in bars); fl = min(b[3] for b in bars)
        fhT = min(b[0] for b in bars if b[2] == fh); flT = min(b[0] for b in bars if b[3] == fl)
        hi = lo = None; hiT = loT = None; called = None
        for (sec, o, h, l, c) in bars:
            if hi is None or h > hi: hi, hiT = h, sec
            if lo is None or l < lo: lo, loT = l, sec
            if sec - OPEN < MINBAR*60: continue
            rng = hi - lo
            if rng <= 0: continue
            first = 'LOD' if loT < hiT else 'HOD'
            posr = (c - lo)/rng if first == 'LOD' else (hi - c)/rng
            posr = max(0.0, min(1.0, posr))
            if cell(posr, (sec-OPEN)/60.0) >= THRESH:
                called = dict(t=sec, px=c, first=first, rng=rng, posr=posr,
                              extT=(loT if first=='LOD' else hiT),
                              extPx=(lo if first=='LOD' else hi))
                break
        if not called: continue
        # the OPPOSITE side's FINAL extreme
        if called['first'] == 'LOD': oppT, oppPx, side = fhT, fh, 'HOD'
        else:                        oppT, oppPx, side = flT, fl, 'LOD'
        firstHeld = (called['extPx'] == (fl if called['first']=='LOD' else fh))
        out.append(dict(day=d, side=side, callT=called['t'], callPx=called['px'],
                        rng=called['rng'], posr=called['posr'],
                        took=(called['extT']-OPEN)/60.0,
                        oppT=oppT, oppPx=oppPx,
                        ahead=1 if oppT > called['t'] else 0,
                        mins_to=(oppT-called['t'])/60.0,
                        move=(oppPx-called['px']) if side=='HOD' else (called['px']-oppPx),
                        moveR=((oppPx-called['px']) if side=='HOD' else (called['px']-oppPx))/called['rng'],
                        firstHeld=1 if firstHeld else 0,
                        minsLeft=(CLOSE-called['t'])/60.0))
    return out

def cover(rows, key, lo_q, hi_q, cond=None):
    """coverage of a quantile window fitted on TRAIN and scored on TEST, split by date"""
    rows = sorted(rows, key=lambda r: r['day'])
    k = int(len(rows)*0.6)
    tr, te = rows[:k], rows[k:]
    v_tr = np.array([r[key] for r in tr]); v_te = np.array([r[key] for r in te])
    lo, hi = np.quantile(v_tr, lo_q), np.quantile(v_tr, hi_q)
    hit = float(np.mean((v_te >= lo) & (v_te <= hi)))
    return lo, hi, hit, len(te)

def main():
    S = M.load('data/es-1min/ES TestingData.txt')
    rows = rows_for(S)
    print('sessions with a call: %d of %d' % (len(rows), len(S)))
    ahead = [r for r in rows if r['ahead']]
    print('far side still ahead at the call: %d (%.0f%%)' % (len(ahead), 100*len(ahead)/len(rows)))
    print('first extreme actually held    : %.0f%%' % (100*np.mean([r['firstHeld'] for r in rows])))
    print('median call time               : %s' % _clk(np.median([r['callT'] for r in ahead])))
    A = ahead
    mt = np.array([r['mins_to'] for r in A]); mv = np.array([r['moveR'] for r in A])
    print('\nTIME FROM THE CALL TO THE OPPOSITE EXTREME (minutes)')
    for q in (5,10,25,50,75,90,95):
        print('   p%-3d %6.0f min  (%.1fh)' % (q, np.percentile(mt,q), np.percentile(mt,q)/60))
    print('\nMOVE TO THE OPPOSITE EXTREME, as a multiple of the range so far')
    for q in (5,10,25,50,75,90,95):
        print('   p%-3d %6.2f x' % (q, np.percentile(mv,q)))
    print('\nOUT-OF-SAMPLE COVERAGE OF AN UNCONDITIONAL WINDOW (fit on the first 60%% of days)')
    print('   %-8s %-26s %-26s' % ('target','TIME window','coverage'))
    for want,(a,b) in [(50,(.25,.75)),(60,(.20,.80)),(70,(.15,.85)),(80,(.10,.90)),(90,(.05,.95))]:
        lo,hi,hit,n = cover(A,'mins_to',a,b)
        print('   %-8s %5.0f-%5.0f min (%.1f-%.1fh, width %.1fh)   hit %.0f%%  n=%d'
              % (str(want)+'%', lo, hi, lo/60, hi/60, (hi-lo)/60, 100*hit, n))
    print('\n   %-8s %-26s %-26s' % ('target','PRICE window (x range so far)','coverage'))
    for want,(a,b) in [(50,(.25,.75)),(60,(.20,.80)),(70,(.15,.85)),(80,(.10,.90)),(90,(.05,.95))]:
        lo,hi,hit,n = cover(A,'moveR',a,b)
        print('   %-8s %5.2f-%5.2f x  (width %.2fx)                 hit %.0f%%  n=%d'
              % (str(want)+'%', lo, hi, hi-lo, 100*hit, n))
    # how often does a NARROW 30-minute box land?
    print('\nTHE 30-MINUTE BOX HE ASKED FOR (median-centred, unconditional)')
    med = np.median([r['mins_to'] for r in A])
    hit30 = np.mean([abs(r['mins_to']-med) <= 15 for r in A])
    hit60 = np.mean([abs(r['mins_to']-med) <= 30 for r in A])
    print('   +-15 min around the median: %.0f%%   +-30 min: %.0f%%' % (100*hit30, 100*hit60))
    json.dump(rows, open('/tmp/second_rows.json','w'))
    print('\nrows written to /tmp/second_rows.json')

def _clk(sec):
    sec=int(sec); return '%02d:%02d' % (sec//3600, (sec%3600)//60)

main()
