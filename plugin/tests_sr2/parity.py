import sys, os, subprocess, numpy as np, pandas as pd, pyarrow.parquet as pq
from zoneinfo import ZoneInfo
SF="/mnt/user-data/uploads/level-reversal-analytics/data/sweep_feats"; ST="/mnt/user-data/uploads/OneDrive--LRA-Data/mbo-store/raw"
RTH={"GC":(440,750),"HG":(430,750),"CL":(480,810)}
def run(M, days):
    lv=pd.read_csv(f"{SF}/lv_{M}.csv.gz"); ep=pd.read_csv(f"{SF}/ep_{M}.csv.gz"); dd=pd.read_csv(f"{SF}/days_{M}.csv.gz").set_index("day"); tot=dict(py=0,cx=0,both=0,pygold=0,cxgold=0,bothgold=0)
    for day in days:
        t=pq.read_table(f"{ST}/m={M}/session={day}/part.parquet",columns=["ts","act","side","pt","sz","oid"],filters=[("act","in",["T"])]).to_pandas()
        off=int(pd.Timestamp(day+" 12:00",tz=ZoneInfo("America/Chicago")).utcoffset().total_seconds()); ns=t.ts.values+off*10**9-int(pd.Timestamp(day).value); sd=t.side.astype(str).values; m=sd!="N"
        ev=pd.DataFrame(dict(k="F",t=(ns[m]//10**6)+86400000,px=t.pt.values[m],sz=t.sz.values[m],a=np.where(sd[m]=="B",1,-1),id=t.oid.values[m])); ev.to_csv("ev.csv",header=False,index=False)
        l=lv[lv.day==day]; pd.DataFrame(dict(n=l.lvl.str.replace(",",";"),p=l.p,s=l.side,f=l.ta+86400)).to_csv("lv.csv",header=False,index=False)
        d=dd.loc[day]; out=subprocess.run(["./srrun","ev.csv","lv.csv",str(RTH[M][0]*60),str(RTH[M][1]*60),str(d.atr)],capture_output=True,text=True).stdout.splitlines()
        R=[x.split("|") for x in out if x.startswith("R|")]; cx={(r[4],int(r[7])):(int(r[14]),float(r[8]),int(r[2])-86400000) for r in R}
        e=ep[ep.day==day]; py={(n,int(v)):(int(g),float(x),int(tt)) for n,v,g,x,tt in zip(e.lvl,e.visit,e.stop_run,e.x,e.t)}
        both=set(cx)&set(py); tot["py"]+=len(py); tot["cx"]+=len(cx); tot["both"]+=len(both); tot["pygold"]+=sum(v[0] for v in py.values()); tot["cxgold"]+=sum(v[0] for v in cx.values()); tot["bothgold"]+=sum(1 for k in both if cx[k][0] and py[k][0])
        dx=[abs(cx[k][1]-py[k][1]) for k in both]; mis=[(k,py[k],cx[k]) for k in both if cx[k][0]!=py[k][0]]
        print(day,"crossings py",len(py),"engine",len(cx),"matched",len(both),"| gold py",sum(v[0] for v in py.values()),"engine",sum(v[0] for v in cx.values()),"| median |dx|",round(float(np.median(dx)),2) if dx else None,"| mismatches",mis[:3])
    print(M,tot)
if __name__=="__main__": run(sys.argv[1],sys.argv[2:])
