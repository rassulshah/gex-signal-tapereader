import databento as db, numpy as np, pandas as pd, sys
f=sys.argv[1]; day=sys.argv[2]; out=sys.argv[3]
st=db.DBNStore.from_file(f)
ids={}
for sym,ivs in st.mappings.items():
    for iv in ivs:
        ids[int(iv['symbol'])]=sym
parts=[]
for a in st.to_ndarray(count=2_000_000):
    a=a[(a['action']==b'T')|(a['action']==b'F')]
    parts.append(a[['ts_recv','ts_event','instrument_id','action','side','price','size','order_id']].copy())
A=np.concatenate(parts)
cnt=pd.Series(A['instrument_id'][A['action']==b'T']).value_counts()
front=None
for iid,n in cnt.items():
    s=ids.get(int(iid),'?')
    if '-' not in s and s.startswith('ES'): front=iid; print('front',s,n); break
A=A[A['instrument_id']==front]
d=pd.DataFrame({'ts':A['ts_recv'].astype('int64'),'act':A['action'].astype(str),'side':A['side'].astype(str),'px':A['price']/1e9,'sz':A['size'],'oid':A['order_id']})
d.to_parquet(out); print(day,len(d))
