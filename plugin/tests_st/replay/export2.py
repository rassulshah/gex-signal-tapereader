# export T / F rows of one market-session (LRA raw MBO store) + session levels (PDH/PDL prior RTH, ONH/ONL, LonHI/LonLO from 08:30)
# + MenthorQ EOD levels for that session (CR PS HVL GW0 CR0 PS0 HVL0 1DH 1DL; available from the start)
import duckdb, sys, pandas as pd
m, s, prev = sys.argv[1], sys.argv[2], sys.argv[3]
R='/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store/raw'
c=duckdb.connect()
def load(sess):
    return c.execute(f"select ts, act, side, px, sz, oid from read_parquet('{R}/m={m}/session={sess}/part.parquet', file_row_number=true) where act in ('T','F') order by file_row_number").df()
d=load(s)
d['ct']=pd.to_datetime(d.ts).dt.tz_localize('UTC').dt.tz_convert('America/Chicago')
d['sec']=(d.ct.dt.tz_localize(None).astype('int64')//10**9)
d['ms']=d.ts//10**6
d[['ms','sec','act','side','px','sz','oid']].to_csv(f'{m}-{s}.csv',index=False,header=False)
p=c.execute(f"select ts, px from read_parquet('{R}/m={m}/session={prev}/part.parquet') where act='T'").df()
p['ct']=pd.to_datetime(p.ts).dt.tz_localize('UTC').dt.tz_convert('America/Chicago').dt.tz_localize(None)
rth=p[(p.ct.dt.time>=pd.Timestamp('08:30').time())&(p.ct.dt.time<pd.Timestamp('15:00').time())&(p.ct.dt.date==pd.Timestamp(prev).date())]
tt=d[d.act=='T'].copy(); tt['c']=tt.ct.dt.tz_localize(None)
on=tt[tt.c<pd.Timestamp(s+' 08:30')]
lon=tt[(tt.c>=pd.Timestamp(s+' 02:00'))&(tt.c<pd.Timestamp(s+' 08:30'))]
e=int(pd.Timestamp(s+' 08:30').value//10**9)
L=[('PDH',rth.px.max(),0),('PDL',rth.px.min(),0),('ONH',on.px.max(),e),('ONL',on.px.min(),e),('LonHI',lon.px.max(),e),('LonLO',lon.px.min(),e)]
mq=pd.read_csv('/mnt/user-data/uploads/level-reversal-analytics/data/menthorq/history/levels_2026-10.csv')
q=mq[(mq.market==m)&(mq.kind=='eod')&(mq.for_session==s)&(mq.code.isin(['CR','PS','HVL','GW0','CR0','PS0','HVL0','1DH','1DL']))]
L+= [(r.code,r.price,0) for r in q.itertuples()]
with open(f'{m}-{s}.lv','w') as f:
    for n,v,t in L:
        if v==v: f.write(f"{n} {v} {t}\n")
print(m,s,len(d),'levels',len(L),'mq',len(q))
