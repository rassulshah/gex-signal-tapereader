"""zzo_ref.py - an independent Python model of ZzoDeltaLogic.h (lsZzoDelta 1.3.0 / lsZzoVolume 1.2.0) used by test_zzodelta.cpp:
it writes one line per completed wave (end bar, direction, delta, volume, normals, tags) and the C++ engine must print the same.
Usage: python3 zzo_ref.py <bars.csv> <ES|GC> <tick> > <expected.txt>     (bars: t_end,h,l,vol,delta per line, # comments)
"""
import math, statistics, sys

HIST, MIN_HIST = 5, 3
AB_EFFORT, AB_BACK = 1.9, 0.75
CMX_VOL, COB_VOL, ST_VOL, ST_BACK = 2.5, 1.5, 0.7, 0.75
SW_DELTA, SW_VOL, SW_SIZE, SW_WEAK_VOL = 1.9, 1.5, 1.5, 1.0
NAMES = ["ONH", "ONL", "LonHI", "LonLO", "PDH", "PDL", "PFH", "PFL"]
RTH_T = {"ES": (510, 900), "NQ": (510, 900), "CL": (480, 810), "NG": (480, 810), "GC": (440, 750), "HG": (430, 750), "EU": (440, 840)}
NONE, ON, RTH, POST = 0, 1, 2, 3


def classify(t, per, op, cl):
    days = t // 86400; s = t - days * 86400 - per; d = days
    while s < 0:
        s += 86400; d -= 1
    if s >= 17 * 3600: kind, sess = ON, d + 1
    elif s < op * 60: kind, sess = ON, d
    elif s + per <= cl * 60: kind, sess = RTH, d
    else: kind, sess = POST, d
    wd = (sess + 4) % 7
    if wd in (0, 6): kind = NONE
    return sess * 4 + kind


