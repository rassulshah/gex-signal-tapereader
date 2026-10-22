#!/usr/bin/env python3
"""check_hovers_off.py (2026-10-18 23:37 CT, Rassul "get rid of the hover over on the gex levels and other places").
Source contract for every PRICE-CHART hover: the switch is a compile-time false constant, mouse() returns before it reads the event,
the hover draw is gated by the switch, and CreateExtension asks IRT for TRACK_MOUSE only through the switch (TapeReader keeps
TRACK_MOUSE for the chart-wide candle follow - its banner popup is what is switched off). Runtime companion for the Liquidity
Profile: tests_lqp044/test_lqp453_host.cpp (3,400+ mouse events -> no invalidate, identical draw, colours). Run from plugin\\: python3 tests_hover1018/check_hovers_off.py"""
import re, sys, os
os.chdir(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
fails = 0; passes = 0
def check(c, m):
    global fails, passes
    if c: passes += 1
    else: fails += 1; print('FAIL', m)
def body(src, sig):
    i = src.find(sig); assert i >= 0, sig
    j = src.index('{', i); return src[j + 1:j + 400]
def ext_flags(src):
    i = src.find('CreateExtension'); s = src[i:]
    return [l for l in s.splitlines() if 'setFlags(' in l]
P = {   # plugin: (switch, mouse signature, hover-draw gate text, TRACK_MOUSE allowed)
    'LiquidityProfile.cpp': ('LQ_HOVERS', 'int LiquidityProfile::mouse(RTX_EVENT* e)', ['if (!LQ_HOVERS) { S.boHovKey = 0; return; }', 'if (!LQ_HOVERS) { S.hovId = 0; S.hovState = "off"; return; }'], False),
    # (DP 2.19.0, Rassul approved Option B 2026-10-21) the ONE exception: the level % block's hover (DP_PCT_HOVERS); every other DP hover off
    'DealerProfile.cpp':    ('DP_HOVERS', 'int DealerProfile::mouse(RTX_EVENT* e)', ['(DP_HOVERS || DP_PCT_HOVERS) ? hovered() : nullptr', 'if ((DP_HOVERS || hovT[i].pct) && hitTarget(hovT[i]))'], False),
    'BigBidAsk.cpp':        ('BBA_HOVERS', 'int BigBidAsk::mouse(RTX_EVENT* e)', ['if (!BBA_HOVERS || !S.msHas'], False),
    'CobImpulse.cpp':       ('CI_HOVERS', 'int CobImpulse::mouse(RTX_EVENT* e)', ['if (!CI_HOVERS || !S.msHas'], False),
    'TapeReader.cpp':       ('TR_BANNER_HOVER', None, ['const bool in = TR_BANNER_HOVER && '], True),
}
for f, (sw, msig, gates, allowTM) in P.items():
    s = open(f, encoding='latin-1').read()
    check(re.search(r'static const bool ' + sw + r' = false;', s) is not None, f + ': ' + sw + ' = false')
    if msig:
        b = body(s, msig)
        first = [l for l in b.split('\n') if l.strip() and not l.strip().startswith(('#', '//'))][0]   # first statement (skip #ifndef / comments)
        check(sw in first and 'return RTX_FAIL' in first, f + ': mouse() returns RTX_FAIL first while ' + sw + ' is off')
    for g in gates: check(g in s, f + ': hover draw gated: ' + g)
    fl = ext_flags(s)
    if f == 'LiquidityProfile.cpp': fl = [l for l in fl if 'OVERLAY' in l.split('//')[0]]          # the price chart build (the Flow pane keeps its own)
    check(len(fl) >= 1, f + ': setFlags found')
    for l in fl:
        code = l.split('//')[0]
        if allowTM: check('TRACK_MOUSE' in code, f + ': TRACK_MOUSE kept for the candle follow')
        elif f == 'DealerProfile.cpp': check(re.search(r'\|\s*TRACK_MOUSE\b', code) is None and '((DP_HOVERS || DP_PCT_HOVERS) ? TRACK_MOUSE : 0)' in code, f + ': TRACK_MOUSE only through DP_HOVERS / DP_PCT_HOVERS')
        else: check(re.search(r'\|\s*TRACK_MOUSE\b', code) is None and ('(' + sw + ' ? TRACK_MOUSE : 0)') in code, f + ': TRACK_MOUSE only through ' + sw)
# HodLod (2.4.6) already off - leave it, but prove it stays off
s = open('HodLod.cpp', encoding='latin-1').read()
check('static const bool HL_HOVERS = false;' in s, 'HodLod.cpp: HL_HOVERS = false (2.4.6)')
# versions: the constant, the setVersion literal and the settings table agree
V = {'LiquidityProfile.cpp': ('LQ_VERSION "', 'LiquidityProfileSettings.h', 'LiquidityProfileLogic.h'),
     'DealerProfile.cpp': ('DP_VERSION = "', 'DealerProfileSettings.h', None),
     'BigBidAsk.cpp': ('BBA_VERSION "', None, 'BigBidAskLogic.h'),
     'CobImpulse.cpp': ('CI_VERSION "', 'CobImpulseSettings.h', None),
     'TapeReader.cpp': ('TR_VERSION "', 'TapeReaderSettings.h', 'TapeReaderLogic.h')}
for f, (k, st, lg) in V.items():
    s = open(f, encoding='latin-1').read()
    v = s[s.index(k) + len(k):].split('"')[0]
    sv = re.search(r'setVersion\("([^"]+)"\)', s).group(1)
    check(v == sv, f + ': ' + v + ' == setVersion ' + sv)
    if st: check('PLUGIN_VERSION = "' + v + '"' in open(st, encoding='latin-1').read(), st + ': PLUGIN_VERSION ' + v)
    if lg: check(('VERSION = "' + v + '"') in open(lg, encoding='latin-1').read(), lg + ': VERSION ' + v)
print('hovers off contract: %d passed, %d failed' % (passes, fails))
sys.exit(1 if fails else 0)
