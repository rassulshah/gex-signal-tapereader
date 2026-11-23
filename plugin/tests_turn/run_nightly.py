import sys, json, os, hashlib, glob, shutil, warnings; warnings.filterwarnings('error')
os.environ['LRA_MBO_STORE']='/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store'
sys.path.insert(0,'/home/claude/turn/nightly'); import turn_pane as TN
repo,flex,today=sys.argv[1],sys.argv[2],sys.argv[3]
def md5(f):
    with open(f,'rb') as h: return hashlib.md5(h.read()).hexdigest()
S=TN.run(repo,flex=flex,today=today); TN.write_report(repo,S)
print('errors',S['errors'],'| odds',S['odds'],'| best proven',S['best_proven'],'|',{m:(v['sessions'],v['new_norm_days'],v['sweeps_added'],v['ranks']) for m,v in S['markets'].items()})
files=sorted(glob.glob(flex+'/LRA-Turn-*.csv'))+[repo+'/data/turn_pane/sweeps.csv',repo+'/data/turn_pane/norms_daily.csv']
h1={f:md5(f) for f in files}
for f in files: shutil.copy(f,f+'.run1')
S2=TN.run(repo,flex=flex,today=today); h2={f:md5(f) for f in files}
print('second run the same night: added',sum(v['sweeps_added'] for v in S2['markets'].values()),'| files identical',h1==h2,[f.split('/')[-1] for f in h1 if h1[f]!=h2[f]])
