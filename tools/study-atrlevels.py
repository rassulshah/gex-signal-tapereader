#!/usr/bin/env python3
"""
DOES THE DAILY ATR - AND THE IDENTITY OF THE LEVEL - IMPROVE THE TOUCH AND TIMING MODELS?

    "can you use the expected move, the daily atr, and can you think of other things"  - 2026-08-28

Two questions, both free (no new data):
  A · ATR NORMALISATION. "how much of a normal day has this session already used" (range/ATR14) and
      "how much room is left" - the realized-vol analogue of the expected move.
  B · LEVEL IDENTITY. At the SAME distance and the same time left, does it matter WHAT the level is -
      the prior-day high/low/close, the overnight high/low, or a round number?
      ⚠ This is the dry run for the gamma question. If level identity moves the probability for a
      PRIOR-DAY high, it can move it for a put wall - and the experiment is already built.
"""
import numpy as np, collections
from importlib.machinery import SourceFileLoader
from sklearn.model_selection import GroupKFold
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.metrics import roc_auc_score, brier_score_loss
SP=SourceFileLoader('sp2','tools/study-secondpred2.py').load_module()
RTH_A,RTH_B=8*3600+30*60,15*3600
raw=SP.load24('data/es-1min/ES TestingData.txt')
days=sorted(raw)

# daily ATR(14) from RTH true ranges
daystat={}
prevC=None
trs=[]
for d in days:
    rth=[b for b in raw[d] if RTH_A<=b[0]<=RTH_B]
    if len(rth)<386: continue
    hi=max(x[2] for x in rth); lo=min(x[3] for x in rth); cl=rth[-1][4]
    tr=max(hi-lo, abs(hi-prevC) if prevC else 0, abs(lo-prevC) if prevC else 0)
    daystat[d]=dict(hi=hi,lo=lo,cl=cl,atr=(np.mean(trs[-14:]) if len(trs)>=5 else np.nan))
    trs.append(tr); prevC=cl

rows=[]
for di,d in enumerate(days):
    if d not in daystat: continue
    rth=[b for b in raw[d] if RTH_A<=b[0]<=RTH_B]
    on=[b for b in raw.get(days[di-1],[]) if b[0]>=17*3600]+[b for b in raw[d] if b[0]<RTH_A] if di>0 else []
    onH=max((x[2] for x in on),default=np.nan); onL=min((x[3] for x in on),default=np.nan)
    pd_=daystat.get(days[di-1]) if di>0 else None
    atr=daystat[d]['atr']
    if not (atr==atr) or atr<=0: continue
    closes=[x[4] for x in rth]; hi=lo=None
    for i,(sec,o,h,l,c,v) in enumerate(rth):
        if hi is None or h>hi: hi=h
        if lo is None or l<lo: lo=l
        mins=(sec-RTH_A)/60.
        if mins<30 or mins>300 or int(mins)%30: continue
        rng=hi-lo
        if rng<=0: continue
        seg=np.array(closes[:i+1]); rv=float(np.std(np.diff(seg))) if i>5 else 0.
        if rv<=0: continue
        minsLeft=(RTH_B-sec)/60.; sig=rv*np.sqrt(max(1.,minsLeft))
        fut=rth[i+1:]
        if not fut: continue
        futHi=max(x[2] for x in fut); futLo=min(x[3] for x in fut)
        cand=[]
        for k in (0.15,0.3,0.5,0.75,1.0,1.5,2.0,2.5):
            for side in (1,-1): cand.append((c+side*k*sig, side, 'sigma'))
        if pd_:
            for nm,px in (('pdh',pd_['hi']),('pdl',pd_['lo']),('pdc',pd_['cl'])):
                cand.append((px, 1 if px>c else -1, nm))
        if on:
            cand.append((onH,1 if onH>c else -1,'onh')); cand.append((onL,1 if onL>c else -1,'onl'))
        rnd=25*round(c/25.0)
        cand.append((rnd, 1 if rnd>c else -1, 'round'))
        for lvl,side,kind in cand:
            dist=abs(lvl-c)
            if dist<1e-6 or dist>4*sig: continue
            y=1 if (futHi>=lvl if side>0 else futLo<=lvl) else 0
            rows.append(dict(day=d,mins=mins,minsLeft=minsLeft,side=side,kind=kind,
                             dist=dist,distSig=dist/sig,distATR=dist/atr,distR=dist/rng,
                             rng=rng,rv=rv,atr=atr,rngATR=rng/atr,roomATR=max(0.,(atr-rng))/atr,
                             posInRng=(c-lo)/rng,y=y))
