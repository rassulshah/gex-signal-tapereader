import sys, subprocess, numpy as np, pandas as pd, pyarrow.parquet as pq
from zoneinfo import ZoneInfo
sys.path.insert(0, "../rev2"); import os; os.chdir("../rev2"); from an import prep; os.chdir("../sr")
SF="/mnt/user-data/uploads/level-reversal-analytics/data/sweep_feats"; ST="/mnt/user-data/uploads/OneDrive--LRA-Data/mbo-store/raw"; RTH={"GC":(440,750),"HG":(430,750),"CL":(480,810)}
def run(M):
    os.chdir("../rev2"); E,k,nrec=prep(M); os.chdir("../sr"); lv=pd.read_csv(f"{SF}/lv_{M}.csv.gz"); lv=lv[lv.kind.isin(["key","mq"])]; dd=pd.read_csv(f"{SF}/days_{M}.csv.gz").set_index("day")
    Ea=pd.read_pickle(f"../rev2/ev_{M}.pkl"); Ea=Ea[Ea.kind.isin(["key","mq"])&(Ea.sr_frozen==1)&(Ea.reclaimed==1)&(Ea.pen_atr<=1.6)&(Ea.b_cross_rec<=nrec)&(Ea.new_ext==1)]
    days=sorted(dd.index); a=b=c=0
    for day in days:
        t=pq.read_table(f"{ST}/m={M}/session={day}/part.parquet",columns=["ts","act","side","pt","sz","oid"],filters=[("act","in",["T"])]).to_pandas()
        off=int(pd.Timestamp(day+" 12:00",tz=ZoneInfo("America/Chicago")).utcoffset().total_seconds()); ns=t.ts.values+off*10**9-int(pd.Timestamp(day).value); sd=t.side.astype(str).values; m=sd!="N"
        pd.DataFrame(dict(k="F",t=(ns[m]//10**6)+86400000,px=t.pt.values[m],sz=t.sz.values[m],a=np.where(sd[m]=="B",1,-1),id=t.oid.values[m])).to_csv("ev.csv",header=False,index=False)
        l=lv[lv.day==day]; pd.DataFrame(dict(n=l.lvl.str.replace(",",";"),p=l.p,s=l.side,f=l.ta+86400)).to_csv("lv.csv",header=False,index=False); d=dd.loc[day]
        out=subprocess.run(["./srrun","ev.csv","lv.csv",str(RTH[M][0]*60),str(RTH[M][1]*60),str(d.atr),str(nrec)],capture_output=True,text=True).stdout.splitlines()
        V={(x.split("|")[3],(int(x.split("|")[2])-86400)//180-1) for x in out if x.startswith("V|")}
        e=Ea[Ea.day==day]; s=dd.loc[day]; P={(n,int(b0)+int(q)-int(i0)) for n,b0,q,i0 in zip(e.lvl,e.bar0,e.q,e.i0)}
        a+=len(P); b+=len(V); c+=len(P&V)
        if P!=V: print(day,"study only",sorted(P-V)[:3],"engine only",sorted(V-P)[:3])
    print(M,"reversal arrows: study",a,"engine",b,"same",c)
for M in sys.argv[1:]: run(M)
