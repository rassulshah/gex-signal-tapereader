"""lsTurn 1.2.0 with the saved full setting (display-full.txt: every piece on, coloured from 0.5) against 1.1.0, Friday's ES morning, five
zooms, with and without the mouse: the same draws in the same order; the only difference allowed = an Effort bar that READS 0.5 (its value
was 0.45 - 0.499) is coloured now (the colour follows the number written), box and number."""
G, R, A = '22c55e', 'ef4444', '64748b'
def load(f): return [l.rstrip('\n').split('|', 10) for l in open(f, encoding='cp1252') if l.count('|') >= 10]
tot = 0
for z in (60, 22, 12, 6, 3):
    for kind in ('draws', 'mouse'):
        a = load(f'out/v12/v110_{z}_{kind}.txt'); b = load(f'out/v12/full_{z}_{kind}.txt')
        if len(a) != len(b): print(z, kind, 'DIFFERENT LENGTH', len(a), len(b)); tot += 1; continue
        diff = [(x, y) for x, y in zip(a, b) if x != y]
        ok = [d for d in diff if d[0][:5] == d[1][:5] and d[0][5] == A and d[1][5] in (G, R) and d[0][10] == d[1][10] and d[0][10] in ('', '0.5')]
        other = [d for d in diff if d not in ok]
        print(f'{z:>2} px {kind}: {len(a)} draws, differences {len(diff)}: grey -> coloured on Effort bars that read 0.5: {len(ok)}; anything else: {len(other)}')
        tot += len(other)
print('OK' if not tot else 'DIFFERENCES')
