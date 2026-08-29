#!/usr/bin/env python3
"""
VARIANT SWEEP FOR THE DEFLECTION RULE — scored WITHOUT his labels.

His marks are examples, not an exhaustive set, so precision cannot be scored against them. What CAN
be scored, label-free, is the FORWARD BEHAVIOUR: when the rule fires, does price actually turn away
from the node, and by how much? A rule that fires on noise shows it here as a collapsing win rate.

⚠ NON-CIRCULAR BY CONSTRUCTION. The forward stats are computed over EVERY trigger, not over the
subset later classified 'deflect'. Classifying by forward price and then scoring forward price
would manufacture any number wanted.

Fade convention: a test from ABOVE (downmove) is faded LONG - MFE measured upward from the trigger
close, MAE downward. Mirrored for a test from below.
"""
import json, io, glob, datetime, collections, statistics

TOPN, FWD = 5, 10          # 10 bars = 30 minutes
def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000-5*3600).strftime('%H:%M')
def atr14(bars):
    out=[None]*len(bars); trs=[]
    for i,(t,hi,lo,c) in enumerate(bars):
        pc=bars[i-1][3] if i else c
        trs.append(max(hi-lo,abs(hi-pc),abs(lo-pc)))
        if i>=13: out[i]=sum(trs[i-13:i+1])/14.0
    return out

def load(f):
    d=json.load(io.open(f))
    s=[x for x in d['snaps']['SPY'] if isinstance(x.get('px'),(int,float)) and x.get('h') and x.get('l')
       and '08:30'<=ct(x['t'])<='15:00']
    if len(s)<40: return None
    med=statistics.median([x['px'] for x in s])
    s=[x for x in s if abs(x['px']-med)/med<0.02]                 # despike
    bars=[(x['t'],x['h'],x['l'],x['px']) for x in s]
    rank=collections.Counter()
    for x in s:
        for i,(k,p) in enumerate(((x.get('tri') or {}).get('SPY',{}) or {}).get('top',[])):
            if i<TOPN: rank[k]+=1
    return bars, atr14(bars), [k for k,n in rank.items() if n>=.25*len(s)], rank

def sweep(near, thru, use_close):
    """near = approach tolerance (ATR), thru = max penetration (ATR), use_close = trigger on close."""
    trig=[]
    for f in sorted(glob.glob('data/2026-*.json')):
        L=load(f)
        if not L: continue
        bars,A,nodes,rank=L
        raw=[]
        for k in nodes:
            for i,(t,hi,lo,c) in enumerate(bars):
                a=A[i]
                if not a or i+FWD>=len(bars): continue
                pc=bars[i-1][3] if i else c
                probe_dn = c if use_close else lo
                probe_up = c if use_close else hi
                if pc>k and (k-thru*a) <= probe_dn <= (k+near*a):  raw.append((i,k,+1,a,c))
                elif pc<k and (k-near*a) <= probe_up <= (k+thru*a): raw.append((i,k,-1,a,c))
        # collapse to PRICE EVENTS (his "one circle = one deflection")
        raw.sort()
        ev=[]
        for i,k,side,a,c in raw:
            if ev and i-ev[-1][0]<=2 and abs(k-ev[-1][1])<=2 and side==ev[-1][2]:
                if rank[k]>rank[ev[-1][1]]: ev[-1]=(ev[-1][0],k,side,a,ev[-1][4])
                continue
            ev.append((i,k,side,a,c))
        for i,k,side,a,c in ev:
            fwd=bars[i+1:i+1+FWD]
            mfe=max((b[1]-c) if side>0 else (c-b[2]) for b in fwd)   # favourable to the fade
            mae=max((c-b[2]) if side>0 else (b[1]-c) for b in fwd)   # adverse
            trig.append((mfe,mae,a))
    if not trig: return None
    n=len(trig)
    win=sum(1 for m,_,a in trig if m>=a)                      # turned away by a full ATR
    mfe=statistics.median([m for m,_,_ in trig])
    mae=statistics.median([x for _,x,_ in trig])
    exp=statistics.mean([m-x for m,x,_ in trig])
    return n, 100.*win/n, mfe, mae, exp

print('  trigger   approach  thru      n   turned>=1ATR   medMFE  medMAE   expectancy')
print('  '+'-'*74)
for use_close in (False,True):
    for near in (0.75,1.0,1.25):
        for thru in (1.0,1.5,2.0):
            r=sweep(near,thru,use_close)
            if not r: continue
            n,w,mfe,mae,exp=r
            star=' <<<' if abs(near-1.0)<1e-9 and abs(thru-2.0)<1e-9 else ''
            print('  %-8s  %4.2f ATR  %3.1f  %5d   %5.1f%%        %+.2f   %+.2f    %+.3f%s'
                  %('close' if use_close else 'low/high',near,thru,n,w,mfe,mae,exp,star))
    print()
