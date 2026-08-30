#!/usr/bin/env python3
"""
DOES GAMMA ARRIVING AT A NODE MAKE IT HOLD?

Operator, 2026-08-30: "many times new pullback nodes show up to support a move or to stop a move ...
i want to use delta to show an increase in this gamma and the arrows showing if there was sometype
of roll where gamma moved from other nodes in to a node to create new support or resistance."

⚠⚠ THIS IS gx-004, PRE-REGISTERED 2026-08-29 BEFORE ANY OF THIS DATA WAS LOOKED AT. The registered
claim, verbatim: "the deflect/break split is decided by whether the node is GAINING dollars in the
minutes before price arrives, not by the node's size." Registered n_needed: 150 sessions.

⚠⚠ THERE ARE NINE. Everything below is UNDERPOWERED BY A FACTOR OF SIXTEEN and is reported as a
direction to watch, never as a rate to trade. The pre-registration is what makes it worth running
early: the hypothesis cannot be reshaped to fit what comes out.
"""
import json, io, glob, datetime, statistics, collections, math

TOPN, NEAR = 5, 1.0          # node universe, and the ATR band from the deflection geometry
GROW_BARS = 5                # how far back to measure the node's mass change

def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000-5*3600).strftime('%H:%M')
def atr14(bars):
    out=[None]*len(bars); trs=[]
    for i,(t,hi,lo,c) in enumerate(bars):
        pc=bars[i-1][3] if i else c
        trs.append(max(hi-lo,abs(hi-pc),abs(lo-pc)))
        if i>=13: out[i]=sum(trs[i-13:i+1])/14.0
    return out

rows=[]
for f in sorted(glob.glob('data/2026-*.json')):
    d=json.load(io.open(f))
    s=[x for x in d['snaps']['SPY'] if isinstance(x.get('px'),(int,float)) and x.get('h') and x.get('l')
       and x.get('nodes') and '08:30'<=ct(x['t'])<='15:00']
    if len(s)<40: continue
    med=statistics.median([x['px'] for x in s]); s=[x for x in s if abs(x['px']-med)/med<0.02]
    bars=[(x['t'],x['h'],x['l'],x['px']) for x in s]; A=atr14(bars)
    # per-bar mass for every strike, so a node's CHANGE is readable
    # ⚠⚠ ABSOLUTE DOLLARS, NOT %King. The first run of this study used tri[].pct — which is the
    # node's mass RELATIVE TO THE KING. That share falls whenever the KING grows, even if the node's
    # own gamma is flat, so it answers a different question than "is gamma arriving here". The
    # recorder carries `nodes[].abs` in dollars; that is the quantity his claim is about.
    mass=[]
    for x in s:
        m={}
        for nd in (x.get('nodes') or []):
            k=nd.get('k'); a=nd.get('abs')
            if isinstance(k,(int,float)) and isinstance(a,(int,float)): m[k]=a
        mass.append(m)
    rank=collections.Counter()
    for m in mass:
        for i,k in enumerate(sorted(m, key=lambda z:-m[z])):
            if i<TOPN: rank[k]+=1
    nodes=[k for k,n in rank.items() if n>=0.25*len(s)]
    scale=[]
    for m in mass:
        v=sorted(m.values(), reverse=True)
        scale.append(v[0] if v else 0.0)     # the day's biggest node, to express growth in %
    for k in nodes:
        for i in range(GROW_BARS+1, len(bars)-6):
            a=A[i]
            if not a: continue
            t,hi,lo,c=bars[i]; pc=bars[i-1][3]
            # the deflection geometry, v15.01: 1 ATR short / 1.5 ATR through, on the WICK
            if pc>k:  hit=(k-1.5*a) <= lo <= (k+NEAR*a); side=+1
            elif pc<k: hit=(k-NEAR*a) <= hi <= (k+1.5*a); side=-1
            else: continue
            if not hit: continue
            now=mass[i].get(k, 0.0); then=mass[i-GROW_BARS].get(k, 0.0)
            ref=max(1.0, scale[i])
            growPct=100.0*(now-then)/ref     # growth as a share of the biggest node — comparable across days
            fwd=bars[i+1:i+7]; end=fwd[-1][3]
            deflect = ((end-k)*side > 0)
            rows.append(dict(day=d['date'], k=k, at=ct(t), grow=growPct, absNow=now,
                             absChg=now-then, deflect=deflect))

n=len(rows)
if not n: print('no touches'); raise SystemExit
base=sum(1 for r in rows if r['deflect'])/n
print('touches %d over %d sessions   deflect rate %.0f%%\n'%(n, len(set(r['day'] for r in rows)), 100*base))
print('%-26s %6s %9s %s'%('the node into the touch','n','deflect','vs base'))
print('-'*62)
for nm,f in [('GAINING $ (>+5% of top)',  lambda r: r['grow']>5),
             ('flat (-5..+5%)',            lambda r: -5<=r['grow']<=5),
             ('SHEDDING $ (<-5% of top)',  lambda r: r['grow']<-5)]:
    g=[r for r in rows if f(r)]
    if len(g)<8: print('%-26s %6d   (too few to read)'%(nm,len(g))); continue
    d1=sum(1 for r in g if r['deflect'])/len(g)
    se=math.sqrt(d1*(1-d1)/len(g))
    print('%-26s %6d %8.0f%% %+7.0f pp   %s'%(nm,len(g),100*d1,100*(d1-base),
          'inside noise' if abs(d1-base)<2*se else 'OUTSIDE noise'))
print('\n⚠ n_needed for gx-004 is 150 sessions. There are %d. This is a DIRECTION, not a rate.'
      %len(set(r['day'] for r in rows)))
