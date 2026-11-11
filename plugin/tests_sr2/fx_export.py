import sys, numpy as np, pandas as pd, pyarrow.parquet as pq, struct
from zoneinfo import ZoneInfo
M, day, h0, h1, out = sys.argv[1], sys.argv[2], float(sys.argv[3]), float(sys.argv[4]), sys.argv[5]
SF="/mnt/user-data/uploads/level-reversal-analytics/data/sweep_feats"; fp=f"/mnt/user-data/uploads/OneDrive--LRA-Data/mbo-store/raw/m={M}/session={day}/part.parquet"
t=pq.read_table(fp,columns=["ts","act","side","pt","sz","oid"]).to_pandas(); off=int(pd.Timestamp(day+" 12:00",tz=ZoneInfo("America/Chicago")).utcoffset().total_seconds())
t["sec"]=(t.ts.values+off*10**9-int(pd.Timestamp(day).value))//10**9; t["act"]=t.act.astype(str); t["side"]=t.side.astype(str); t["seq"]=np.arange(len(t))
t=t[(t.sec>=h0*3600)&(t.sec<h1*3600)]
T=t[(t.act=="T")&(t.side!="N")][["ts","seq","pt","oid","side"]].rename(columns={"oid":"aggr","side":"ag"}); F=t[t.act=="F"].copy()
F=pd.merge_asof(F.sort_values("seq"),T.sort_values("seq")[["seq","aggr","ts","ag"]].rename(columns={"ts":"ts_t"}),on="seq",direction="backward"); F=F[(F.ts==F.ts_t)&(((F.side=="B")&(F.ag=="A"))|((F.side=="A")&(F.ag=="B")))]
last=pd.merge_asof(t[t.act.isin(["A","C"])].sort_values("seq"),T.sort_values("seq")[["seq","pt"]].rename(columns={"pt":"lp"}),on="seq",direction="backward")
B=last[(last.lp.notna())&((last.pt-last.lp).abs()<=40)&(last.side.isin(["A","B"]))]
DB=20000*86400
rows=[(int(r.seq),DB+int(r.sec),3,0 if r.side=="B" else 1,int(r.pt),int(r.sz),int(r.oid%2147483647),int(r.aggr%2147483647)) for r in F.itertuples()]
rows+=[(int(r.seq),DB+int(r.sec),0 if r.act=="A" else 2,0 if r.side=="B" else 1,int(r.pt),int(r.sz),int(r.oid%2147483647),0) for r in B.itertuples()]
rows.sort()
with open(out,"wb") as f:
    for r in rows: f.write(struct.pack("<ibbiIii",r[1],r[2],r[3],r[4],r[5],r[6],r[7]))
lv=pd.read_csv(f"{SF}/lv_{M}.csv.gz"); lv=lv[(lv.day==day)&(lv.kind=="key")]
names=set()
with open(out+".levels","w") as f:
    for r in lv.itertuples():
        for n in r.lvl.split("+"):
            if n.startswith("swing") or n in names: continue
            names.add(n); f.write(f"{n}|{int(r.p)}\n")
print(M,day,"rows",len(rows),"fills",len(F),"book",len(B),"levels",sorted(names))
