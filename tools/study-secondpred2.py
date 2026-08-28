#!/usr/bin/env python3
"""
SECOND PASS at the opposite extremity, after the first pass tested ONE framing and stopped.

    "did you try your best ... different factors, combinations, models"  - operator, 2026-08-28

WHAT IS NEW HERE
  1. THE OVERNIGHT SESSION. The corpus is 24-hour and every study so far threw the ETH bars away.
  2. VOLUME. Present in the file since the first study; never used.
  3. LEVELS: distance from the call to the PRIOR DAY's high/low/close, and to the ON high/low.
  4. PRICE AS A SURVIVAL CURVE - P(the far side reaches X) - instead of an interval.
  5. TIME AS A HAZARD - P(it prints in the next 60 min | it has not yet), per bar, with features,
     against the clock-only baseline. Time was only ever tested as a duration + quantile interval.

⚠ Every model is scored out-of-fold, grouped BY SESSION. Every one is compared against the
baseline that already exists (the unconditional distribution / the clock alone). A model that does
not beat that has learned to read a clock - F-1 and F-6 both died that way.
"""
import csv, io, collections, json, sys
import numpy as np
from sklearn.model_selection import GroupKFold
from sklearn.ensemble import GradientBoostingClassifier, GradientBoostingRegressor
from sklearn.metrics import roc_auc_score, brier_score_loss

RTH_A, RTH_B = 8*3600+30*60, 15*3600
ETH_A = 17*3600          # prior 17:00 CT
HLTAB=[[4,7,8,9,12,15,20,28,47],[10,18,21,33,36,39,57,66,84],[16,26,34,49,56,65,68,84,96],
 [22,33,50,55,65,78,85,95,99],[27,45,62,67,73,81,91,97,100],[32,58,72,75,84,88,95,98,100],
 [40,59,76,86,89,95,98,99,100],[47,74,84,90,96,99,99,100,100]]
