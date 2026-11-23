"""1.0.1 against 1.1.0 on Friday's ES morning (44 candles), from the plugin's own draw lists at several zooms:
(1) everything 1.0.1 drew is still drawn, at the same place in the same colour (strip, axis, every bar); (2) what is new is Effort bars only,
and each one equals the calculation made apart from the plugin (pandas: data/eff_ref_ES.csv); (3) the labels."""
import sys, pandas as pd
ref = pd.read_csv('data/eff_ref_ES.csv').set_index('key'); K0 = 1795163040; SW = {1795164660, 1795165380}
G, R, A = '22c55e', 'ef4444', '64748b'
def load(f):
    out = []
    for ln in open(f, 'rb').read().decode('cp1252').splitlines():
        c = ln.split('|', 10)
        if len(c) == 11: out.append((c[0], int(c[1]), int(c[2]), int(c[3]), int(c[4]), c[5], c[10]))
    return out
def barpx(v, half): return max(2, round(min(v, 3.0) / 3.0 * half + 1e-9))
bad = 0
for z in (60, 22, 12, 6, 3):
    o, n = load(f'out/z/old_{z}_draws.txt'), load(f'out/z/new_{z}_draws.txt')
    ob = [d for d in o if d[0] == 'box']; nb = [d for d in n if d[0] == 'box']; ot = [d for d in o if d[0] == 'text']; nt = [d for d in n if d[0] == 'text']
    miss = [d for d in ob if d not in nb]                                   # (1) every box of 1.0.1
    strip_same = [d for d in ot if d[2] <= 21] == [d for d in nt if d[2] <= 21]
    ol = [d for d in ot if d[2] > 21]; nl = [d for d in nt if d[2] > 21]; lost = [d for d in ol if d not in nl]
    new = [d for d in nb if d not in ob]
    axis = [d for d in nb if d[4] - d[2] == 1 and d[3] - d[1] > 100][0][2]; top = 35 if any(d[2] == 2 and d[4] == 20 for d in nb) else 15; half = max(4, (157 - top) // 2 - 1)   # 3 px: the pane is too narrow for the strip
    gap = 2 if z >= 12 else 1 if z >= 6 else 0; w = max(1, min((z - gap - 2) // 2, 20)); x0 = 20 + z // 2
    wrong = 0; seen = 0
    for i in range(43):                                                     # (2) every closed candle
        key = K0 + 180 * i; x = x0 + i * z; xr = x + (gap - gap // 2); top, bot = ref.loc[key, 'top'], ref.loc[key, 'bot']
        up = [d for d in nb if d[1] == xr and d[3] == xr + w and d[4] == axis and d[4] - d[2] > 1 or (d[1] == xr and d[3] == xr + w and d[4] == axis and z < 6)]
        dn = [d for d in nb if d[1] == xr and d[3] == xr + w and d[2] == axis + 1]
        eu = (barpx(round(top * 1000) / 1000, half), G if round(top * 1000) >= 500 else A)
        ok = len(up) == 1 and (up[0][4] - up[0][2], up[0][5]) == eu
        if key not in SW:
            ed = (barpx(round(bot * 1000) / 1000, half), R if round(bot * 1000) >= 500 else A)
            ok = ok and len(dn) == 1 and (dn[0][4] - dn[0][2], dn[0][5]) == ed
        seen += 1; wrong += 0 if ok else 1
        if not ok and wrong <= 3: print('   candle', i, pd.Timestamp(key, unit='s').strftime('%H:%M'), 'up', up, 'want', eu, 'dn', dn)
    newcols = {d[1] for d in new}; okcols = {x0 + i * z + (gap - gap // 2) for i in range(44)}
    stray = [d for d in new if d[1] not in okcols or d[5] not in (G, R, A)]
    print(f'{z:>2} px: 1.0.1 boxes {len(ob)} all still drawn: {not miss}; strip the same: {strip_same}; new boxes {len(new)} (all Effort bars in the right column: {not stray}); '
          f'closed candles {seen}, Effort = the independent calculation on {seen - wrong}; labels {len(ol)} -> {len(nl)}, of the old ones {len(ol) - len(lost)} kept')
    bad += len(miss) + (0 if strip_same else 1) + len(stray) + wrong
print('DIFFERENCES' if bad else 'OK')