print('rows %d · sessions %d'%(len(rows),len(set(r['day'] for r in rows))))
y=np.array([r['y'] for r in rows]); g=np.array([r['day'] for r in rows])
def cv(feats):
    X=np.array([[r[f] for f in feats] for r in rows]); p=np.zeros(len(y))
    for tr,te in GroupKFold(n_splits=5).split(X,y,g):
        m=GradientBoostingClassifier(n_estimators=120,max_depth=3,learning_rate=.06,random_state=7).fit(X[tr],y[tr])
        p[te]=m.predict_proba(X[te])[:,1]
    return roc_auc_score(y,p), brier_score_loss(y,p), p
print('\nA · DOES ATR NORMALISATION HELP THE TOUCH MODEL?')
base=['distSig','minsLeft','mins','distR','posInRng','rng','rv','side']
a,b,_=cv(base);                      print('   the shipped inputs                    AUC %.4f Brier %.4f'%(a,b))
a2,b2,_=cv(base+['atr']);            print('   + daily ATR(14)                       AUC %.4f (%+0.4f)'%(a2,a2-a))
a3,b3,_=cv(base+['rngATR']);         print('   + range so far / ATR                  AUC %.4f (%+0.4f)'%(a3,a3-a))
a4,b4,_=cv(base+['roomATR']);        print('   + room left in a normal day           AUC %.4f (%+0.4f)'%(a4,a4-a))
a5,b5,_=cv(base+['distATR']);        print('   + distance measured in ATR            AUC %.4f (%+0.4f)'%(a5,a5-a))
a6,b6,_=cv(base+['atr','rngATR','roomATR','distATR'])
print('   + all four ATR terms                  AUC %.4f (%+0.4f)  Brier %.4f'%(a6,a6-a,b6))

print('\nB · DOES THE IDENTITY OF THE LEVEL MATTER, AT THE SAME DISTANCE?')
print('   %-8s %7s %11s %14s %12s'%('kind','n','mean dist(s)','touched','vs sigma-matched'))
byk=collections.defaultdict(list)
for r in rows: byk[r['kind']].append(r)
sig_rows=byk['sigma']
for kind in ('pdh','pdl','pdc','onh','onl','round'):
    R=byk.get(kind,[])
    if len(R)<200: continue
    md=np.mean([r['distSig'] for r in R]); hit=np.mean([r['y'] for r in R])
    # matched control: sigma levels at a similar distance and time of day
    ctl=[r for r in sig_rows if abs(r['distSig']-md)<0.25]
    print('   %-8s %7d %11.2f %13.0f%% %11.0f%%  (n=%d)'%(kind,len(R),md,100*hit,100*np.mean([r['y'] for r in ctl]),len(ctl)))

# ---- B2 · THE RESIDUAL TEST — the design the gamma question will reuse -----------------------
# Train ONLY on distance-defined (sigma) levels, then ask the named levels whether they are touched
# MORE or LESS often than a generic level at the same distance, time and volatility would be.
print('\nB2 · RESIDUAL TEST — actual vs what a distance-only model expects')
FE=['distSig','minsLeft','mins','distR','posInRng','rng','rv','side']
sig=[r for r in rows if r['kind']=='sigma']
Xs=np.array([[r[f] for f in FE] for r in sig]); ys=np.array([r['y'] for r in sig]); gs=np.array([r['day'] for r in sig])
mdl=GradientBoostingClassifier(n_estimators=150,max_depth=3,learning_rate=.06,random_state=7).fit(Xs,ys)
print('   %-8s %7s %13s %11s %10s'%('kind','n','model expects','actual','lift'))
for kind in ('pdh','pdl','pdc','onh','onl','round'):
    R=[r for r in rows if r['kind']==kind]
    if len(R)<200: continue
    X=np.array([[r[f] for f in FE] for r in R]); Y=np.array([r['y'] for r in R])
    p=mdl.predict_proba(X)[:,1]
    lift=100*(Y.mean()-p.mean())
    se=100*np.sqrt(Y.mean()*(1-Y.mean())/max(1,len(set(r['day'] for r in R))))
    print('   %-8s %7d %12.0f%% %10.0f%% %8.1f pts  (+-%.1f, session-clustered)'
          %(kind,len(R),100*p.mean(),100*Y.mean(),lift,se))
