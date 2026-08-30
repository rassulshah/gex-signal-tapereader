#!/usr/bin/env python3
"""
GREEN DAY / RED DAY — does the session close above or below its own RTH open?

⚠⚠ THE DISCIPLINE, STATED BEFORE ANY NUMBER IS PRODUCED, because with 284 sessions and a free
choice of features a 65% "accuracy" can be manufactured in twenty minutes and will mean nothing:
  · the BASE RATE is quoted first and every result is judged against IT, not against 50%.
  · features are PRE-SPECIFIED here, in this list, before being measured.
  · the split is TIME-ORDERED — train on the earlier sessions, test on the later ones. Random CV on
    a time series leaks the future through the fold boundary and always flatters.
  · nothing is reported as a model until it beats the base rate OUT OF SAMPLE.
"""
import pickle, datetime, statistics, math, collections
S=pickle.load(open('/tmp/sess.pkl','rb')); ds=sorted(S)

def feats(i):
    d=ds[i]; R=S[d]['rth']; ON=S[d]['on']
    o=R[0][1]; c=R[-1][4]
    prev = S[ds[i-1]]['rth'][-1][4] if i>0 else None
    def upto(mins):
        cut=(8*60+30)+mins
        return [b for b in R if b[0].hour*60+b[0].minute <= cut]
    ib30, ib60 = upto(30), upto(60)
    f={}
    f['dow']      = d.weekday()
    f['gap']      = (o-prev)/prev*10000 if prev else 0.0          # overnight gap, bp
    f['on_rng']   = (max(b[2] for b in ON)-min(b[3] for b in ON))/o*10000 if ON else 0.0
    f['r30']      = (ib30[-1][4]-o)/o*10000                        # first 30m return, bp
    f['r60']      = (ib60[-1][4]-o)/o*10000
    f['ib30rng']  = (max(b[2] for b in ib30)-min(b[3] for b in ib30))/o*10000
    f['ib60rng']  = (max(b[2] for b in ib60)-min(b[3] for b in ib60))/o*10000
    # IB30 break: after the first 30m, which side of the IB30 range broke FIRST?
    hi=max(b[2] for b in ib30); lo=min(b[3] for b in ib30); brk=0
    for b in R:
        if b[0].hour*60+b[0].minute <= (8*60+30)+30: continue
        if b[2]>hi: brk=1; break
        if b[3]<lo: brk=-1; break
    f['ib30brk']=brk
    # where the open sits inside the overnight range (0=at ON low, 1=at ON high)
    if ON:
        h=max(b[2] for b in ON); l=min(b[3] for b in ON)
        f['on_pos']=(o-l)/(h-l) if h>l else .5
    else: f['on_pos']=.5
    return f, (1 if c>o else 0), (c-o)/o*10000

X=[];y=[];ret=[]
for i in range(1,len(ds)):
    f,g,r=feats(i); X.append(f); y.append(g); ret.append(r)
n=len(y); base=sum(y)/n
print('sessions %d   %s -> %s'%(n,ds[1],ds[-1]))
print('BASE RATE: green %.1f%%  (red %.1f%%)  -> always-guess-green scores %.1f%%'%(100*base,100*(1-base),100*max(base,1-base)))
print()

def auc(v,lab):
    p=sorted(zip(v,lab)); pos=sum(lab); neg=len(lab)-pos
    if not pos or not neg: return .5
    r=0; i=0; rank=0
    order=sorted(range(len(v)), key=lambda k:v[k])
    for rk,k in enumerate(order,1):
        if lab[k]: r+=rk
    return (r - pos*(pos+1)/2)/(pos*neg)

print('%-10s %7s %7s   %s'%('feature','AUC','|lift|','reading'))
print('-'*62)
for k in ['dow','gap','on_rng','r30','r60','ib30rng','ib60rng','ib30brk','on_pos']:
    v=[f[k] for f in X]; a=auc(v,y)
    print('%-10s %7.3f %7.3f   %s'%(k,a,abs(a-.5)*2,
      'nothing' if abs(a-.5)<.03 else ('WEAK' if abs(a-.5)<.07 else 'worth testing')))

# ---------------------------------------------------------------------------------------------
# OUT-OF-SAMPLE. Train on the earlier 2/3, test on the later 1/3. No random folds: this is a time
# series and a random split leaks the future across the fold boundary.
print()
def fit(rows, lab, keys, iters=3000, lr=.06):
    m=len(keys); w=[0.0]*m; b=0.0
    mu=[statistics.mean([r[k] for r in rows]) for k in keys]
    sd=[statistics.pstdev([r[k] for r in rows]) or 1 for k in keys]
    Z=[[(r[k]-mu[j])/sd[j] for j,k in enumerate(keys)] for r in rows]
    for _ in range(iters):
        gw=[0.0]*m; gb=0.0
        for z,t in zip(Z,lab):
            p=1/(1+math.exp(-max(-30,min(30,sum(wi*zi for wi,zi in zip(w,z))+b))))
            e=p-t
            for j in range(m): gw[j]+=e*z[j]
            gb+=e
        for j in range(m): w[j]-=lr*gw[j]/len(Z)
        b-=lr*gb/len(Z)
    def pred(r):
        z=[(r[k]-mu[j])/sd[j] for j,k in enumerate(keys)]
        return 1/(1+math.exp(-max(-30,min(30,sum(wi*zi for wi,zi in zip(w,z))+b))))
    return pred

