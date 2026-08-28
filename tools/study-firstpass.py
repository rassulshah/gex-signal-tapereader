#!/usr/bin/env python3
"""
TIMING, PROPERLY POSED: given price REACHES a level, WHEN does it get there?

    "did you do your best in creating a good model for predicting hod lod times"  - 2026-08-28

The earlier passes asked "when does the extremum print" - unconditional, and therefore nearly
uniform with a spike into the close. This asks the first-passage question instead, which is the one
a trader puts, and adds the feature every earlier pass lacked: the RATE at which price is closing on
the level (approach velocity), not just how far away it is.

Baselines it must beat, or it is not worth shipping:
  · the clock alone
  · distance / sigma alone (the touch model's own inputs)
"""
import numpy as np, collections
from importlib.machinery import SourceFileLoader
from sklearn.model_selection import GroupKFold
from sklearn.ensemble import GradientBoostingRegressor, GradientBoostingClassifier
from sklearn.metrics import roc_auc_score
SP=SourceFileLoader('sp2','tools/study-secondpred2.py').load_module()
RTH_A,RTH_B=8*3600+30*60,15*3600
raw=SP.load24('data/es-1min/ES TestingData.txt')
rows=[]
for d in sorted(raw):
    rth=[b for b in raw[d] if RTH_A<=b[0]<=RTH_B]
    if len(rth)<386: continue
    closes=[x[4] for x in rth]
    hi=lo=None
    for i,(sec,o,h,l,c,v) in enumerate(rth):
        if hi is None or h>hi: hi=h
        if lo is None or l<lo: lo=l
        mins=(sec-RTH_A)/60.
        if mins<30 or mins>300 or int(mins)%15: continue
        rng=hi-lo
        if rng<=0: continue
        seg=np.array(closes[:i+1]); rv=float(np.std(np.diff(seg))) if i>5 else 0.0
        if rv<=0: continue
        minsLeft=(RTH_B-sec)/60.
        sig=rv*np.sqrt(max(1.,minsLeft))
        # APPROACH VELOCITY: how fast price has been closing on the upside over 15 and 30 min
        v15=(c-closes[max(0,i-15)])/15.0
        v30=(c-closes[max(0,i-30)])/30.0
        fut=rth[i+1:]
        if not fut: continue
        for k in (0.5,1.0,1.5):
            for side in (1,-1):
                lvl=c+side*k*sig
                t_touch=None
                for (s2,o2,h2,l2,c2,v2) in fut:
                    if (h2>=lvl if side>0 else l2<=lvl): t_touch=(s2-sec)/60.; break
                if t_touch is None: continue          # ⚠ CONDITIONAL ON REACHING IT
                rows.append(dict(day=d,mins=mins,minsLeft=minsLeft,k=k,side=side,rng=rng,rv=rv,
                                 dist=k*sig, distR=(k*sig)/rng,
                                 vel15=side*v15/rv, vel30=side*v30/rv,   # oriented TOWARD the level
                                 posInRng=(c-lo)/rng, t=t_touch,
                                 # the analytic first-passage mean under a driftless random walk:
                                 # E[T] is infinite, so use the MEDIAN of the inverse-Gaussian-like
                                 # scaling law T ~ (distance/sigma_per_min)^2
                                 fpt=(k*sig/rv)**2))
print('first-passage rows: %d over %d sessions' % (len(rows), len(set(r['day'] for r in rows))))
y=np.array([r['t'] for r in rows]); g=np.array([r['day'] for r in rows])
print('median time to touch: %.0f min · p25 %.0f · p75 %.0f' % (np.median(y),np.percentile(y,25),np.percentile(y,75)))

def cv(feats, loss='absolute_error'):
    X=np.array([[r[f] for f in feats] for r in rows])
    p=np.zeros(len(y))
    for tr,te in GroupKFold(n_splits=5).split(X,y,g):
        m=GradientBoostingRegressor(loss=loss,n_estimators=200,max_depth=3,learning_rate=.06,
                                    random_state=7).fit(X[tr],y[tr])
        p[te]=m.predict(X[te])
    return float(np.median(np.abs(p-y))), p

print('\nPOINT PREDICTION of minutes-to-touch (median absolute error, lower is better):')
print('   the unconditional median              %5.1f min' % np.median(np.abs(np.median(y)-y)))
e,_=cv(['mins','minsLeft']);                     print('   the clock alone                       %5.1f min' % e)
e,_=cv(['fpt']);                                 print('   the analytic first-passage scaling    %5.1f min' % e)
e,_=cv(['dist','rv','minsLeft']);                print('   distance + vol + time left            %5.1f min' % e)
e,_=cv(['dist','rv','minsLeft','mins','k','distR','posInRng']);  print('   + the touch model inputs              %5.1f min' % e)
e,pv=cv(['dist','rv','minsLeft','mins','k','distR','posInRng','vel15','vel30','fpt'])
print('   + APPROACH VELOCITY + first-passage    %5.1f min   <- the untried features' % e)

# does it separate SOON from LATE? that is what a face can use
print('\nAS A CLASSIFIER — does it arrive within the next hour?')
yb=(y<=60).astype(int)
def auc(feats):
    X=np.array([[r[f] for f in feats] for r in rows]); p=np.zeros(len(yb))
    for tr,te in GroupKFold(n_splits=5).split(X,yb,g):
        m=GradientBoostingClassifier(n_estimators=200,max_depth=3,learning_rate=.06,random_state=7).fit(X[tr],yb[tr])
        p[te]=m.predict_proba(X[te])[:,1]
    return roc_auc_score(yb,p), p
a,_=auc(['mins','minsLeft']);                            print('   clock alone                           AUC %.3f' % a)
a,_=auc(['dist','rv','minsLeft']);                       print('   distance + vol + time left            AUC %.3f' % a)
a,pb=auc(['dist','rv','minsLeft','mins','k','distR','posInRng','vel15','vel30','fpt'])
print('   + approach velocity + first-passage    AUC %.3f   (base rate %.0f%%)' % (a,100*yb.mean()))
print('\n   calibration of the full model:')
for lo_,hi_ in ((0,.2),(.2,.4),(.4,.6),(.6,.8),(.8,1.01)):
    s=[(q,yy) for q,yy in zip(pb,yb) if lo_<=q<hi_]
    if len(s)<150: continue
    print('      predicted %3.0f-%3.0f%%  n=%5d  actual %3.0f%%' % (100*lo_,100*hi_,len(s),100*np.mean([x[1] for x in s])))
