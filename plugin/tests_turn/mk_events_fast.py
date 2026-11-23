# raw Databento MBO (store) -> IRT-style order events, as tests_turn/mk_events.py (same lines), vectorised
import sys, pyarrow.parquet as pq, pandas as pd, numpy as np
m, day, t0s, t1s, out = sys.argv[1:6]
RAW=f'/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store/raw/m={m}/session={day}/part.parquet'
t0=pd.Timestamp(f'{day} {t0s}',tz='America/Chicago').value; t1=pd.Timestamp(f'{day} {t1s}',tz='America/Chicago').value
d=pq.read_table(RAW,filters=[('ts','>=',t0),('ts','<',t1)],columns=['ts','act','side','pt','sz','oid']).to_pandas()
d['act']=d.act.astype(str); d['side']=d.side.astype(str)
off=pd.Timestamp(f'{day} 12:00',tz='America/Chicago').utcoffset().total_seconds()
sec=(d.ts.values//10**9 + int(off)).astype(np.int64)
A=d.act.values; S=d.side.values; O=d.oid.values.astype(np.int64)
isT=A=='T'
# the aggressor of each row = the last T row before it
tbs=np.where(S=='B',1,np.where(S=='A',0,2)); tid=np.where(tbs!=2,O,0)
idx=np.where(isT,np.arange(len(d)),-1); idx=np.maximum.accumulate(idx)
ag_bs=np.where(idx>=0,tbs[np.maximum(idx,0)],2); ag_id=np.where(idx>=0,tid[np.maximum(idx,0)],0)
act=np.full(len(d),-1); act[A=='A']=0; act[A=='M']=1; act[A=='C']=2; act[(A=='F')&(O!=ag_id)]=3
keep=act>=0
bs=np.where(act==3,ag_bs,np.where(S=='B',1,0)); aid=np.where(act==3,ag_id,0)
o=pd.DataFrame({'sec':sec[keep],'act':act[keep],'bs':bs[keep],'pt':d.pt.values[keep],'sz':d.sz.values[keep],'oid':O[keep],'aid':aid[keep]})
o.to_csv(out,sep=' ',header=False,index=False)
print(m,len(d),'lines',len(o),'fills',int((o.act==3).sum()))
