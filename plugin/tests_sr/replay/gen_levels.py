"""levels file for replay_sr: K <name> <px> (ONH/ONL PDH/PDL PFH/PFL LonHI/LonLO from the 3-min bars, KeyLevelsLogic.h windows),
M <ct_sec> <name> <px> (every MenthorQ INTRADAY snapshot for the session, MenthorQ's own prices; 1D Min / Max skipped)."""
import sys, pandas as pd
sys.path.insert(0,'/home/claude/sr/py'); from levels import keylevels, MQ
m,day,out=sys.argv[1],sys.argv[2],sys.argv[3]
K=keylevels(m,day)
q=pd.read_csv(MQ); q=q[(q.kind=='intraday')&(q.market==m)&(q.for_session==day)&~q.code.isin(['1DL','1DH'])]
q['ct']=((pd.to_datetime(q.asof_et)-pd.Timedelta('1h'))-pd.Timestamp('1970-01-01'))//pd.Timedelta('1s')
with open(out,'w') as f:
    for k,v in K.items(): f.write(f"K {k} {v}\n")
    for r in q.sort_values('ct').itertuples(): f.write(f"M {r.ct} {r.code} {r.price}\n")
print(out,len(K),q.ct.nunique())
