#!/usr/bin/env python3
"""
Does a runner-up node's SIZE, or its GROWTH, predict that it takes the crown?

Operator question, 2026-08-29: "i am trying to figure out which price level will become the new
king before it happens ... if we see a king and then another node growing and the delta increasing,
we can say that it may become the new king ... is it even something that should be considered or is
the node growth enough?"

The panel already ships a succession model (successionFromPct) that uses SIZE ALONE — the largest
|%King| that is not the King. Its claimed backtest, in the source since v10.40:
    succession(cont>=60) -> King rolled to that strike w/in 20 bars 112/148 (76%)
This script re-derives that on the 9 recorded day files and asks whether GROWTH adds anything.

⚠ EVERY NUMBER HERE IS REAL-TIME BY CONSTRUCTION. The predictor is read at snap i and the outcome
is read strictly after it. No hindsight side-selection — F-12 is what happens otherwise.
"""
import json, io, glob, sys
from collections import defaultdict

HORIZON_MIN = 30          # "within the next 30 minutes"

def load():
    out=[]
    for f in sorted(glob.glob('data/2026-*.json')):
        d=json.load(io.open(f))
        s=(d.get('snaps') or {}).get('SPY') or []
        s=[x for x in s if isinstance(x.get('king'),(int,float))]
        if len(s)>20: out.append((d.get('date'), s))
    return out

days=load()
print('days: %d   snaps: %d' % (len(days), sum(len(s) for _,s in days)))

# ---- 1 · HOW OFTEN DOES THE CROWN ACTUALLY CHANGE? -------------------------------------------
tot_ch=0; per=[]
for date,s in days:
    ch=0; prev=s[0]['king']
    for x in s[1:]:
        if x['king']!=prev: ch+=1; prev=x['king']
    per.append((date,ch,len(s))); tot_ch+=ch
print('\n--- base rate: crown changes per session ---')
for d,c,n in per: print('  %s  changes=%-3d snaps=%d' % (d,c,n))
print('  TOTAL %d changes over %d sessions  = %.1f per session' % (tot_ch,len(days),tot_ch/len(days)))

# durable changes only: the new king must still hold DWELL snaps later
DWELL=2
dur=0
for date,s in days:
    prev=s[0]['king']
    for i in range(1,len(s)):
        if s[i]['king']!=prev:
            if all(s[j]['king']==s[i]['king'] for j in range(i,min(i+DWELL,len(s)))): dur+=1
            prev=s[i]['king']
print('  DURABLE (held >=%d snaps): %d  = %.2f per session' % (DWELL,dur,dur/len(days)))

# ---- 2 · THE PREDICTOR ------------------------------------------------------------------------
# At each snap: the runner-up by size (deriv.ranks[1]), its size m, and its GROWTH over the last
# k snaps. Outcome: does the crown sit on that strike at any point in the next HORIZON minutes?
def ranks(x):
    r=(x.get('deriv') or {}).get('ranks') or []
    return [(q.get('k'), q.get('m')) for q in r if q.get('k') is not None]

def study(minSize, needGrowth, growWin=3):
    tp=fp=0; fired=0
    for date,s in days:
        for i,x in enumerate(s):
            R=ranks(x)
            if len(R)<2: continue
            king=x['king']
            cand=[(k,m) for k,m in R if k!=king]
            if not cand: continue
            k2,m2=cand[0]
            if m2 is None or m2<minSize: continue
            if needGrowth:
                j=i-growWin
                if j<0: continue
                Rp=dict(ranks(s[j]))
                if k2 not in Rp or Rp[k2] is None: continue
                if not (m2 > Rp[k2]): continue          # must be GROWING
            fired+=1
            t0=x['t']
            hit=any(y['king']==k2 for y in s[i+1:] if y['t']-t0 <= HORIZON_MIN*60000)
            if hit: tp+=1
            else:   fp+=1
    n=tp+fp
    return fired, n, (100.0*tp/n if n else 0.0)

