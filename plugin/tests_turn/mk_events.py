# raw Databento MBO (store) -> IRT-style order events: sec action buySell tick size oid aid  (sec = CT clock seconds since 1970, aligned like IRT)
import sys, pyarrow.parquet as pq, pyarrow.compute as pc, pandas as pd, numpy as np
m, day, t0s, t1s, out = sys.argv[1:6]
RAW=f'/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store/raw/m={m}/session={day}/part.parquet'
t0=pd.Timestamp(f'{day} {t0s}',tz='America/Chicago').value; t1=pd.Timestamp(f'{day} {t1s}',tz='America/Chicago').value
tb=pq.read_table(RAW,filters=[('ts','>=',t0),('ts','<',t1)])
d=tb.to_pandas(); d['act']=d.act.astype(str); d['side']=d.side.astype(str)
off=pd.Timestamp(f'{day} 12:00',tz='America/Chicago').utcoffset().total_seconds()
sec=(d.ts.values//10**9 + int(off)).astype(np.int64)
A=d.act.values; S=d.side.values; P=d.pt.values; Z=d.sz.values; O=d.oid.values.astype(np.int64)
ag_bs=2; ag_id=0; n=0
with open(out,'w') as f:
    for i in range(len(d)):
        a=A[i]
        if a=='A': f.write(f"{sec[i]} 0 {1 if S[i]=='B' else 0} {P[i]} {Z[i]} {O[i]} 0\n")
        elif a=='M': f.write(f"{sec[i]} 1 {1 if S[i]=='B' else 0} {P[i]} {Z[i]} {O[i]} 0\n")
        elif a=='C': f.write(f"{sec[i]} 2 {1 if S[i]=='B' else 0} {P[i]} {Z[i]} {O[i]} 0\n")
        elif a=='T': ag_bs = 1 if S[i]=='B' else 0 if S[i]=='A' else 2; ag_id=O[i] if ag_bs!=2 else 0
        elif a=='F' and O[i]!=ag_id: f.write(f"{sec[i]} 3 {ag_bs} {P[i]} {Z[i]} {O[i]} {ag_id}\n"); n+=1
print(m,len(d),'fills',n)
