#!/usr/bin/env python3
"""
COUNTING DEFLECTIONS THE WAY THE OPERATOR DEFINES THEM.

His teaching, 2026-08-30, with five circled examples on QQQ and SPXW charts:

  "price tested a node and deflected away from it ... in each circle you only count it as 1
   deflection otherwise you will be counting node deflections for every 3min bar that is next to
   each other that is touching the node, which is incorrect ... price doesn't have to touch it
   exactly. It can be very close to it, touch it, go through it a little bit and reverse."

⚠⚠ THE WHOLE POINT IS THE EPISODE. A naive count marks every bar whose wick is inside the zone, so a
node price sits on for twenty minutes scores seven deflections instead of one. Each contiguous visit
is ONE event. This is the same shape as `kingTapsCross`, which already counts TAP EPISODES with an
`inTap` latch — the pattern existed and the deflection code did not use it.

⚠ THE PANEL'S OWN CONSTANTS ARE REUSED, NOT RE-INVENTED. `deflectionAt()` in the userscript already
defines the zone, the reversal and the confirmation; it is LIVE-only (it looks for the MOST RECENT
tap in an 8-bar window) and has no episode notion. Same thresholds here so a study number and a face
number can never mean different things.

  DEFLECT_ZONE    0.50   enter within this of the node = a touch (SPY/QQQ strikes)
  DEFLECT_AWAY    0.45   must reverse away by at least this
  DEFLECT_CONFIRM 2      the reversal must hold for this many closed bars

⚠ PENETRATION IS ALLOWED AND IS NOT A BREAK. Price may pass THROUGH the node and still deflect - the
test is which side it ENDS on relative to the side it ARRIVED from, not whether it stayed clear.
A visit that ends on the far side, sustained, is a BREAK and is counted separately.
"""
import json, io, glob, datetime

# ⚠⚠ CALIBRATED AGAINST HIS OWN CIRCLES, 2026-08-30. He marked ~12 deflections and one BREAK across
# 2026-08-25/26/27 on SPY 3m. Looking up the node behind every circled price:
#
#     circled 763.20 -> node 763  0.20 away   peak  89%King
#     circled 765.90 -> node 766  0.10 away   peak 100%
#     circled 765.20 -> node 765  0.20 away   peak 100%
#     circled 766.20 -> node 766  0.20 away   peak  48%
#     circled 768.30 -> node 768  0.30 away   peak 100%
#
# TWO THINGS FALL OUT AND BOTH WERE WRONG BEFORE:
#  1. EVERY circle is within 0.30 of its node, not 0.50. A 0.50 band admits visits that never
#     reached the node, which is where the over-count came from — six "deflections" in 30 minutes on
#     2026-08-27, all of them "stopped short", none of which he would have circled.
#  2. EVERY node he circled is a STRONG one — 48% to 100% of King. MIN_PCT of 20 was letting minor
#     shelves count as tests. He does not circle those.
#
# ⚠ AND HE LABELLED A BREAK: 2026-08-26 ~12:00, price through the 765 node and CONTINUING down to
# ~764. That is the counter-example the classifier needs — same approach, same touch, opposite
# outcome — and it is why `classify` decides on which side price ENDS, never on whether it touched.
# ⚠⚠ SECOND CALIBRATION, 2026-08-30 — he sent a chart with DEFLECTIONS IN WHITE AND BREAKDOWNS IN
# RED across 2026-08-20/21/24, and the chart header named the node set outright:
#     765.0: +88.4M   768.0: +54.7M   770.0: +52.9M   764.0: +42.1M   766.0: +32.2M
# He is watching the TOP FEW NODES BY DOLLARS. Checking all 12 of his marks against the recorded
# trinity: EVERY ONE is within 0.40 of a node (most within 0.30, several exactly 0.00) and EVERY
# node is in the TOP 5 on 60-127 of ~125 bars. A %King floor was the wrong instrument - it is a
# RANK question, not a threshold question, and rank is what the chart draws.
#
# ⚠⚠ AND THE SAME NODE GIVES BOTH OUTCOMES. On 2026-08-24 node 764 carries one WHITE deflection and
# TWO RED breakdowns. So a node is never "a deflection node": the node selects WHERE, the price
# action decides WHAT. Any model that scores nodes as reliable-or-not has already mis-framed it.
# "breakout would just be the opposite of the breakdowns" - so a break carries its DIRECTION.
TOPN    = 5             # the node universe: strikes ranked in the top N by mass at that moment
ZONE, AWAY, CONFIRM = 0.40, 0.45, 2
REACH   = 0.40          # his furthest mark from its node was 0.40; nothing beyond that counts

def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000-5*3600).strftime('%H:%M')

REARM = 2.0 * ZONE        # price must get this far clear of the node before a NEW test can count

