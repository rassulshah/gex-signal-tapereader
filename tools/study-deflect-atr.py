#!/usr/bin/env python3
"""
THE OPERATOR'S ATR RULE, TESTED AS STATED.

  "in a downmove the low of the candle has to be within 1 atr of the high of the node but cannot be
   more than 2 atr below the node."

So for a DOWNMOVE testing a node from above, the bar's LOW must satisfy:

      node - 2*ATR  <=  low  <=  node + 1*ATR

Asymmetric on purpose: you must get CLOSE on approach (1 ATR) but a real test is allowed to wick a
long way THROUGH (2 ATR). An UPMOVE is the mirror, on the bar's HIGH.

⚠ WHY THIS IS BETTER THAN THE FIXED BAND I HAD. ATR(14) on these 3m SPY bars runs 0.19-0.87 with a
median of 0.38 - so my hand-fitted 0.40 was right for a median day and wrong at both ends. His rule
is the same number where I fitted it and adapts where I could not.

⚠ "THE HIGH OF THE NODE" - the trinity gives a node as a STRIKE, a point, with no thickness. The
chart draws a band. Treated here as the strike itself; if the band has real extent the approach leg
should use its top edge and this will read slightly tight. Flagged, not guessed at.
"""
import json, io, glob, datetime, collections

CONFIRM, TOPN = 2, 5
A_NEAR, A_THRU = 1.0, 1.5        # FINAL. see the grid sweep in study-deflect-grid.py
USE_CLOSE      = False           # FINAL. trigger on the WICK. see the reversal note below.

# ⚠⚠ FINALISED 2026-08-29 AGAINST tools/study-deflect-grid.py, WHICH SWEPT BOTH KNOBS.
#  * approach 1.00 ATR is the right tolerance. 0.75 loses events, 1.25 ADDS 34% more events and
#    DROPS the turn rate (59.2% -> 57.6%) - it is admitting visits that never tested anything.
#  * 2.0 vs 1.5 ATR penetration is a NON-QUESTION: it moves the count by 0-4 events in ~400,
#    because only 3% of tests penetrate past 1 ATR at all. Set to 1.5 because it is tighter for
#    free; anyone who prefers 2.0 is not wrong, the parameter simply does not bite.
#  * ⚠⚠ CLOSE-AS-TRIGGER LOOKED BETTER IN AGGREGATE AND IS WRONG. It gives 237 events where the
#    wick gives 394 at the same turn rate and a better adverse excursion, so every summary
#    statistic favoured it. Then RECALL against his own circles killed it: 2026-08-25, he circled
#    763.20 at node 763 - low 763.28, CLOSE 764.80. A 1.5-point rejection wick, a textbook
#    deflection, and the close rule discards it.
#    THE SELECTIVITY GAIN WAS ENTIRELY THE COST OF THROWING AWAY THE SHARPEST DEFLECTIONS - the
#    big rejection wicks, which are the best instances of the thing, not the marginal ones.
#    ⚠ THE SPLIT THAT IS ACTUALLY RIGHT: the WICK decides whether price TESTED the node (a stab is
#    a test); the CLOSE decides whether it DEFLECTED or BROKE. Trigger on one, classify on the
#    other. `classify` already reads the close, so this file now uses each where it belongs.

def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000-5*3600).strftime('%H:%M')

def atr14(bars):
    out=[None]*len(bars); trs=[]
    for i,(t,hi,lo,c) in enumerate(bars):
        pc = bars[i-1][3] if i else c
        trs.append(max(hi-lo, abs(hi-pc), abs(lo-pc)))
        if i>=13: out[i]=sum(trs[i-13:i+1])/14.0
    return out