print('\n--- does the runner-up take the crown within %dm? ---' % HORIZON_MIN)
print('  %-34s %7s %8s' % ('predictor','n','hit%'))
for ms in (40,50,60,70,80):
    f,n,p = study(ms, False)
    print('  size >= %-3d%%%-22s %7d %7.1f%%' % (ms,'',n,p))
print()
for ms in (40,50,60,70,80):
    f,n,p = study(ms, True)
    print('  size >= %-3d%% AND growing%-11s %7d %7.1f%%' % (ms,'',n,p))

# ---- 3 · WHAT DOES CHANCE LOOK LIKE? ----------------------------------------------------------
# the same question asked of a node picked WITHOUT looking at size: the base rate a specific
# non-king strike takes the crown in the window.
base_hit=0; base_n=0
for date,s in days:
    for i,x in enumerate(s):
        R=ranks(x); king=x['king']
        cand=[k for k,m in R if k!=king]
        if not cand: continue
        t0=x['t']
        for k2 in cand:
            base_n+=1
            if any(y['king']==k2 for y in s[i+1:] if y['t']-t0<=HORIZON_MIN*60000): base_hit+=1
print('\n--- chance: ANY non-king ranked strike, same window ---')
print('  n=%d  hit=%.1f%%' % (base_n, 100.0*base_hit/base_n if base_n else 0))

# ---- 4 · THE DECISIVE COMPARISON: growing vs NOT growing, at the SAME size ---------------------
# ⚠ The table above filters on size in both arms, so size is controlled AT the threshold but not
# WITHIN it. If growing nodes are simply bigger nodes, the lift is size wearing a growth badge.
# This splits one population by growth alone.
def split(minSize, growWin=3, horizon=HORIZON_MIN):
    g=[0,0]; ng=[0,0]                      # [hit, n]
    for date,s in days:
        for i,x in enumerate(s):
            R=ranks(x); king=x['king']
            cand=[(k,m) for k,m in R if k!=king]
            if not cand: continue
            k2,m2=cand[0]
            if m2 is None or m2<minSize: continue
            j=i-growWin
            if j<0: continue
            Rp=dict(ranks(s[j]))
            if k2 not in Rp or Rp[k2] is None: continue
            t0=x['t']
            hit=any(y['king']==k2 for y in s[i+1:] if y['t']-t0<=horizon*60000)
            b = g if m2>Rp[k2] else ng
            b[1]+=1; b[0]+= 1 if hit else 0
    return g,ng

print('\n--- SAME size band, split by growth alone ---')
print('  %-16s %10s %10s %8s' % ('size band','GROWING','FLAT/DRAIN','lift'))
for ms in (40,50,60,70,80):
    g,ng=split(ms)
    pg=100.0*g[0]/g[1] if g[1] else 0
    pn=100.0*ng[0]/ng[1] if ng[1] else 0
    print('  size >= %-3d%%      %5.1f%% n=%-4d %5.1f%% n=%-4d %+7.1fpp' % (ms,pg,g[1],pn,ng[1],pg-pn))

# ---- 5 · HORIZON SENSITIVITY, and the shipped 76% claim ---------------------------------------
# The source has carried this since v10.40:
#   succession(cont>=60) -> King rolled to that strike w/in 20 bars 112/148 (76%)
# 20 bars is not defined in minutes there. Sweep the horizon and see if 76% appears anywhere.
print('\n--- horizon sweep, size >= 60%% (chasing the shipped 76%% claim) ---')
for hz in (10,20,30,60,120,240,390):
    tot=[0,0]
    for date,s in days:
        for i,x in enumerate(s):
            R=ranks(x); king=x['king']
            cand=[(k,m) for k,m in R if k!=king]
            if not cand: continue
            k2,m2=cand[0]
            if m2 is None or m2<60: continue
            t0=x['t']
            hit=any(y['king']==k2 for y in s[i+1:] if y['t']-t0<=hz*60000)
            tot[1]+=1; tot[0]+= 1 if hit else 0
    print('    within %4dm   n=%-5d %5.1f%%' % (hz,tot[1],100.0*tot[0]/tot[1] if tot[1] else 0))
