# (src parquet with ts/act/side/px/sz/oid, optional prior-day parquet) -> replay CSV + levels (PDH/PDL prior RTH, ONH/ONL, LonHI/LonLO)
import duckdb, sys, pandas as pd, os
src, prev, out, day = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
c=duckdb.connect()
d=c.execute(f"select ts, act, side, px, sz, oid from read_parquet('{src}', file_row_number=true) where act in ('T','F') order by file_row_number").df()
ct=pd.to_datetime(d.ts).dt.tz_localize('UTC').dt.tz_convert('America/Chicago').dt.tz_localize(None)
d['sec']=ct.astype('int64')//10**9; d['ms']=d.ts//10**6
d[['ms','sec','act','side','px','sz','oid']].to_csv(out+'.csv',index=False,header=False)
lv=[]
T=d[d.act=='T'].assign(c=ct[d.act=='T'])
o830=pd.Timestamp(day+' 08:30'); e=int(o830.value//10**9)
on=T[T.c<o830]; lon=T[(T.c>=pd.Timestamp(day+' 02:00'))&(T.c<o830)]
if len(on): lv+= [('ONH',on.px.max(),e),('ONL',on.px.min(),e)]
if len(lon): lv+= [('LonHI',lon.px.max(),e),('LonLO',lon.px.min(),e)]
if prev!='-' and os.path.exists(prev):
    p=c.execute(f"select ts, px from read_parquet('{prev}') where act='T'").df()
    pc=pd.to_datetime(p.ts).dt.tz_localize('UTC').dt.tz_convert('America/Chicago').dt.tz_localize(None)
    pd_=(pd.Timestamp(day)-pd.Timedelta(days=1)); 
    while pd_.weekday()>4: pd_-=pd.Timedelta(days=1)
    r=p[(pc>=pd_+pd.Timedelta('8h30min'))&(pc<pd_+pd.Timedelta('15h'))]
    if len(r): lv+= [('PDH',r.px.max(),0),('PDL',r.px.min(),0)]
open(out+'.lv','w').write(''.join(f"{n} {v} {t}\n" for n,v,t in lv))
print(out,len(d),lv)