cut=int(n*2/3)
for name,keys in [('r30 only',['r30']), ('r60 only',['r60']),
                  ('r30 + ib30brk',['r30','ib30brk']),
                  ('r60 + ib30brk',['r60','ib30brk']),
                  ('r60 + ib30brk + gap + dow',['r60','ib30brk','gap','dow'])]:
    p=fit(X[:cut],y[:cut],keys)
    ps=[p(r) for r in X[cut:]]; ys=y[cut:]
    acc=sum(1 for q,t in zip(ps,ys) if (q>=.5)==(t==1))/len(ys)
    bl=max(sum(ys)/len(ys), 1-sum(ys)/len(ys))
    print('%-28s OOS acc %.1f%%   (test base %.1f%%)   lift %+.1f pp   AUC %.3f'
          %(name,100*acc,100*bl,100*(acc-bl),auc(ps,ys)))
print('   n_train %d   n_test %d   test window %s -> %s'%(cut,n-cut,ds[cut+1],ds[-1]))

# ---------------------------------------------------------------------------------------------
# WALK-FORWARD + CALIBRATION. One 95-day window is thin; refit every 10 sessions on everything
# before it, so every session after the burn-in is scored by a model that never saw it.
print()
KEYS=['r60','ib30brk']
P=[];Y=[]
start=150
for i in range(start,n,10):
    p=fit(X[:i],y[:i],KEYS)
    for j in range(i,min(i+10,n)): P.append(p(X[j])); Y.append(y[j])
acc=sum(1 for q,t in zip(P,Y) if (q>=.5)==(t==1))/len(P)
bl=max(sum(Y)/len(Y),1-sum(Y)/len(Y))
se=math.sqrt(acc*(1-acc)/len(P))
print('WALK-FORWARD  n=%d   acc %.1f%% ±%.1f   base %.1f%%   lift %+.1f pp   AUC %.3f'
      %(len(P),100*acc,100*se,100*bl,100*(acc-bl),auc(P,Y)))

print('\nCALIBRATION — does a "70%" call actually come in at 70%?')
print('  %-16s %5s %8s %9s'%('model says','n','green','claimed'))
for lo,hi in [(0,.35),(.35,.45),(.45,.55),(.55,.65),(.65,1.01)]:
    b=[(q,t) for q,t in zip(P,Y) if lo<=q<hi]
    if len(b)<8: continue
    print('  %-16s %5d %7.0f%% %8.0f%%'%('%.0f-%.0f%%'%(100*lo,100*hi),len(b),
          100*sum(t for _,t in b)/len(b), 100*statistics.mean([q for q,_ in b])))

print('\nWHEN IT COMMITS (the number that matters for trading it):')
for th in [.60,.65,.70]:
    conf=[(q,t) for q,t in zip(P,Y) if abs(q-.5)>=th-.5]
    if not conf: continue
    hit=sum(1 for q,t in conf if (q>=.5)==(t==1))/len(conf)
    print('  |p-50%%| >= %2.0f pp : fires on %3d of %d days (%.0f%%)  and is right %.0f%%'
          %(100*(th-.5),len(conf),len(P),100*len(conf)/len(P),100*hit))

# ---------------------------------------------------------------------------------------------
# ⚠ DOES THE MODEL BEAT THE ONE-LINE RULE? If "green if price is up at 09:30" matches the logistic,
# the logistic is decoration and the RULE ships — it is inspectable and has no parameters to rot.
print('\nNAIVE RULES, same walk-forward window:')
Wy=Y; idx=list(range(start,n))[:len(Y)]
for nm,fn in [('up at 09:00 (r30>0)',      lambda r: r['r30']>0),
              ('up at 09:30 (r60>0)',      lambda r: r['r60']>0),
              ('IB30 broke UP first',      lambda r: r['ib30brk']>0),
              ('r60>0 AND IB30 broke up',  lambda r: r['r60']>0 and r['ib30brk']>0)]:
    hits=[]; fires=0
    for j,t in zip(idx,Wy):
        c=fn(X[j])
        if nm.startswith('r60>0 AND'):
            allow = (X[j]['r60']>0) == (X[j]['ib30brk']>0)   # only call when they agree
            if not allow: continue
        fires+=1; hits.append(1 if (1 if c else 0)==t else 0)
    if hits:
        a=sum(hits)/len(hits)
        print('  %-26s fires %3d/%d  right %.0f%%  (model 74%%, base 53%%)'%(nm,fires,len(Wy),100*a))

# ---------------------------------------------------------------------------------------------
# ⚠⚠ WHEN IS THE CALL ACTUALLY AVAILABLE? "IB30 broke up first" is only useful if the break
# happens early. A break at 14:00 is not a forecast, it is a description.
print('\nWHEN DOES THE IB30 BREAK HAPPEN?')
times=[]
for i in range(1,len(ds)):
    d=ds[i]; R=S[d]['rth']; o=R[0][1]
    ib=[b for b in R if b[0].hour*60+b[0].minute<=(8*60+30)+30]
    hi=max(b[2] for b in ib); lo=min(b[3] for b in ib); got=None
    for b in R:
        m=b[0].hour*60+b[0].minute
        if m<=(8*60+30)+30: continue
        if b[2]>hi or b[3]<lo: got=m; break
    if got: times.append(got)
times.sort()
def cl(m): return '%02d:%02d'%(m//60,m%60)
print('  n=%d  median %s   p25 %s   p75 %s   p90 %s'
      %(len(times),cl(times[len(times)//2]),cl(times[len(times)//4]),
        cl(times[3*len(times)//4]),cl(times[int(.9*len(times))])))
for cut in [(9*60+15),(9*60+30),(10*60),(11*60)]:
    print('    broken by %s : %.0f%% of sessions'%(cl(cut),100*sum(1 for t in times if t<=cut)/len(times)))