def cell(p,m): return HLTAB[max(0,min(7,int(p*8)))][max(0,min(8,int(m//45)))]

def load24(path):
    """the WHOLE day, not just RTH - that is the point of this pass."""
    ses=collections.defaultdict(list)
    with io.open(path,encoding='utf-8',errors='replace') as f:
        first=f.readline(); f.seek(0)
        tab=first.count('\t')>first.count(',')
        sep='\t' if tab else ','
        hdr='Open' in first or 'Date' in first
        cols=None
        if hdr:
            cols=[c.strip() for c in next(csv.reader([first],delimiter=sep))]; f.readline()
        for parts in csv.reader(f,delimiter=sep):
            if len(parts)<6: continue
            try:
                if cols:
                    r=dict(zip(cols,parts)); stamp=(r.get('Date') or r.get('DateTime') or '').strip('"')
                    o,h,l,c=float(r['Open']),float(r['High']),float(r['Low']),float(r['Close'])
                    v=float(r.get('Volume') or r.get('VOL') or 0)
                else:
                    stamp=parts[1].strip('"'); o,h,l,c=map(float,parts[2:6])
                    v=float(parts[6]) if len(parts)>6 else 0.0
                d,t=(stamp.split('T',1) if 'T' in stamp else stamp.split(' ',1))
                pt=t.split(':'); sec=int(pt[0])*3600+int(pt[1])*60
                ses[d].append((sec,o,h,l,c,v))
            except (ValueError,IndexError,KeyError): continue
    return {d:sorted(v) for d,v in ses.items()}

def build(path, th=85):
    raw=load24(path)
    days=sorted(raw)
    out=[]; bars_rows=[]
    prev=None
    for di,d in enumerate(days):
        allb=raw[d]
        rth=[b for b in allb if RTH_A<=b[0]<=RTH_B]
        if len(rth)<386: 
            prev=None if not rth else dict(rng=max(x[2] for x in rth)-min(x[3] for x in rth),
                                           close=rth[-1][4], hi=max(x[2] for x in rth), lo=min(x[3] for x in rth))
            continue
        # OVERNIGHT: this date's bars before the RTH open (00:00-08:29) + the prior date's >=17:00
        on_today=[b for b in allb if b[0]<RTH_A]
        on_prev=[b for b in raw.get(days[di-1],[]) if b[0]>=ETH_A] if di>0 else []
        on=on_prev+on_today
        onH=max((x[2] for x in on), default=np.nan); onL=min((x[3] for x in on), default=np.nan)
        onV=sum(x[5] for x in on) if on else np.nan
        op=rth[0][1]
        fl=min(x[3] for x in rth); fh=max(x[2] for x in rth)
        flT=min(x[0] for x in rth if x[3]==fl); fhT=min(x[0] for x in rth if x[2]==fh)
        ib30h=max(x[2] for x in rth[:30]); ib30l=min(x[3] for x in rth[:30])
        closes=[x[4] for x in rth]
        hi=lo=None;hiT=loT=None;cum=0.0;call=None
        for i,(sec,o,h,l,c,v) in enumerate(rth):
            cum+=v
            if hi is None or h>hi: hi,hiT=h,sec
            if lo is None or l<lo: lo,loT=l,sec
            mins=(sec-RTH_A)/60.
            if mins<5: continue
            rng=hi-lo
            if rng<=0: continue
            first='LOD' if loT<hiT else 'HOD'
            posr=max(0.,min(1.,(c-lo)/rng if first=='LOD' else (hi-c)/rng))
            if call is None and cell(posr,mins)>=th:
                seg=np.array(closes[:i+1])
                eff=abs(seg[-1]-seg[0])/max(1e-9,np.sum(np.abs(np.diff(seg))))
                call=dict(day=d, i=i, mins=mins, callT=sec, px=c, first=first, rng=rng, posr=posr,
                          took=((loT if first=='LOD' else hiT)-RTH_A)/60.,
                          wickPct=abs(op-(lo if first=='LOD' else hi))/rng,
                          fromOpen=(c-op)/rng, eff=float(eff),
                          rv=float(np.std(np.diff(seg))) if i>5 else 0.0,
                          minsLeft=(RTH_B-sec)/60., vol=cum,
                          onRng=(onH-onL) if on else np.nan,
                          onRngRel=((onH-onL)/rng) if on and rng>0 else np.nan,
                          onPos=((c-onL)/(onH-onL)) if on and onH>onL else np.nan,
                          ib30=(ib30h-ib30l), ib30Rel=(ib30h-ib30l)/rng if rng>0 else np.nan,
                          dow=float(np.datetime64(d,'D').astype('datetime64[D]').astype(int)%7),
                          prevRng=(prev['rng'] if prev else np.nan),
                          gap=((op-prev['close'])/prev['rng']) if prev and prev['rng']>0 else np.nan,
                          dPDH=((prev['hi']-c)/rng) if prev and rng>0 else np.nan,
                          dPDL=((c-prev['lo'])/rng) if prev and rng>0 else np.nan,
                          # ⚠⚠ THE RUNNING EXTREMES AS AT THE CALL. The first cut read `lo`/`hi`
                          # AFTER the loop - i.e. the FINAL session extremes - so `held` was
                          # always 1 and the "first extreme held" filter was a no-op that silently
                          # kept every failed call in the sample. Capture the state at the moment
                          # the decision is taken, never after it.
                          callLo=lo, callHi=hi)
            if call is not None:
                # PER-BAR HAZARD ROWS, every 5 minutes after the call
                if (i-call['i'])%5==0:
                    side='HOD' if call['first']=='LOD' else 'LOD'
                    oppT = fhT if side=='HOD' else flT
                    if oppT<=sec:      # already printed -> stop emitting
                        pass
                    else:
                        curFar = hi if side=='HOD' else lo
                        bars_rows.append(dict(day=d, sec=sec, mins=mins, elapsed=(sec-call['callT'])/60.,
                            minsLeft=(RTH_B-sec)/60., rng=hi-lo, posr=posr,
                            distFar=abs(c-curFar)/max(1e-9,(hi-lo)),
                            eff=call['eff'], rv=call['rv'], onRngRel=call['onRngRel'],
                            volRate=cum/max(1.,mins), ib30Rel=call['ib30Rel'], gap=call['gap'],
                            dow=call['dow'], took=call['took'],
                            y60=1 if oppT<=sec+3600 else 0,
                            yClose=1 if oppT>=RTH_B-45*60 else 0))
        if call:
            side='HOD' if call['first']=='LOD' else 'LOD'
            oppT,oppPx=(fhT,fh) if side=='HOD' else (flT,fl)
            held=(call['callLo']<=fl+1e-9) if call['first']=='LOD' else (call['callHi']>=fh-1e-9)
            ext=(oppPx-call['px']) if side=='HOD' else (call['px']-oppPx)
            call.update(side=side,oppT=oppT,oppPx=oppPx,held=1 if held else 0,
                        ahead=1 if oppT>call['callT'] else 0, tmin=(oppT-call['callT'])/60.,
                        finalRng=fh-fl, expMult=(fh-fl)/call['rng'])
            out.append(call)
        prev=dict(rng=fh-fl, close=rth[-1][4], hi=fh, lo=fl)
    return out, bars_rows

def auc_report(name, X, y, g, feats, base_feats):
    def run(F):
        idx=[feats.index(f) for f in F]
        p=np.zeros(len(y))
        for tr,te in GroupKFold(n_splits=5).split(X,y,g):
            m=GradientBoostingClassifier(n_estimators=140,max_depth=2,learning_rate=.06,
                                         random_state=7).fit(X[tr][:,idx],y[tr])
            p[te]=m.predict_proba(X[te][:,idx])[:,1]
        return roc_auc_score(y,p), brier_score_loss(y,p), p
    a0,b0,_=run(base_feats); a1,b1,p1=run(feats)
    print('   %-34s baseline(clock) AUC %.4f Brier %.4f  |  all factors AUC %.4f Brier %.4f  (%+0.4f)'
          % (name,a0,b0,a1,b1,a1-a0))
    return p1

if __name__=='__main__':
    TH=int(sys.argv[1]) if len(sys.argv)>1 else 85
    S,B=build('data/es-1min/ES TestingData.txt', TH)
    A=[r for r in S if r['ahead'] and r['held']]
    print('call >=%d%%: %d sessions · %d with the far side ahead and the first extreme holding'%(TH,len(S),len(A)))
    print('   overnight range available on %d of them' % sum(1 for r in A if r['onRngRel']==r['onRngRel']))

    print('\nA · PRICE AS A SURVIVAL CURVE — P(the far side extends at least X x the range so far)')
    FEATS=['mins','posr','rng','took','wickPct','fromOpen','eff','rv','minsLeft','vol',
           'onRngRel','onPos','ib30Rel','dow','prevRng','gap','dPDH','dPDL']
    X=np.array([[np.nan_to_num(r[f],nan=0.0) for f in FEATS] for r in A])
    g=np.array([r['day'] for r in A])
    for xmult in (1.15,1.3,1.5,1.8):
        y=np.array([1 if r['expMult']>=xmult else 0 for r in A])
        if y.mean() in (0,1) or y.sum()<20: continue
        p=np.zeros(len(y))
        for tr,te in GroupKFold(n_splits=5).split(X,y,g):
            m=GradientBoostingClassifier(n_estimators=140,max_depth=2,learning_rate=.06,
                                         random_state=7).fit(X[tr],y[tr])
            p[te]=m.predict_proba(X[te])[:,1]
        print('   final range >= %.2fx range-so-far   base %3.0f%%   AUC %.3f   Brier %.3f'
              % (xmult,100*y.mean(),roc_auc_score(y,p),brier_score_loss(y,p)))

    print('\nB · TIME AS A HAZARD — per bar, every 5 min after the call (n=%d bar-rows)'%len(B))
    BF=['mins','elapsed','minsLeft','rng','posr','distFar','eff','rv','onRngRel','volRate',
        'ib30Rel','gap','dow','took']
    XB=np.array([[np.nan_to_num(r[f],nan=0.0) for f in BF] for r in B])
    gB=np.array([r['day'] for r in B])
    y60=np.array([r['y60'] for r in B]); yC=np.array([r['yClose'] for r in B])
    auc_report('prints within the NEXT 60 MIN', XB,y60,gB,BF,['mins','minsLeft'])
    auc_report('prints in the LAST 45 MIN of the day', XB,yC,gB,BF,['mins','minsLeft'])
