#!/usr/bin/env python3
"""
DID A KING CAUSE THE DEFLECTION? — how close the three crowns actually sit to the day's extremes.

Operator, 2026-08-29: "next to the levels lets also have either a LodN and HodN depending on whether
the extremity was a lod or hod. This node will typically be a king node or at least a node that is
strong. Typically we are looking for 1 of the three kings ... to have caused a deflection."

Before building a field that NAMES a cause, the question is whether the kings are near the extremes
more than chance puts them there. A king within 5 points of the high on a 70-point day is only
interesting if a random level would not be.

⚠⚠ ES/SPY, NOT SPX/SPY. The recorded snaps carry SPY prices and SPX/SPY in `xm`; ES is a FUTURE on
SPX and trades at a basis. Using SPX/SPY put every crown ~13 ES points low on the first run of this —
the exact scale-chain landmine (L-F) this project keeps catching. The ratio is derived per snapshot
from the SPY price and a fixed ES/SPX basis.
"""
import json, io, glob, datetime, random, statistics

BASIS = 1.0022          # ES / SPX
random.seed(11)

def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000-5*3600).strftime('%H:%M')

rows=[]
for f in sorted(glob.glob('data/2026-*.json')):
    d=json.load(io.open(f))
    s=[x for x in (d.get('snaps') or {}).get('SPY',[]) if isinstance(x.get('px'),(int,float))]
    if len(s)<40: continue
    # the session's extremes, in SPY space
    hi=max(s,key=lambda x:(x.get('h') or x['px'])); lo=min(s,key=lambda x:(x.get('l') or x['px']))
    HI=hi.get('h') or hi['px']; LO=lo.get('l') or lo['px']
    def kingsAt(x, px):
        tri=x.get('tri') or {}; xm=x.get('xm') or {}
        spy=(xm.get('SPY') or {}).get('px'); spx=(xm.get('SPXW') or {}).get('px'); q=(xm.get('QQQ') or {}).get('px')
        if not (spy and spx): return None
        R=(spx/spy)*BASIS                     # ES per SPY dollar
        ky=(tri.get('SPY') or {}).get('king'); kx=(tri.get('SPXW') or {}).get('king'); kq=(tri.get('QQQ') or {}).get('king')
        out={}
        if ky: out['SPY']=ky*R
        if kx: out['SPXW']=kx*BASIS
        if kq and q: out['QQQ']=(px*R)*(kq/q)
        return (R, out)
    for lab, snap, pxSpy in (('HOD',hi,HI), ('LOD',lo,LO)):
        k=kingsAt(snap, pxSpy)
        if not k: continue
        R, K = k
        if not K: continue
        pxEs=pxSpy*R
        rngEs=(HI-LO)*R
        if rngEs<=0: continue
        best=min(((abs(pxEs-v), n) for n,v in K.items()))
        rows.append({'d':d.get('date'),'what':lab,'px':pxEs,'rng':rngEs,
                     'dist':best[0],'book':best[1],
                     'all':{n:abs(pxEs-v) for n,v in K.items()}})

print('extremes with a full crown set: %d  over %d sessions'
      % (len(rows), len(set(r['d'] for r in rows))))
print()
print('%-12s %-5s %-9s %-8s %-6s  %s' % ('date','what','ES px','nearest','dist','all three (pts away)'))
for r in rows:
    print('%-12s %-5s %-9.0f %-8s %-6.0f  %s' % (r['d'], r['what'], r['px'], r['book'], r['dist'],
          ' '.join('%s %.0f'%(n,v) for n,v in sorted(r['all'].items(), key=lambda z:z[1]))))

D=[r['dist'] for r in rows]
print('\nnearest-crown distance to an extreme:  median %.0f pts   min %.0f   max %.0f' %
      (statistics.median(D), min(D), max(D)))
for thr in (3,5,8,12,20):
    n=sum(1 for v in D if v<=thr)
    print('   within %2d pts: %4.0f%%  (%d of %d)' % (thr, 100.0*n/len(D), n, len(D)))

# ⚠ THE CONTROL. On a 70-point day with three crowns scattered in it, SOMETHING is always nearish.
# Compare against three sham levels drawn uniformly inside the same session's range.
print('\nCONTROL — three sham levels drawn inside the same session range:')
ctl=[]
for r in rows:
    lo_, hi_ = r['px']-r['rng'], r['px']+r['rng']
    sham=[random.uniform(r['px']-r['rng'], r['px']+r['rng']) for _ in range(3)]
    ctl.append(min(abs(r['px']-v) for v in sham))
print('   median %.0f pts' % statistics.median(ctl))
for thr in (3,5,8,12,20):
    n=sum(1 for v in ctl if v<=thr)
    print('   within %2d pts: %4.0f%%  (%d of %d)   -> crowns are %+.0f pp better'
          % (thr, 100.0*n/len(ctl), n, len(ctl),
             100.0*sum(1 for v in D if v<=thr)/len(D) - 100.0*n/len(ctl)))