def episodes(bars, k):
    """bars = [(t, hi, lo, close)] -> list of visits to the zone around k. One per contiguous run."""
    out, cur = [], None
    for i, (t, hi, lo, c) in enumerate(bars):
        inside = (lo <= k + ZONE) and (hi >= k - ZONE)
        if inside:
            if cur is None:
                # which side did price ARRIVE from? the last close outside the zone.
                side = 0
                for j in range(i-1, -1, -1):
                    pc = bars[j][3]
                    if abs(pc - k) > ZONE:
                        side = 1 if pc > k else -1
                        break
                cur = {'i0': i, 'from': side, 'lo': lo, 'hi': hi}
            else:
                cur['lo'] = min(cur['lo'], lo); cur['hi'] = max(cur['hi'], hi)
            cur['i1'] = i
        elif cur is not None:
            out.append(cur); cur = None
    if cur is not None: out.append(cur)
    # ⚠⚠ RE-ARM, AND THIS IS THE HALF I GOT WRONG FIRST. Leaving the zone by a tick and stepping back
    # in is the SAME test continuing, not a second one. Without this, node 765 on 2026-08-27 scored
    # four "deflections" at 08:42 / 08:51 / 09:00 / 09:09 — one consolidation against the node, cut
    # into four by a few bars that happened to close just outside. His rule: one circle, one
    # deflection. So after a visit, price must get clear by REARM and STAY clear before the next
    # visit is a new event.
    merged=[]
    for ep in out:
        if merged:
            prev=merged[-1]
            gap=bars[prev.get('i1',prev['i0'])+1: ep['i0']]
            cleared=any(abs(c-k) >= REARM for (_,_,_,c) in gap)
            if not cleared:
                prev['i1']=ep.get('i1',ep['i0'])
                prev['lo']=min(prev['lo'],ep['lo']); prev['hi']=max(prev['hi'],ep['hi'])
                continue
        merged.append(ep)
    return merged

def classify(bars, k, ep):
    """after the visit ends: deflected back the way it came, broke through, or neither."""
    end = ep.get('i1', ep['i0'])
    after = bars[end+1: end+1+CONFIRM+4]
    if len(after) < CONFIRM: return None
    arrived = ep['from']
    if arrived == 0: return None                     # gapped into it; no side to reverse toward
    held = 0; broke = 0
    for (t, hi, lo, c) in after:
        if abs(c - k) < AWAY: continue
        if (c - k) * arrived > 0: held += 1          # back on the side it came from = DEFLECTED
        else: broke += 1                             # ended on the far side = BROKE
    if held >= CONFIRM: 
        pen = (ep['hi'] - k) if arrived < 0 else (k - ep['lo'])
        return ('deflect', round(pen, 2))
    if broke >= CONFIRM: return ('break', None)
    return None

rows=[]
for f in sorted(glob.glob('data/2026-*.json')):
    d=json.load(io.open(f))
    s=[x for x in d['snaps']['SPY']
       if isinstance(x.get('px'),(int,float)) and x.get('h') and x.get('l')
       and '08:30' <= ct(x['t']) <= '15:00']
    if len(s) < 40: continue
    bars=[(x['t'], x['h'], x['l'], x['px']) for x in s]
    # the nodes that existed that day, from the trinity tops
    # the node universe is a RANK, not a threshold: strikes that spent real time in the top N
    import collections as _c
    rank=_c.Counter()
    for x in s:
        for i,(k,pct) in enumerate(((x.get('tri') or {}).get('SPY',{}) or {}).get('top', [])):
            if i < TOPN: rank[k]+=1
    nodes={k:n for k,n in rank.items() if n >= 0.25*len(s)}   # present in the top N a quarter of the day
    dfl=brk=0; naive=0; detail=[]
    for k in sorted(nodes):
        eps=episodes(bars, k)
        for ep in eps:
            naive += (ep.get('i1', ep['i0']) - ep['i0'] + 1)     # what counting every bar would give
            # ⚠ THE VISIT MUST HAVE REACHED THE NODE. Entering a band is not testing a level.
            reached = (ep['lo'] <= k + REACH) and (ep['hi'] >= k - REACH)
            if not reached: continue
            r=classify(bars, k, ep)
            if not r: continue
            if r[0]=='deflect':
                dfl+=1; detail.append((ct(bars[ep['i0']][0]), k, r[1], 'deflect'))
            else:
                # a break carries its DIRECTION — down through the node, or up through it
                brk+=1; detail.append((ct(bars[ep['i0']][0]), k, None,
                                       'BREAKDOWN' if ep['from']>0 else 'BREAKOUT'))
    rows.append({'d':d['date'],'nodes':len(nodes),'deflect':dfl,'break':brk,'naive':naive,'detail':detail})
    print('%s  nodes %2d   DEFLECTIONS %2d   breaks %2d   (naive bar-count would say %3d)'
          % (d['date'], len(nodes), dfl, brk, naive))
    for t,k,pen,kind in detail[:8]:
        print('        %s  %-9s node %-7s %s' % (t, kind, k,
              '' if pen is None else (('penetrated %.2f'%pen) if pen>0 else ('reached to %.2f'%abs(pen)))))

if rows:
    D=sum(r['deflect'] for r in rows); B=sum(r['break'] for r in rows); N=sum(r['naive'] for r in rows)
    n=len(rows)
    print('\n%d sessions   %d deflections (%.1f/session)   %d breaks   deflect rate %.0f%%'
          % (n, D, D/n, B, 100.0*D/(D+B) if (D+B) else 0))
    print('⚠ counting every touching BAR instead of every VISIT would have reported %d — %.0fx too many.'
          % (N, N/D if D else 0))
