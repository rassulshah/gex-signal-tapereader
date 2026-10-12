"""explore.py -- lsCVD 1.1.0 rule choice: features per confirmed signal, choose on the first half of sessions, test on the second."""
import pandas as pd, numpy as np, warnings; warnings.filterwarnings('ignore')
from study import outcome, TICK
def features(outdir='out'):
    rows=[]
    for m,t in TICK.items():
        b=pd.read_csv(f'{outdir}/{m}_bars.csv'); e=pd.read_csv(f'{outdir}/{m}_events.csv'); idx={r:i for i,r in enumerate(b.raw)}
        H=b.h.values; L=b.l.values
        for _,x in e.iterrows():
            if x.state!='confirmed': continue
            i=idx[x.raw]; side=1 if x.dir=='bull' else -1
            rng=np.median(H[max(0,i-20):i]-L[max(0,i-20):i]) if i>0 else np.nan
            risk=abs(b.c.iat[i]-x.invalidation)/rng if rng>0 else np.nan
            tod=(x.raw%86400)/3600
            rows.append(dict(m=m,type=x.type,dir=x.dir,session=b.session.iat[i],tod=tod,lv=x.level_id.split('@')[0],risk=risk,
                             r=outcome(b,i,side,b.c.iat[i],x.invalidation,t)))
    d=pd.DataFrame(rows).dropna(subset=['r','risk'])
    d['pd']=d.lv.str.contains('PD|PF'); return d
def s(g): 
    t=g.r.mean()/(g.r.std(ddof=1)/np.sqrt(len(g))) if len(g)>2 else np.nan
    return f"n={len(g):3d} avgR={g.r.mean():+.2f} win={(g.r>0).mean():.0%} t={t:+.1f}"
if __name__=='__main__':
    d=features(); cut=np.median(sorted(d.session.unique()))
    A=d[d.session<=cut]; B=d[d.session>cut]
    print('all          A',s(A),' | B',s(B))
    cap=A.risk.quantile(2/3); print('risk cap (A 2/3 quantile, x median bar range):',round(cap,2))
    rules={'prior-day/session levels':lambda x:x.pd,'stop <= cap':lambda x:x.risk<=cap,'Div only':lambda x:x.type=='Div',
           'Div+BR':lambda x:x.type!='Exh','levels & cap':lambda x:x.pd&(x.risk<=cap),'levels & cap & no Exh':lambda x:x.pd&(x.risk<=cap)&(x.type!='Exh')}
    for k,f in rules.items(): print(f'{k:24s} A',s(A[f(A)]),' | B',s(B[f(B)]))
    for k,f in rules.items(): print(f'{k:24s} ALL',s(d[f(d)]))
