#!/usr/bin/env python3
"""
THE UNIT IS A PULLBACK, NOT A BAR-TOUCH.

Operator, 2026-08-31: "each day will only have a few pullback opportunties and each pullback has 1
deflection so when you have that many deflections in a day, its because the data was not classified
correctly."

⚠⚠ HE IS RIGHT AND I HAD REGRESSED. tools/study-deflect-atr.py already collapsed touches into PRICE
EVENTS after he taught me "in each circle you only count it as 1 deflection" — ~10 per session.
tools/study-rollsupport.py then went back to counting EVERY BAR against EVERY NODE: 481 over four
sessions, ~120 a day. A count that large is not a measurement of pullbacks, it is a measurement of
how many bars sit near a node.

THE UNIT, DEFINED:
  A PULLBACK is a counter-move of at least PB_MIN ATR away from a local extreme, which then REVERSES
  back at least PB_CONFIRM ATR. Its turning point is ONE event. The node question is asked once, at
  that turning point — not at every bar that happens to be nearby.

⚠ This is a SWING definition, so it is symmetric and blind to the node: pullbacks are found from
price alone, and only THEN is a node looked for. Finding swings by looking for nodes would
manufacture the correlation the study is meant to test.
"""
import json, io, glob, datetime, statistics, collections, math

# ⚠⚠ A SWING POINT IS THE EXTREME OF ITS OWN NEIGHBOURHOOD. The first attempt asked only "did price
# come down and go back up", which every third bar satisfies — 43 a session, still nothing like the
# "few pullback opportunities" he described. A pullback the eye picks out is the LOWEST low for half
# an hour either side, and a real excursion. Both conditions, or the count is meaningless.
PB_WIN = 10                       # bars each side it must be the extreme of (10 x 3m = 30 min)
PB_MIN, PB_CONFIRM = 3.0, 2.0     # ATR: how far it ran out, and how far it came back
NODE_NEAR, NODE_THRU = 1.0, 1.5   # the deflection band settled at v15.01

def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000-5*3600).strftime('%H:%M')
def atr14(bars):
    out=[None]*len(bars); trs=[]
    for i,(t,hi,lo,c) in enumerate(bars):
        pc=bars[i-1][3] if i else c
        trs.append(max(hi-lo,abs(hi-pc),abs(lo-pc)))
        if i>=13: out[i]=sum(trs[i-13:i+1])/14.0
    return out

def swings(bars, A):
    """A pullback turn: the extreme of its own +-PB_WIN neighbourhood, reached by a real excursion
    and left by a real reversal. One entry per turn."""
    out=[]; n=len(bars)
    for i in range(PB_WIN+4, n-PB_WIN-1):
        a=A[i]
        if not a: continue
        lo=bars[i][2]; hi=bars[i][1]
        win=bars[i-PB_WIN:i+PB_WIN+1]
        # ⚠ THE NEIGHBOURHOOD TEST is what makes this "a pullback you would circle" rather than
        # "a bar lower than the one before it".
        if lo<=min(b[2] for b in win):
            ran_out = max(b[1] for b in bars[i-PB_WIN:i]) - lo
            came_bk = max(b[1] for b in bars[i+1:i+PB_WIN+1]) - lo
            if ran_out>=PB_MIN*a and came_bk>=PB_CONFIRM*a: out.append((i,'LOW',lo))
        if hi>=max(b[1] for b in win):
            ran_out = hi - min(b[2] for b in bars[i-PB_WIN:i])
            came_bk = hi - min(b[2] for b in bars[i+1:i+PB_WIN+1])
            if ran_out>=PB_MIN*a and came_bk>=PB_CONFIRM*a: out.append((i,'HIGH',hi))
    out.sort(); keep=[]
    for e in out:
        # ⚠ ONE EVENT PER TURN, regardless of side: a bar cannot be both ends of one pullback.
        if keep and e[0]-keep[-1][0] <= PB_WIN: 
            if e[1]==keep[-1][1] and ((e[1]=='LOW' and e[2]<keep[-1][2]) or (e[1]=='HIGH' and e[2]>keep[-1][2])):
                keep[-1]=e
            continue
        keep.append(e)
    return keep

tot=[]
for f in sorted(glob.glob('data/2026-*.json')):
    d=json.load(io.open(f))
    s=[x for x in d['snaps']['SPY'] if isinstance(x.get('px'),(int,float)) and x.get('h') and x.get('l')
       and '08:30'<=ct(x['t'])<='15:00']
    if len(s)<40: continue
    med=statistics.median([x['px'] for x in s]); s=[x for x in s if abs(x['px']-med)/med<0.02]
    bars=[(x['t'],x['h'],x['l'],x['px']) for x in s]; A=atr14(bars)
    sw=swings(bars,A)
    tot.append((d['date'], len(sw), [ct(bars[i][0]) for i,_,_ in sw]))

print('%-14s %s'%('session','pullbacks'))
print('-'*58)
for day,n,times in tot: print('%-14s %2d   %s'%(day,n,' '.join(times[:10])))
ns=[n for _,n,_ in tot]
print('\nmedian %.0f per session (was ~120 counting bar-touches)'%statistics.median(ns))

