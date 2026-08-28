#!/usr/bin/env python3
"""
CAN WE PREDICT THE OPPOSITE EXTREMITY - ITS PRICE AND ITS CLOCK - ONCE THE FIRST ONE IS IN?

    "HOD expected around 7772-7792 in 3.5 Hrs between 1:30pm and 2pm - 80%"
    "you can also give different probabilities for the HOD time and the HOD price range"

⚠ ONE ROW PER SESSION, at the first bar the table crosses the chosen threshold, on the side the
panel would ACTUALLY be calling in real time (the running first-printed extreme). No hindsight side
selection - that is what inflated the shipped 92%.
⚠ EVERY interval is fitted out-of-fold (GroupKFold by session date) and scored by COVERAGE on the
folds it did not see. A quantile fitted and scored on the same rows is not evidence.
⚠ THE BASELINE IS THE SAME WINDOW EVERY DAY. Conditioning has to beat that or it is decoration.
"""
import sys, json, collections
import numpy as np
from importlib.machinery import SourceFileLoader
from sklearn.model_selection import GroupKFold
from sklearn.ensemble import GradientBoostingRegressor
M = SourceFileLoader('m', 'tools/model-lodhod.py').load_module()

OPEN, CLOSE = 8*3600+30*60, 15*3600
HLTAB=[[4,7,8,9,12,15,20,28,47],[10,18,21,33,36,39,57,66,84],[16,26,34,49,56,65,68,84,96],
 [22,33,50,55,65,78,85,95,99],[27,45,62,67,73,81,91,97,100],[32,58,72,75,84,88,95,98,100],
 [40,59,76,86,89,95,98,99,100],[47,74,84,90,96,99,99,100,100]]
