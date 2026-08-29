#!/usr/bin/env python3
"""
Third and current answer: the split is MOTION vs STASIS, not growth vs decay.

study-succession2.py compared "growing" against "flat or draining" using a crude 3-snapshot
comparison, and reported growth adding +18-22pp. That lumped two very different populations
together. The panel already computes a per-node state (nodeHist: Building / Fading / Steady, with a
steady band, a slope test and a give-back budget) and splitting by THAT shows:

    Fading converts almost as well as Building. Steady converts BELOW chance.

So a challenger that is MOVING - in either direction - is the signal, and one that is sitting still
is the anti-signal. Nothing new has to be computed: the state is already on the face, it is simply
not connected to the succession claim.

Outcome is the DRAWN crown (kingLatchTick replayed), never the raw tape - see study-kingmoves.py.
"""
import json, io, glob
HZ_LIST=(30,60); LATCH=120000; CHANCE=8.0

def drawn(ser):
    if not ser: return []
    k=ser[0][1]; cand=None; c0=0; out=[(ser[0][0],k)]
    for t,tk in ser[1:]:
        if tk==k: cand=None
        elif cand!=tk: cand=tk; c0=t
        elif t-c0>=LATCH: k=tk; cand=None
        out.append((t,k))
    return out

days=[]
for f in sorted(glob.glob('data/2026-*.json')):
    d=json.load(io.open(f)); s=(d.get('snaps') or {}).get('SPY') or []
    s=[x for x in s if isinstance(x.get('king'),(int,float))]
    if len(s)>=20: days.append((d['date'],s,dict(drawn([(x['t'],x['king']) for x in s]))))

def ranks(x): return [(q['k'],q['m']) for q in ((x.get('deriv') or {}).get('ranks') or []) if q.get('k') is not None and q.get('m') is not None]
def stOf(x,k):
    for nd in (x.get('nodes') or []):
        if nd.get('k')==k: return nd.get('st')
    return None

print('days %d  snaps %d   outcome = the DRAWN crown, chance = %.1f%%'
      % (len(days), sum(len(s) for _,s,_ in days), CHANCE))
print('\n%-8s %-5s | %-17s %-17s %-17s'%('size','horiz','Building','Fading','Steady'))
for ms in (50,60,70,80):
    for hz in HZ_LIST:
        b={}
        for date,s,crown in days:
            for i,x in enumerate(s):
                king=crown.get(x['t'])
                c=[(k,m) for k,m in ranks(x) if k!=king]
                if not c: continue
                k2,m2=c[0]
                if m2<ms: continue
                st=stOf(x,k2) or '?'
                t0=x['t']; h=False
                for y in s[i+1:]:
                    if y['t']-t0>hz*60000: break
                    if crown.get(y['t'])==k2: h=True; break
                e=b.setdefault(st,[0,0]); e[1]+=1; e[0]+=1 if h else 0
        def f(k):
            h,n=b.get(k,[0,0]); return '%5.1f%% n=%-4d'%((100.0*h/n if n else 0),n)
        print('  >=%-3d%% %-5d | %-17s %-17s %-17s'%(ms,hz,f('Building'),f('Fading'),f('Steady')))
print('\nSteady at size>=80 is BELOW chance - a large challenger that is not moving is the cleanest')
print('negative read available: it says this thing is not going anywhere.')
