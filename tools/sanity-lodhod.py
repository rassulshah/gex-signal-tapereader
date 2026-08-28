import importlib.util, numpy as np, collections
from sklearn.linear_model import LogisticRegression
from sklearn.metrics import roc_auc_score, brier_score_loss
spec=importlib.util.spec_from_file_location('m','tools/model-lodhod.py')
mm=importlib.util.module_from_spec(spec); spec.loader.exec_module(mm)
es=mm.load('data/es-1min/ES TestingData.txt')
days,rows=mm.build(es,None)
y=np.array([r['y'] for r in rows]); g=np.array([r['d'] for r in rows])
L=lambda: LogisticRegression(max_iter=2000,C=1.0)

# --- A · is the 5-feature model beating a DUMB LOOKUP TABLE? ---
from sklearn.model_selection import GroupKFold
def lookup_cv(keyfn, nb=8):
    oof=np.zeros(len(y))
    for tr,te in GroupKFold(n_splits=5).split(np.zeros(len(y)),y,g):
        tab=collections.defaultdict(lambda:[0,0])
        for i in tr:
            k=keyfn(rows[i]); tab[k][0]+=1; tab[k][1]+=y[i]
        prior=y[tr].mean()
        for i in te:
            k=keyfn(rows[i]); n,h=tab[k]
            oof[i]=(h+prior*10)/(n+10)          # smoothed
    return oof
kb=lambda r:(min(int(r['posr']*8),7), min(int(r['mins']//45),8))
kb3=lambda r:(min(int(r['posr']*8),7), min(int(r['mins']//45),8), min(int(r['stood']//45),6))
o_tab=lookup_cv(kb); o_tab3=lookup_cv(kb3)
o_m,_,_=mm.cv(rows,['mins','stood','extmin','posr','rsi'],L)
o_2,_,_=mm.cv(rows,['mins','posr'],L)
print('A · IS THE MODEL EARNING ITS COMPLEXITY?')
for nm,o in [('lookup table posr x time',o_tab),('lookup table posr x time x stood',o_tab3),
             ('logistic, 2 features (posr,mins)',o_2),('logistic, 5 features',o_m)]:
    print('   %-34s AUC %.4f  Brier %.4f'%(nm,roc_auc_score(y,o),brier_score_loss(y,o)))

# --- B · SURVIVAL / HAZARD framing, the textbook fit for this question ---
# P(no new extreme before close) built from a per-bar hazard instead of a direct classifier.
haz=[]
for r in rows:
    haz.append(r)
# label: does a NEW extreme print within the next 15 minutes?
byk=collections.defaultdict(list)
for i,r in enumerate(rows): byk[(r['d'],r['side'])].append(i)
hz=np.zeros(len(rows))
for k,ii in byk.items():
    ii.sort(key=lambda i: rows[i]['mins'])
    for j,i in enumerate(ii):
        nxt=[x for x in ii[j+1:] if rows[x]['mins']<=rows[i]['mins']+15]
        # a new extreme printed if `stood` RESET (went down) in that window
        hz[i]=1 if any(rows[x]['stood']<rows[i]['stood'] for x in nxt) else 0
X=np.array([[r['mins'],r['stood'],r['posr'],r['rsi']] for r in rows])
oh=np.zeros(len(y))
for tr,te in GroupKFold(n_splits=5).split(X,hz,g):
    mh=LogisticRegression(max_iter=2000).fit(X[tr],hz[tr]); oh[te]=mh.predict_proba(X[te])[:,1]
# survival to close = product of (1-hazard) over remaining 15-min blocks
surv=np.zeros(len(rows))
for i,r in enumerate(rows):
    blocks=max(0.0,(390-r['mins'])/15.0)
    surv[i]=(1-oh[i])**blocks
print('\nB · SURVIVAL (HAZARD) FRAMING vs DIRECT CLASSIFICATION')
print('   %-34s AUC %.4f  Brier %.4f'%('hazard -> survival curve',roc_auc_score(y,surv),brier_score_loss(y,np.clip(surv,0,1))))
print('   %-34s AUC %.4f  Brier %.4f'%('direct classifier (what I built)',roc_auc_score(y,o_m),brier_score_loss(y,o_m)))

# --- C · REGIME STABILITY ---
print('\nC · REGIME STABILITY (does it hold across different markets?)')
rng={d:(max(x[2] for x in es[d])-min(x[3] for x in es[d])) for d in days}
med=np.median(list(rng.values()))
for nm,sel in [('quiet days  (range < median)',lambda d: rng[d]<med),
               ('volatile days(range >= median)',lambda d: rng[d]>=med),
               ('first half of the corpus',lambda d: d<days[len(days)//2]),
               ('second half',lambda d: d>=days[len(days)//2])]:
    m=np.array([sel(r['d']) for r in rows])
    print('   %-32s AUC %.4f  (n=%d rows, base %.0f%%)'%(nm,roc_auc_score(y[m],o_m[m]),m.sum(),100*y[m].mean()))
