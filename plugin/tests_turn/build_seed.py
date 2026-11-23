"""Build the nightly's seed from the reviewed reference: the 110 sweeps recomputed with lra.turn's OWN functions (must equal the golden
values), the half-hour normals of the research (norms_daily.csv), and check day_norms() against them."""
import sys, json, numpy as np, pandas as pd, warnings; warnings.filterwarnings('ignore')
sys.path.insert(0,'/home/claude/turn/nightly'); import turn_pane as TN
REPO='/mnt/user-data/uploads/level-reversal-analytics'; OUT='/home/claude/turn/work/repo/data/turn_pane/'
G=json.load(open('/mnt/user-data/outputs/turn-review/golden_turn_v1.json')); X=pd.read_csv('/mnt/user-data/outputs/turn-review/ref110_final.csv')
RN=pd.read_csv('/mnt/user-data/uploads/OneDrive--LRA-Data/research/effort-reward/norms_daily.csv',float_precision='round_trip')
N=RN.rename(columns={'day':'sess','n15_agg':'n15','avg_order':'avg'})[['m','sess','tod','n15','avg']].copy(); N['tod']=N.tod.astype(int)
N.to_csv(OUT+'norms_seed.csv',index=False,float_format='%.17g'); print('norms seed rows',len(N))
rows=[]; bad=0
cur=None
for g in sorted(G['sweeps'],key=lambda g:(g['m'],g['sess'],g['t1'])):
    if cur!=(g['m'],g['sess']): T=TN.load_trades(REPO,g['m'],g['sess']); cur=(g['m'],g['sess'])
    t1=pd.Timestamp(g['t1'],tz='America/Chicago').value; d=int(g['dir']); R=TN.readings(T,t1,d,int(g['L']))
    n15,avg=TN.norms_for(N,g['m'],g['sess']); tod=R['tod']
    hx=R['hard_lots']/n15[tod] if n15[tod]>0 else np.nan; bx=R['big_lots']/avg[tod] if avg[tod]>0 else np.nan
    H=int(hx>=TN.HARD_CUT[g['m']]) if np.isfinite(hx) else np.nan; B=int(bx>TN.BIG_CUT[g['m']]) if np.isfinite(bx) else np.nan
    x=X[X.id==g['id']].iloc[0]
    rows.append(dict(id=g['id'],m=g['m'],sess=g['sess'],stamp=TN.ct_to_rt(g['t1']),dir=d,lvt=int(g['L']),lv=str(x.lv),src='seed',stop_lots=R['stop_lots'],stop_x=R['stop_lots']/TN.STOP_TYP[g['m']],
        effort=np.nan if R['effort'] is None else R['effort'],late=R['late'],decay=R['decay'],after=R['after'],F=R['F'],H=H,B=B,ans_ok=int(np.isfinite(hx) and np.isfinite(bx)),hard_x=hx,big_x=bx,
        ext_into=R['ext_into'],ext_left=R['ext_left'],hit=int(x.hit),R=float(x.R),worked=int(bool(x.w)),scored='2026-11-22',_hard=R['hard_lots'],_big=R['big_lots'],_ext=R['ext']))
D=pd.DataFrame(rows); D['marks']=D.F+D.H.fillna(0).astype(int)+D.B.fillna(0).astype(int)
dem=[];ver=[];why=[]
for r in D.itertuples():
    pools={c:D[D.m==r.m][c].dropna().values for c in ('late','decay','after')}          # the reviewed in-sample tables
    dm=TN.dem_of(dict(late=r.late,decay=r.decay,after=r.after),pools); v,w=TN.verdict(dm,int(r.marks),int(r.ext_into),int(r.ext_left),bool(r.ans_ok)); dem.append(dm); ver.append(TN.VERDICTS[v]); why.append(TN.WHY[w])
D['dem']=dem; D['verdict']=ver; D['why']=why; D['odds']=[{'STAND ASIDE':26,'WAIT':52}.get(v,-1) for v in ver]; D['dem2']=np.nan; D['v2']=''
# ---- must equal the golden values
def close(a,b): return (b is None and not np.isfinite(a)) or (b is not None and abs(a-b)<=1e-9*max(1,abs(b)))
for g in G['sweeps']:
    r=D[D.id==g['id']].iloc[0]
    ok=[r.stop_lots==g['stop_lots_sec'],close(r.effort,g['effort_sec']),close(r.late,g['late_rate_sec']),close(r.decay,g['decay_sec']),close(r.after,g['after_buy_sec']),close(r.dem,g['dem']),
        int(r.F)==int(g['fast_sec']),r._hard==int(round(g['hard_lots_sec'])),r._big==int(round(g['big_lots_sec'])),close(r.hard_x,g['hard_sec']),close(r.big_x,g['big_sec']),int(r.marks)==g['marks'],
        bool(r.ans_ok)==bool(g['ans_ok']),r._ext==g['ext_sec'],r.ext_into==int(g['ext_into_s_sec']),r.ext_left==int(g['ext_left_s_sec']),r.verdict==g['verdict']]
    if not all(ok): bad+=1; print('MISMATCH',g['id'],g['m'],g['t1'],[i for i,o in enumerate(ok) if not o])
print('lra.turn readings vs golden: sweeps',len(D),'mismatches',bad,'| verdicts',D.verdict.value_counts().to_dict())
TN.write_csv(D[TN.LEDGER_COLS],OUT+'seed_sweeps.csv')
back=TN.read_ledger(OUT+'seed_sweeps.csv'); print('seed round trip exact:',bool(np.array_equal(back.late.values,D.late.values) and np.array_equal(back.dem.values,D.dem.values) and (back.verdict.values==D.verdict.values).all()),'| stamps', int(back.stamp.min()), int(back.stamp.max()))
# ---- day_norms() reproduces the research normals
for m,s in (('ES','2026-11-19'),('GC','2026-11-20'),('NG','2026-11-18')):
    T=TN.load_trades(REPO,m,s); d=TN.day_norms(T).set_index('tod'); r=N[(N.m==m)&(N.sess==s)].set_index('tod')
    j=d.join(r,rsuffix='_r',how='outer'); print(m,s,'half hours',len(j),'| n15 max diff',float((j.n15-j.n15_r).abs().max()),'| avg max diff',float((j.avg-j.avg_r).abs().max()),'| only one side',int(j.n15.isna().sum()+j.n15_r.isna().sum()))