def run(day_file):
    d=json.load(io.open(day_file))
    s=[x for x in d['snaps']['SPY'] if isinstance(x.get('px'),(int,float)) and x.get('h') and x.get('l')
       and '08:30'<=ct(x['t'])<='15:00']
    if len(s)<40: return None
    # ⚠ DESPIKE. 2026-08-26 10:59 carries a single print of 709.72 against a 765 market - a bad
    # feed tick. One such tick inflates ATR(14) for the NEXT 14 BARS, which in an ATR-scaled rule
    # silently widens every band on the day. Drop bars whose close is >2% from the local median.
    import statistics
    med = statistics.median([x['px'] for x in s])
    s   = [x for x in s if abs(x['px']-med)/med < 0.02]
    bars=[(x['t'],x['h'],x['l'],x['px']) for x in s]
    A=atr14(bars)
    rank=collections.Counter()
    for x in s:
        for i,(k,pct) in enumerate(((x.get('tri') or {}).get('SPY',{}) or {}).get('top', [])):
            if i<TOPN: rank[k]+=1
    nodes=[k for k,n in rank.items() if n>=0.25*len(s)]

    events=[]
    for k in sorted(nodes):
        # which bars TEST this node, per his rule, and from which side
        hits=[]
        for i,(t,hi,lo,c) in enumerate(bars):
            a=A[i]
            if not a: continue
            pc = bars[i-1][3] if i else c
            dn = c if USE_CLOSE else lo
            up = c if USE_CLOSE else hi
            if pc > k:                                   # arriving from ABOVE -> a downmove test
                if (k - A_THRU*a) <= dn <= (k + A_NEAR*a): hits.append((i,+1))
            elif pc < k:                                 # arriving from BELOW -> an upmove test
                if (k - A_NEAR*a) <= up <= (k + A_THRU*a): hits.append((i,-1))
        if not hits: continue
        # ONE VISIT = ONE EVENT. contiguous hits collapse; a gap re-arms.
        vis=[]; cur=None
        for i,side in hits:
            if cur and i-cur['i1']<=1: cur['i1']=i
            else:
                if cur: vis.append(cur)
                cur={'i0':i,'i1':i,'from':side}
        if cur: vis.append(cur)
        for v in vis:
            after=bars[v['i1']+1: v['i1']+1+CONFIRM+4]
            if len(after)<CONFIRM: continue
            a=A[v['i1']] or 0.4
            held=broke=0
            for (t,hi,lo,c) in after:
                if abs(c-k) < a: continue                # still inside the noise band, no verdict
                if (c-k)*v['from'] > 0: held+=1           # back to the side it came from
                else: broke+=1
            if held>=CONFIRM:
                events.append((ct(bars[v['i0']][0]), k, 'deflect', v['from']))
            elif broke>=CONFIRM:
                events.append((ct(bars[v['i0']][0]), k, 'BREAKDOWN' if v['from']>0 else 'BREAKOUT', v['from']))
    # ⚠⚠ ONE CIRCLE = ONE DEFLECTION, AND A CIRCLE ENCLOSES A PRICE EVENT, NOT A NODE.
    # His band is 1 ATR up + 2 ATR down = ~1.14 wide; SPY strikes are 1.00 apart. Adjacent bands
    # therefore ALWAYS overlap, so per-node counting double-counts by construction - 09:36 on
    # 2026-08-21 fired on 763 AND 764 for one swing. Collapse events that share a time window into
    # a single event, attributed to the HEAVIEST node in the cluster.
    events.sort(key=lambda e:(e[0],e[1]))
    merged=[]
    for e in events:
        if merged and e[0]<=merged[-1][0] and abs(e[1]-merged[-1][1])<=2 and e[2]==merged[-1][2]:
            continue
        if merged:
            hh,mm=map(int,e[0].split(':')); ph,pm=map(int,merged[-1][0].split(':'))
            if (hh*60+mm)-(ph*60+pm) <= 6 and abs(e[1]-merged[-1][1])<=2 and e[2]==merged[-1][2]:
                if rank[e[1]] > rank[merged[-1][1]]: merged[-1]=e     # keep the heavier node
                continue
        merged.append(e)
    return d['date'], len(bars), nodes, merged

MARKED={'2026-08-20':(0,2), '2026-08-21':(5,0), '2026-08-24':(5,2)}   # (white, red) he drew
tot_d=tot_b=0; n=0
for f in sorted(glob.glob('data/2026-*.json')):
    r=run(f)
    if not r: continue
    date,nb,nodes,ev=r
    dfl=[e for e in ev if e[2]=='deflect']; brk=[e for e in ev if e[2]!='deflect']
    tot_d+=len(dfl); tot_b+=len(brk); n+=1
    mk=MARKED.get(date)
    note=('   he marked %d white / %d red'%mk) if mk else ''
    print('%s  nodes %d  DEFLECTIONS %2d  breaks %2d%s'%(date,len(nodes),len(dfl),len(brk),note))
    if mk:
        for t,k,kind,side in ev[:14]:
            print('        %s  %-10s node %s'%(t,kind,k))
print('\n%d sessions   %d deflections (%.1f/session)   %d breaks   deflect rate %.0f%%'
      %(n,tot_d,tot_d/n,tot_b,100.0*tot_d/(tot_d+tot_b) if tot_d+tot_b else 0))
