#!/usr/bin/env python3
"""
THE FAR-SIDE TABLES THE PANEL CARRIES — derived here so they can be RE-DERIVED as data grows.

Emits data/es-1min/FARSIDE.json:
  touch   P(a level at d sigma is traded before the close), by distance x minutes-left      (F-14)
  timing  minutes to touch GIVEN it is touched, median and IQR, by distance                 (F-15)
  hazard  P(the far side prints in the last 45 min | it has not printed by this clock)      (F-15)
  floor   the ONE-SIDED 80% floor: the far side is still ahead of +N minutes                (F-13/15)
Every cell carries its own n and a cell under the floor is emitted as null - it must refuse.
"""
import json, numpy as np, collections
from importlib.machinery import SourceFileLoader
SP=SourceFileLoader('sp2','tools/study-secondpred2.py').load_module()
RTH_A,RTH_B=8*3600+30*60,15*3600
MINCELL=60
DBINS=[0.0,0.25,0.5,0.75,1.0,1.5,2.0,3.0]        # distance in sigma
TBINS=[0,45,90,135,180,240,300,390]              # minutes LEFT in the session
def dbin(x):
    for i in range(len(DBINS)-1):
        if DBINS[i]<=x<DBINS[i+1]: return i
    return len(DBINS)-2
def tbin(x):
    for i in range(len(TBINS)-1):
        if TBINS[i]<=x<TBINS[i+1]: return i
    return len(TBINS)-2

raw=SP.load24('data/es-1min/ES TestingData.txt')
touch=collections.defaultdict(lambda:[0,0])      # (d,t) -> [n, hits]
tim=collections.defaultdict(list)                # d -> [minutes to touch]
haz=collections.defaultdict(lambda:[0,0])        # clock bucket -> [still out, landed last45]
flo=[]                                           # minutes from call to far side
days=sorted(raw)
for d in days:
    rth=[b for b in raw[d] if RTH_A<=b[0]<=RTH_B]
    if len(rth)<386: continue
    closes=[x[4] for x in rth]
    hi=lo=None
    for i,(sec,o,h,l,c,v) in enumerate(rth):
        if hi is None or h>hi: hi=h
        if lo is None or l<lo: lo=l
        mins=(sec-RTH_A)/60.
        if mins<5 or int(mins)%5: continue
        seg=np.array(closes[:i+1]); rv=float(np.std(np.diff(seg))) if i>5 else 0.
        if rv<=0: continue
        minsLeft=(RTH_B-sec)/60.
        if minsLeft<5: continue
        sig=rv*np.sqrt(minsLeft)
        fut=rth[i+1:]
        if not fut: continue
        futHi=max(x[2] for x in fut); futLo=min(x[3] for x in fut)
        for k in (0.25,0.4,0.6,0.8,1.0,1.25,1.5,2.0,2.5):
            for side in (1,-1):
                lvl=c+side*k*sig
                hit=(futHi>=lvl) if side>0 else (futLo<=lvl)
                key=(dbin(k),tbin(minsLeft))
                touch[key][0]+=1; touch[key][1]+=1 if hit else 0
                if hit:
                    for (s2,o2,h2,l2,c2,v2) in fut:
                        if (h2>=lvl if side>0 else l2<=lvl):
                            tim[dbin(k)].append((s2-sec)/60.); break
    # hazard + floor use the FAR SIDE of the standing extreme after the >=85% call
S,_=SP.build('data/es-1min/ES TestingData.txt',85)
A=[r for r in S if r['ahead'] and r['held']]
for r in A:
    flo.append(r['tmin'])
    for hh in range(10,15):
        for mm in (0,30):
            t=hh*3600+mm*60
            if r['callT']<=t<r['oppT']:
                haz[(hh,mm)][0]+=1
                if r['oppT']>=RTH_B-45*60: haz[(hh,mm)][1]+=1

TT=[[None]*(len(TBINS)-1) for _ in range(len(DBINS)-1)]
for (di,ti),(n,hit) in touch.items():
    TT[di][ti]=[n, int(round(100.0*hit/n))] if n>=MINCELL else [n,None]
TM=[]
for di in range(len(DBINS)-1):
    v=tim.get(di,[])
    TM.append([len(v), None,None,None] if len(v)<MINCELL else
              [len(v), int(np.median(v)), int(np.percentile(v,25)), int(np.percentile(v,75))])
HZ=[]
for (hh,mm),(n,c) in sorted(haz.items()):
    if n>=25: HZ.append([hh*60+mm, n, int(round(100.0*c/n))])
out=dict(
  corpus=dict(market='ES', sessions=len(set(x['day'] for x in A)), first=days[0], last=days[-1],
              built='2026-08-28', obs=int(sum(v[0] for v in touch.values()))),
  bins=dict(dist=DBINS, minsLeft=TBINS, minCell=MINCELL),
  touch=TT, timing=TM, hazard=HZ,
  floor=dict(n=len(flo), p80=int(np.percentile(flo,20)), p50=int(np.percentile(flo,50)),
             q25=int(np.percentile(flo,25)), q75=int(np.percentile(flo,75))))
json.dump(out, open('data/es-1min/FARSIDE.json','w'), indent=1)
print('sessions %d · touch observations %d' % (out['corpus']['sessions'], out['corpus']['obs']))
print('\nTOUCH TABLE  rows = distance in sigma, cols = minutes left')
print('   %-12s'%'dist \\ left', ' '.join('%6s'%('%d-%d'%(TBINS[i],TBINS[i+1])) for i in range(len(TBINS)-1)))
for di in range(len(DBINS)-1):
    print('   %-12s'%('%.2f-%.2f'%(DBINS[di],DBINS[di+1])),
          ' '.join(('%5s%%'%TT[di][ti][1] if TT[di][ti] and TT[di][ti][1] is not None else '     -') for ti in range(len(TBINS)-1)))
print('\nTIMING (minutes to touch, given touched)')
for di,row in enumerate(TM):
    if row[1] is None: continue
    print('   %.2f-%.2f sigma   n=%5d   median %3d   IQR %3d-%3d' % (DBINS[di],DBINS[di+1],row[0],row[1],row[2],row[3]))
print('\nHAZARD (P last-45 | still out)'); print('  ', HZ)
print('\nFLOOR: 80%% of far sides are still ahead of +%d min (n=%d) · median +%d · IQR %d-%d'
      % (out['floor']['p80'],out['floor']['n'],out['floor']['p50'],out['floor']['q25'],out['floor']['q75']))
