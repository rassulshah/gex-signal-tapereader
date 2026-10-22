# replay_trails_feed.py (2026-10-22, MoveLogic 1.2.0): streams a raw Databento MBO parquet (store/raw/m=<MKT>/session=<day>/part.parquet) to replay_trails on stdin, in file order, low memory
import pyarrow.parquet as pq, numpy as np, sys
pf=pq.ParquetFile(sys.argv[1]); out=sys.stdout.buffer
dt=np.dtype([('ts','<i8'),('act','i1'),('side','i1'),('sz','<i4'),('tk','<i8'),('oid','<u8')])
for b in pf.iter_batches(batch_size=500000, columns=['ts','act','side','px','sz','oid']):
    d=b.to_pandas()
    a=np.zeros(len(d),dtype=dt)
    a['ts']=d.ts.values; a['act']=d.act.fillna('?').map(ord).values.astype('i1'); a['side']=d.side.fillna('N').map(ord).values.astype('i1')
    a['sz']=d.sz.fillna(0).values.astype('<i4'); a['tk']=np.round(d.px.fillna(0).values/0.25).astype('<i8'); a['oid']=d.oid.fillna(0).values.astype('<u8')
    out.write(a.tobytes())
