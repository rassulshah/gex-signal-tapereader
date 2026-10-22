"""MBO -> binary for replay_sr: records (ts_ns i8, ct_sec i8, px_ticks i8, oid u8, sz i4, act u1, side u1, pad u2) = 40 bytes.
NQ: the raw parquet store (one contract). ES: Databento dbn.zst (front outright by trades). CDT = UTC-5 in Oct 2026."""
import sys, numpy as np
DT=np.dtype([('ts','<i8'),('ct','<i8'),('px','<i8'),('oid','<u8'),('sz','<i4'),('act','u1'),('side','u1'),('pad','<u2')])
TICK=0.25
def emit(f, ts, act, side, px, sz, oid):
    r=np.zeros(len(ts),DT); r['ts']=ts; r['ct']=ts//1_000_000_000-5*3600
    p=np.nan_to_num(px,nan=0.0); r['px']=np.rint(p/TICK).astype(np.int64); r['oid']=oid; r['sz']=sz; r['act']=act; r['side']=side
    f.write(r.tobytes())
def parquet(src,out):
    import pyarrow.parquet as pq
    pf=pq.ParquetFile(src); n=0
    with open(out,'wb') as f:
        for b in pf.iter_batches(batch_size=2_000_000, columns=['ts','act','side','px','sz','oid']):
            d=b.to_pydict() if False else None
            ts=b.column('ts').to_numpy(); act=np.frombuffer(np.array(b.column('act').to_pylist(),dtype='S1'),dtype='u1')
            side=np.frombuffer(np.array(b.column('side').to_pylist(),dtype='S1'),dtype='u1')
            emit(f,ts,act,side,b.column('px').to_numpy(zero_copy_only=False),b.column('sz').to_numpy(),b.column('oid').to_numpy()); n+=len(ts)
    print(out,n)
def dbn(src,out,root):
    import databento as db
    st=db.DBNStore.from_file(src); ids={}
    for sym,ivs in st.mappings.items():
        for iv in ivs: ids[int(iv['symbol'])]=sym
    cnt={}
    for a in st.to_ndarray(count=3_000_000):
        t=a['instrument_id'][a['action']==b'T']; u,c=np.unique(t,return_counts=True)
        for i,k in zip(u,c): cnt[int(i)]=cnt.get(int(i),0)+int(k)
    front=max((k for k in cnt if '-' not in ids.get(k,'-') and ids.get(k,'').startswith(root)),key=lambda k:cnt[k]); print('front',ids[front],cnt[front])
    n=0
    with open(out,'wb') as f:
        for a in st.to_ndarray(count=3_000_000):
            a=a[a['instrument_id']==front]
            emit(f,a['ts_recv'].astype('int64'),a['action'].view('u1'),a['side'].view('u1'),a['price'].astype('float64')/1e9*np.where(a['price']==np.iinfo(np.int64).max,np.nan,1),a['size'].astype('int32'),a['order_id']); n+=len(a)
    print(out,n,ids[front])
if __name__=='__main__':
    k=sys.argv[1]
    if k=='pq': parquet(sys.argv[2],sys.argv[3])
    else: dbn(sys.argv[2],sys.argv[3],sys.argv[4])
