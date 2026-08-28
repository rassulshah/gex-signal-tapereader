import numpy as np, collections
from importlib.machinery import SourceFileLoader
from sklearn.model_selection import GroupKFold
from sklearn.ensemble import GradientBoostingClassifier
from sklearn.metrics import roc_auc_score, brier_score_loss
SP=SourceFileLoader('sp2','tools/study-secondpred2.py').load_module()
RTH_A,RTH_B=8*3600+30*60,15*3600
raw=SP.load24('data/es-1min/ES TestingData.txt')
days=sorted(raw)
rows=[]
for d in days:
    rth=[b for b in raw[d] if RTH_A<=b[0]<=RTH_B]
    if len(rth)<386: continue
    closes=[x[4] for x in rth]
    hi=lo=None
    for i,(sec,o,h,l,c,v) in enumerate(rth):
        if hi is None or h>hi: hi=h
        if lo is None or l<lo: lo=l
        mins=(sec-RTH_A)/60.
        if mins<30 or mins>330: continue
        if int(mins)%15: continue                 # decide every 15 minutes
        rng=hi-lo
        if rng<=0: continue
        seg=np.array(closes[:i+1]); rv=float(np.std(np.diff(seg))) if i>5 else 0.0
        minsLeft=(RTH_B-sec)/60.
        # sigma of the remaining session, from realized 1-min vol
        sig=rv*np.sqrt(max(1.0,minsLeft))
        fut=rth[i+1:]
        if not fut: continue
        futHi=max(x[2] for x in fut); futLo=min(x[3] for x in fut)
        for k in (0.25,0.5,0.75,1.0,1.5,2.0):     # level distance in sigma units
            for side in (1,-1):
                dist=k*sig
                if dist<=0: continue
                lvl=c+side*dist
                y=1 if (futHi>=lvl if side>0 else futLo<=lvl) else 0
                rows.append(dict(day=d,mins=mins,minsLeft=minsLeft,k=k,side=side,
                                 rng=rng,rv=rv,distR=dist/rng,posInRng=(c-lo)/rng,y=y))
print('touch rows: %d over %d sessions'%(len(rows),len(set(r['day'] for r in rows))))
FE=['k','minsLeft','mins','distR','posInRng','rng','rv','side']
X=np.array([[r[f] for f in FE] for r in rows]); y=np.array([r['y'] for r in rows]); g=np.array([r['day'] for r in rows])
p=np.zeros(len(y))
for tr,te in GroupKFold(n_splits=5).split(X,y,g):
    m=GradientBoostingClassifier(n_estimators=200,max_depth=3,learning_rate=.06,random_state=7).fit(X[tr],y[tr])
    p[te]=m.predict_proba(X[te])[:,1]
print('P(level is touched before the close):  AUC %.3f   Brier %.3f   base %.0f%%'%(roc_auc_score(y,p),brier_score_loss(y,p),100*y.mean()))
print('\nCALIBRATION (out-of-fold):')
for lo_,hi_ in ((0,.1),(.1,.2),(.2,.3),(.3,.4),(.4,.5),(.5,.6),(.6,.7),(.7,.8),(.8,.9),(.9,1.01)):
    s=[(q,yy) for q,yy in zip(p,y) if lo_<=q<hi_]
    if len(s)<200: continue
    print('   predicted %3.0f-%3.0f%%   n=%6d   actual %3.0f%%'%(100*lo_,100*hi_,len(s),100*np.mean([x[1] for x in s])))
print('\nHOW SHARP IS IT?  share of decisions landing in a confident bucket:')
print('   p<=20%%: %5.1f%% of rows (actual %3.0f%%)   p>=80%%: %5.1f%% (actual %3.0f%%)'
      %(100*np.mean(p<=.2),100*np.mean(y[p<=.2]),100*np.mean(p>=.8),100*np.mean(y[p>=.8])))