def med(v):
    v = sorted(v); n = len(v)
    return v[n // 2] if n % 2 else (v[n // 2 - 1] + v[n // 2]) / 2.0


def main():
    path, mkt, tick = sys.argv[1], sys.argv[2], float(sys.argv[3])
    op, cl = RTH_T[mkt]; per = 180
    B = []
    for ln in open(path):
        if not ln.strip() or ln[0] == '#': continue
        t, h, l, v, d = ln.strip().split(',')
        B.append(dict(t=int(t), h=round(float(h) / tick), l=round(float(l) / tick), vol=int(v), dlt=int(d)))
    for b in B: b['seg'] = classify(b['t'], per, op, cl)
    W = []; S = {}
    kind = lambda seg: seg % 4
    sessof = lambda seg: seg // 4

    def base(dirn, seg, f):
        v = [f(w) for w in reversed(W) if w['dir'] == dirn and w['seg'] == seg][:HIST]
        if len(v) < MIN_HIST and kind(seg) == RTH:
            v += [f(w) for w in reversed(W) if w['dir'] == dirn and w['seg'] < seg and kind(w['seg']) == RTH][:HIST - len(v)]
        if len(v) < MIN_HIST: return None
        m = med(v)
        return m if m > 0 else None

    def levels(sess):
        lv = [None] * 8
        if sess not in S: return lv
        s = S[sess]
        if s['on']: lv[0], lv[1] = s['on']
        if s['lon']: lv[2], lv[3] = s['lon']
        for k in sorted([k for k in S if k < sess], reverse=True):
            if S[k]['rth']: lv[4], lv[5] = S[k]['rth']; break
        for k in sorted([k for k in S if k < sess], reverse=True):
            if S[k]['full']: lv[6], lv[7] = S[k]['full']; break
        return lv

    def hl(a, b):
        return (b['h'], b['l']) if a is None else (max(a[0], b['h']), min(a[1], b['l']))

    def add_sess(b):
        k = kind(b['seg'])
        if k == NONE: return
        s = S.setdefault(sessof(b['seg']), dict(full=None, rth=None, on=None, lon=None, swept=0))
        s['full'] = hl(s['full'], b)
        if k == RTH: s['rth'] = hl(s['rth'], b)
        if k == ON:
            s['on'] = hl(s['on'], b)
            sec = b['t'] - (b['t'] // 86400) * 86400
            if 2 * 3600 < sec <= min(7 * 3600 + 1800, op * 60): s['lon'] = hl(s['lon'], b)
        while len(S) > 40: del S[min(S)]

    def make(s, e, d):
        w = dict(s=s, e=e, dir=d, dlt=sum(B[i]['dlt'] for i in range(s, e + 1)), vol=sum(B[i]['vol'] for i in range(s, e + 1)), seg=B[e]['seg'])
        w['med'] = base(d, w['seg'], lambda x: abs(x['dlt'])); w['medV'] = base(d, w['seg'], lambda x: x['vol'])
        p0 = s - 1 if s > 0 else 0
        w['ext'] = B[e]['h'] if d > 0 else B[e]['l']
        w['size'] = B[e]['h'] - B[p0]['l'] if d > 0 else B[p0]['h'] - B[e]['l']
        w['medS'] = base(d, w['seg'], lambda x: x['size'])
        return w

    def absorb(w):
        if w['med'] is None: return 0
        E = w['dir'] * w['dlt']; Er = E / w['med']
        if not Er > AB_EFFORT: return 0
        ps = next((x for x in reversed(W) if x['seg'] == w['seg'] and x['dir'] == w['dir']), None)
        po = next((x for x in reversed(W) if x['seg'] == w['seg'] and x['dir'] != w['dir']), None)
        if ps is None or po is None: return 0
        pd = abs(po['dlt'])
        if pd > 0 and E / pd >= 1.0 and po['size'] > 0 and w['size'] / po['size'] <= AB_BACK: return 1
        if w['dir'] * (w['ext'] - ps['ext']) <= 0: return 2
        return 0

    def voltags(w):
        w['cmx'] = w['ar'] = w['st'] = False
        if w['medV'] is None: return
        Vr = w['vol'] / w['medV']
        same = [x for x in reversed(W) if x['seg'] == w['seg'] and x['dir'] == w['dir']][:5]
        opp = [x for x in reversed(W) if x['seg'] == w['seg'] and x['dir'] != w['dir']][:2]
        c = Vr >= CMX_VOL and len(same) >= 3 and len(opp) >= 2
        if c and any(w['vol'] <= x['vol'] for x in same): c = False
        if c: c = w['dir'] * (same[0]['ext'] - same[1]['ext']) > 0 and w['dir'] * (opp[0]['ext'] - opp[1]['ext']) > 0
        if c:
            prev = []
            i = w['s'] - 1
            while i >= 0 and B[i]['seg'] == w['seg']:
                prev.append(B[i]['h'] if w['dir'] > 0 else B[i]['l']); i -= 1
            c = bool(prev) and w['dir'] * (w['ext'] - (max(prev) if w['dir'] > 0 else min(prev))) > 0
        w['cmx'] = c
        if W:
            p = W[-1]
            if p['cmx'] and p['dir'] != w['dir'] and p['seg'] == w['seg'] and Vr >= COB_VOL: w['ar'] = True
        if len(W) > 1:
            a, cc = W[-1], W[-2]
            if a['ar'] and cc['cmx'] and cc['dir'] == w['dir'] and a['seg'] == w['seg'] and cc['seg'] == w['seg'] and w['vol'] <= ST_VOL * cc['vol'] and a['size'] > 0 and w['size'] >= ST_BACK * a['size']:
                w['st'] = True

    def cob(w):
        w['cobTag'] = 0; w['swLv'] = 0
        if not W: return
        p = W[-1]
        if not p['sw'] or p['seg'] != w['seg'] or p['dir'] == w['dir']: return
        w['swLv'] = p['sw']
        if w['med'] is None or w['medV'] is None or w['medS'] is None: return
        Er = w['dir'] * w['dlt'] / w['med']; Vr = w['vol'] / w['medV']; Sr = w['size'] / w['medS']
        if Er >= SW_DELTA and Vr >= SW_VOL and Sr >= SW_SIZE: w['cobTag'] = 1
        elif Vr < SW_WEAK_VOL or Er < 0: w['cobTag'] = 2

    def sweeps(w):
        w['sw'] = 0
        if kind(w['seg']) != RTH: return
        ss = sessof(w['seg']); lv = levels(ss)
        hs = []; i = w['s'] - 1
        while i >= 0 and B[i]['seg'] == w['seg']:
            hs.append(B[i]); i -= 1
        hi = max(b['h'] for b in hs) if hs else None; lo = min(b['l'] for b in hs) if hs else None
        s = S.setdefault(ss, dict(full=None, rth=None, on=None, lon=None, swept=0))
        for k in range(8):
            if lv[k] is None or s['swept'] & (1 << k): continue
            if k % 2 == 0 and w['dir'] > 0 and w['ext'] > lv[k] and (hi is None or hi <= lv[k]): w['sw'] |= 1 << k; s['swept'] |= 1 << k
            if k % 2 == 1 and w['dir'] < 0 and w['ext'] < lv[k] and (lo is None or lo >= lv[k]): w['sw'] |= 1 << k; s['swept'] |= 1 << k

    dirn = ext = start = 0; anyw = False
    for j in range(len(B)):
        if j == 1: dirn = 1 if B[1]['h'] >= B[0]['h'] else -1; ext = 0; start = 0
        done = None
        if j >= 1:
            if dirn == 1:
                if B[j]['h'] > B[ext]['h']: ext = j
                elif B[j]['l'] < B[j - 1]['l'] and j != ext:
                    done = (start + 1 if anyw else start, ext, 1); start = ext; ext = j; dirn = -1; anyw = True
            else:
                if B[j]['l'] < B[ext]['l']: ext = j
                elif B[j]['h'] > B[j - 1]['h'] and j != ext:
                    done = (start + 1 if anyw else start, ext, -1); start = ext; ext = j; dirn = 1; anyw = True
        if done:
            w = make(*done); w['ab'] = absorb(w); voltags(w); cob(w); sweeps(w); W.append(w)
        add_sess(B[j])
    f = lambda x: 'na' if x is None else '%.1f' % x
    for w in W:
        print('%d,%d,%d,%d,%s,%s,%s,%d,%d,%d,%d,%d,%d,%d' % (w['e'], w['dir'], w['dlt'], w['vol'], f(w['med']), f(w['medV']), f(w['medS']), w['ab'], int(w['cmx']), int(w['ar']), int(w['st']), w['sw'], w['cobTag'], w['swLv']))


if __name__ == '__main__':
    main()