# ==============================================================================================
# THE THREE CONDITIONS HE NAMED, TESTED SEPARATELY.
#   NEW      a node appears at the turn that was not there before
#   MORE     a node that already existed grows into the turn
#   ROLLING  mass left another strike and arrived at this one
# ⚠ They are DIFFERENT claims and must not be pooled: "new" and "more" can both be true of one
# strike, and "rolling" is the only one that asserts anything about WHERE the gamma came from.
#
# THE OUTCOME IS "DID THE TURN HOLD", NOT "DID IT DEFLECT". Every swing reversed — that is the
# definition — so deflect/break is not a question here. The tradeable question is whether the
# reversal SURVIVED: did price come back and take the turning point out?
NEW_MIN   = 8          # % of the day's biggest node — below this an "appearance" is rounding
GROW_MIN  = 8          # % of the biggest node it must ADD to count as growing
LOOK_BACK = 5          # bars before the turn over which growth is measured
HOLD_BARS = 20         # bars the turn must survive to have "held"

def nodesAt(x):
    m={}
    for nd in (x.get('nodes') or []):
        k=nd.get('k'); a=nd.get('abs')
        if isinstance(k,(int,float)) and isinstance(a,(int,float)): m[k]=a
    return m

ev=[]
for f in sorted(glob.glob('data/2026-*.json')):
    d=json.load(io.open(f))
    s=[x for x in d['snaps']['SPY'] if isinstance(x.get('px'),(int,float)) and x.get('h') and x.get('l')
       and '08:30'<=ct(x['t'])<='15:00']
    if len(s)<40: continue
    med=statistics.median([x['px'] for x in s]); s=[x for x in s if abs(x['px']-med)/med<0.02]
    bars=[(x['t'],x['h'],x['l'],x['px']) for x in s]; A=atr14(bars)
    mass=[nodesAt(x) for x in s]
    if not any(mass): continue
    for i,side,px in swings(bars,A):
        if i-LOOK_BACK<0 or not mass[i]: continue
        a=A[i] or 0.4
        ref=max(1.0, max(mass[i].values()) if mass[i] else 1.0)
        # the node nearest the turn, inside the deflection band
        cand=[k for k in mass[i] if (px>=k and (px-k)<=NODE_NEAR*a) or (px<k and (k-px)<=NODE_THRU*a)]
        if not cand: 
            ev.append(dict(day=d['date'],cond='no node',held=None)); continue
        k=min(cand, key=lambda z:abs(z-px))
        now=mass[i].get(k,0.0); then=mass[i-LOOK_BACK].get(k,0.0)
        grew=100.0*(now-then)/ref
        isNew  = (then < NEW_MIN/100.0*ref) and (now >= NEW_MIN/100.0*ref)
        isMore = (not isNew) and grew >= GROW_MIN
        # ROLLING: did any OTHER strike shed roughly what this one gained, over the same window?
        shed=None
        for k2,v2 in mass[i-LOOK_BACK].items():
            if k2==k: continue
            drop=v2-mass[i].get(k2,0.0)
            if drop>0 and abs(drop-(now-then))/max(1.0,abs(now-then)) < 0.5 and drop>=GROW_MIN/100.0*ref:
                shed=k2; break
        isRoll = (grew>0 and shed is not None)
        cond = 'ROLLING' if isRoll else ('NEW' if isNew else ('MORE' if isMore else 'node, flat'))
        # did the turn hold?
        fwd=bars[i+1:i+1+HOLD_BARS]
        if side=='LOW': held = all(b[2] > px - 0.05 for b in fwd) if fwd else None
        else:           held = all(b[1] < px + 0.05 for b in fwd) if fwd else None
        ev.append(dict(day=d['date'],cond=cond,held=held,k=k,grew=round(grew,1),from_=shed))

print('\n' + '='*66)
print('THE THREE CONDITIONS — outcome is "did the turn HOLD %d bars"'%HOLD_BARS)
print('='*66)
byc=collections.Counter(e['cond'] for e in ev)
scored=[e for e in ev if e['held'] is not None]
base=sum(1 for e in scored if e['held'])/len(scored) if scored else 0
print('\n%d pullbacks over %d sessions   held %.0f%% overall\n'%(len(ev),len(set(e['day'] for e in ev)),100*base))
print('%-14s %5s %7s %s'%('at the turn','n','held',''))
print('-'*46)
for c in ['NEW','MORE','ROLLING','node, flat','no node']:
    g=[e for e in ev if e['cond']==c and e['held'] is not None]
    if not g: print('%-14s %5d   —'%(c,byc[c])); continue
    hr=sum(1 for e in g if e['held'])/len(g)
    print('%-14s %5d %6.0f%%  %s'%(c,len(g),100*hr,'(n too small to read)' if len(g)<20 else ''))
print('\n⚠⚠ EVERY CELL IS UNDERPOWERED. gx-004 asks for 150 sessions; there are %d, and a "few per'%len(set(e['day'] for e in ev)))
print('   session" unit means ~3 events a day. This is the MEASUREMENT FRAME being made correct,')
print('   not a result. The frame is the deliverable; the numbers arrive with the sessions.')
