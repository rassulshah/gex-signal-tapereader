"""lsTurn 1.2.0, the default (no display file = JUST EFFORT) on Friday's ES morning (44 candles), from the plugin's own draw lists at five zooms:
every closed candle has ONE bar a side, centred, with the height of a calculation made apart from the plugin (pandas: data/eff_ref_ES.csv) and
green / red exactly when the number written on it is 1.5 or more; nothing else is drawn under the top line; the top line is the last closed candle."""
import pandas as pd
ref = pd.read_csv('data/eff_ref_ES.csv').set_index('key'); K0 = 1795163040
G, R, A = '22c55e', 'ef4444', '64748b'
def load(f): return [l.rstrip('\n').split('|', 10) for l in open(f, encoding='cp1252') if l.count('|') >= 10]
def barpx(v, half): return max(2, round(min(v, 3.0) / 3.0 * half + 1e-9))
def label(v): return '99+' if v >= 99.5 else ('%.0f' % v if v >= 9.95 else '%.1f' % v)
bad = 0
for z in (60, 22, 12, 6, 3):
    d = load(f'out/v12/def_{z}_draws.txt'); bx = [c for c in d if c[0] == 'box']; tx = [c for c in d if c[0] == 'text']
    axis = [int(c[2]) for c in bx if int(c[4]) - int(c[2]) == 1 and int(c[3]) - int(c[1]) > 100][0]
    strip = any(int(c[2]) == 2 and int(c[4]) == 20 for c in bx); top = 35 if strip else 15; half = max(4, (157 - top) // 2 - 1)
    w = max(1, min(z - (6 if z >= 12 else 2 if z >= 6 else 1), 40)); x0 = 20 + z // 2
    bars = [c for c in bx if int(c[2]) > 21 and int(c[4]) - int(c[2]) != 1] if strip else [c for c in bx if int(c[4]) - int(c[2]) != 1 or int(c[3]) - int(c[1]) < 100]
    bars = [c for c in bars if not (int(c[4]) - int(c[2]) == 1 and int(c[3]) - int(c[1]) > 100)]
    want = {}; col = 0
    for i in range(43):
        key = K0 + 180 * i; x = x0 + i * z; l = x - w // 2
        for side, v in ((0, ref.loc[key, 'top']), (1, ref.loc[key, 'bot'])):
            v = round(v * 1000) / 1000; h = barpx(v, half); on = float(label(v).rstrip('+')) >= 1.5; col += on
            want[(l, axis - h, l + w, axis) if side == 0 else (l, axis + 1, l + w, axis + 1 + h)] = (G if side == 0 else R) if on else A
    got = {(int(c[1]), int(c[2]), int(c[3]), int(c[4])): c[5] for c in bars}
    labs = [c for c in tx if int(c[2]) > 21]; wrongcol = [c for c in labs if (float(c[10].rstrip('+')) >= 1.5) != (c[5] != A)]
    st = ' '.join(c[10] for c in tx if int(c[2]) <= 21)
    ok = got == want and not wrongcol and (not strip or st in ('10:30 AM Buyers Light Effort 0.6x / Sellers Light Effort 0.6x', '10:30 AM Buyers Light Effort / Sellers Light Effort', 'Buyers Light Effort / Sellers Light Effort'))   # a narrow pane drops the numbers, then the time
    print(f'{z:>2} px: bars {len(got)} = the 86 of the independent calculation: {got == want}; coloured {sum(v != A for v in got.values())} (expected {col}); numbers {len(labs)}, each coloured as its bar says: {not wrongcol}; top line: {st if strip else "(pane too narrow)"}')
    bad += 0 if ok else 1
print('OK' if not bad else 'DIFFERENCES')
