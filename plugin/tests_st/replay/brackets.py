import pandas as pd, numpy as np, sys, glob
rows=[]
for f in sorted(glob.glob('rp/ev-*.csv')):
    tag=f.split('ev-')[1][:-4]; m=tag[:2]; tick={'ES':.25,'NQ':.25,'CL':.01,'GC':.1}[m]
    E=pd.read_csv(f)
    if E.empty: continue
    T=pd.read_csv(f'rp/{tag}.csv',header=None,names=['ms','sec','act','side','px','sz','oid']); T=T[T.act=='T']
    ms=T.ms.values; sec=T.sec.values; px=T.px.values
    for _,e in E.iterrows():
        atr=e.atr_ticks*tick
        if not atr>0: continue
        rev=1 if e.side=='down' else -1
        def first(i0,p0,k):
            seg=px[i0:]; 
            up=np.where(rev*(seg-p0)>=k*atr)[0]; dn=np.where(rev*(seg-p0)<=-k*atr)[0]
            a=up[0] if len(up) else 10**12; b=dn[0] if len(dn) else 10**12
            return 'rev' if a<b else ('cont' if b<a else 'none')
        i0=np.searchsorted(ms,e.fired_ms,side='right'); i1=np.searchsorted(sec,e.decided_t,side='left')
        r=dict(mkt=m,tag=tag,outcome=e.outcome,kind=e.ref_kind,ratio=e.ratio,
               f1=first(i0,e.st_px,1),f2=first(i0,e.st_px,2),
               d1=first(i1,e.decided_close,1) if i1<len(px) else 'none',d2=first(i1,e.decided_close,2) if i1<len(px) else 'none',
               rth=510<=e.minute<900)
        rows.append(r)
R=pd.DataFrame(rows); R.to_csv('rp/brackets_out.csv',index=False)
def tab(D,name):
    out=[]
    for (m,o),g in D.groupby(['mkt','outcome']):
        n=len(g)
        def pr(c): 
            x=g[c]; d=(x!='none').sum(); return f"{(x=='rev').sum()}/{d}={100*(x=='rev').sum()/max(d,1):.0f}%"
        out.append(f"{name} {m} {o:9s} n={n:4d}  rev-first 1ATR from St: {pr('f1'):>12s}  2ATR: {pr('f2'):>12s} | from decision 1ATR: {pr('d1'):>12s} 2ATR: {pr('d2'):>12s}")
    print('\n'.join(out))
tab(R,'ALL'); tab(R[R.rth],'RTH'); tab(R[R.kind==0],'KEY')
print(R.groupby(['tag']).agg(n=('outcome','size'),absorbed=('outcome',lambda x:(x=='absorbed').mean())).round(2).to_string())
