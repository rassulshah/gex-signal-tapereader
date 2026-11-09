import sys, numpy as np, pyarrow.parquet as pq
src = sys.argv[1]; f = pq.ParquetFile(src); CT = -6*3600*10**9
dt = np.dtype([('act','i1'),('side','i1'),('px','i4'),('sz','i4'),('oid','u8'),('t','i8')])
amap = {'A':0,'M':1,'C':2,'F':3,'R':9}; out = sys.stdout.buffer
for rg in range(f.num_row_groups):
    T = f.read_row_group(rg, columns=['ts','act','side','pt','sz','oid'])
    ts = T.column('ts').to_numpy()
    act = T.column('act').dictionary_encode() if False else T.column('act')
    a = np.array([amap.get(x,-1) for x in act.to_pylist()], dtype='i1'); sd = np.array(T.column('side').to_pylist())
    rec = np.zeros(len(ts), dtype=dt); rec['act']=a; rec['side']=np.where(sd=='B',0,np.where(sd=='A',1,-1)); rec['px']=T.column('pt').to_numpy(); rec['sz']=T.column('sz').to_numpy(); rec['oid']=T.column('oid').to_numpy(); rec['t']=ts//10**6
    hm = ((ts+CT)//10**9) % 86400
    keep = a >= 0
    stop = (hm >= 15*3600) & (hm < 16*3600)
    if stop.any():
        i = int(np.argmax(stop)); out.write(rec[:i][keep[:i]].tobytes()); break
    out.write(rec[keep].tobytes())