def cell(p,m): return HLTAB[max(0,min(7,int(p*8)))][max(0,min(8,int(m//45)))]

def sessions_rows(S, th):
    days=sorted(S); out=[]; prev=None
    for d in days:
        b=S[d]
        fl=min(x[3] for x in b); fh=max(x[2] for x in b)
        flT=min(x[0] for x in b if x[3]==fl); fhT=min(x[0] for x in b if x[2]==fh)
        op=b[0][1]; closes=[x[4] for x in b]
        hi=lo=None; hiT=loT=None; row=None
        for i,(sec,o,h,l,c) in enumerate(b):
            if hi is None or h>hi: hi,hiT=h,sec
            if lo is None or l<lo: lo,loT=l,sec
            mins=(sec-OPEN)/60.
            if mins<5: continue
            rng=hi-lo
            if rng<=0: continue
            first='LOD' if loT<hiT else 'HOD'
            posr=max(0.,min(1.,(c-lo)/rng if first=='LOD' else (hi-c)/rng))
            if cell(posr,mins)>=th:
                rets=np.diff(np.array(closes[:i+1]))
                row=dict(day=d, mins=mins, callT=sec, px=c, first=first, rng=rng, posr=posr,
                         took=((loT if first=='LOD' else hiT)-OPEN)/60.,
                         wickPct=abs(op-(lo if first=='LOD' else hi))/rng,
                         fromOpen=(c-op)/rng,
                         rv=float(np.std(rets)) if len(rets)>5 else 0.0,
                         minsLeft=(CLOSE-sec)/60.,
                         prevRng=(prev['rng'] if prev else np.nan),
                         gap=((op-prev['close'])/prev['rng']) if prev and prev['rng']>0 else np.nan)
                break
        if row:
            # what actually happened on the OTHER side, for the whole session
            if row['first']=='LOD': oppT,oppPx,side=fhT,fh,'HOD'
            else:                   oppT,oppPx,side=flT,fl,'LOD'
            heldFirst = (lo<=fl+1e-9) if row['first']=='LOD' else (hi>=fh-1e-9)
            row.update(side=side, oppT=oppT, oppPx=oppPx, held=1 if heldFirst else 0,
                       ahead=1 if oppT>row['callT'] else 0,
                       tmin=(oppT-row['callT'])/60.,
                       moveR=((oppPx-row['px']) if side=='HOD' else (row['px']-oppPx))/row['rng'],
                       movePts=abs(oppPx-row['px']),
                       oppClock=oppT)
            out.append(row)
        prev=dict(rng=fh-fl, close=b[-1][4])
    return out

FE=['mins','posr','rng','took','wickPct','fromOpen','rv','minsLeft','prevRng','gap']

def oof_quantiles(rows, target, qs, feats=FE, folds=5):
    X=np.array([[np.nan_to_num(r[f], nan=0.0) for f in feats] for r in rows])
    y=np.array([r[target] for r in rows]); g=np.array([r['day'] for r in rows])
    out={q: np.zeros(len(y)) for q in qs}; base={q: np.zeros(len(y)) for q in qs}
    for tr,te in GroupKFold(n_splits=folds).split(X,y,g):
        for q in qs:
            m=GradientBoostingRegressor(loss='quantile', alpha=q, n_estimators=160,
                                        max_depth=2, learning_rate=0.06, subsample=0.9,
                                        random_state=7).fit(X[tr],y[tr])
            out[q][te]=m.predict(X[te])
            base[q][te]=np.quantile(y[tr],q)
    return y,out,base

def report(name, y, mdl, base, lo, hi, unit=''):
    cov_m=float(np.mean((y>=mdl[lo])&(y<=mdl[hi]))); w_m=float(np.median(mdl[hi]-mdl[lo]))
    cov_b=float(np.mean((y>=base[lo])&(y<=base[hi]))); w_b=float(np.median(base[hi]-base[lo]))
    print('   %-22s model: cov %3.0f%%  median width %6.1f%s   |  same-window-every-day: cov %3.0f%%  width %6.1f%s'
          % (name, 100*cov_m, w_m, unit, 100*cov_b, w_b, unit))
    return cov_m, w_m, cov_b, w_b

def main():
    S=M.load('data/es-1min/ES TestingData.txt')
    print('THE TRADEOFF: how late the call fires, how often it is right IN REAL TIME,')
    print('and whether the far side is still ahead when it does.\n')
    print('   %-6s %5s %9s %11s %14s' % ('thresh','n','correct','median CT','far side ahead'))
    keep={}
    for th in (70,80,85,90,95):
        rows=sessions_rows(S,th)
        acc=np.mean([r['held'] for r in rows]); med=OPEN+int(np.median([r['mins'] for r in rows]))*60
        ahead=np.mean([r['ahead'] for r in rows])
        print('   %-6s %5d %8.0f%% %11s %13.0f%%' % ('>=%d%%'%th, len(rows), 100*acc,
              '%d:%02d'%(med//3600,(med%3600)//60), 100*ahead))
        keep[th]=rows
    for th in (70,90):
        rows=[r for r in keep[th] if r['ahead'] and r['held']]
        print('\n=== CALL AT >=%d%%  ·  n=%d sessions where the call was right and the far side was ahead ===' % (th,len(rows)))
        med=OPEN+int(np.median([r['mins'] for r in rows]))*60
        print('    median call time %d:%02d · median minutes to the opposite extreme %.0f'
              % (med//3600,(med%3600)//60, np.median([r['tmin'] for r in rows])))
        for tgt,unit,qs in (('tmin',' min',(0.10,0.25,0.75,0.90)),
                            ('moveR',' x',(0.10,0.25,0.75,0.90)),
                            ('oppClock',' s',(0.10,0.25,0.75,0.90))):
            if tgt=='oppClock': continue
            y,mdl,base=oof_quantiles(rows,tgt,qs)
            print('   target = %s' % ({'tmin':'MINUTES to the opposite extreme','moveR':'MOVE to it, x range so far'}[tgt]))
            report('  nominal 80%',y,mdl,base,0.10,0.90,unit)
            report('  nominal 50%',y,mdl,base,0.25,0.75,unit)
    json.dump({str(k):[{kk:(vv if not isinstance(vv,(np.floating,np.integer)) else float(vv)) for kk,vv in r.items()} for r in v]
               for k,v in keep.items()}, open('/tmp/secondpred_rows.json','w'), default=float)
if __name__=='__main__':
    main()
