# levels_oh.py -- the MenthorQ intraday level set in force just before the RTH open (08:25 CT = 09:25 ET) for a market / session,
# as the plugin would read it from MenthorQLevels.csv: CR / PS / HVL / GW (0DTE and not), G1-G5.  usage: python -I levels_oh.py MKT YYYY-MM-DD out.txt
import sys, csv
mkt, day, out = sys.argv[1], sys.argv[2], sys.argv[3]
NAME = {'CR': ('CR', 4, 0), 'PS': ('PS', 5, 0), 'HVL': ('HVL', 6, 0), 'CR0': ('CR 0DTE', 0, 1), 'PS0': ('PS 0DTE', 1, 1), 'HVL0': ('HVL 0DTE', 2, 1), 'GW0': ('GW 0DTE', 3, 1)}
for g in range(1, 6): NAME[f'G{g}'] = (f'G{g}', 8 + g, 0)
rows = [r for r in csv.DictReader(open('/mnt/user-data/uploads/level-reversal-analytics/data/menthorq/history/levels_2026-10.csv'))
        if r['market'] == mkt and r['for_session'] == day and r['kind'] == 'intraday' and r['asof_et'] <= day + 'T09:25:00']
last = max(r['asof_et'] for r in rows)
with open(out, 'w') as f:
    for r in rows:
        if r['asof_et'] != last or r['code'] not in NAME: continue
        nm, pr, z = NAME[r['code']]; gex = float(r['gex']) if r['gex'] else 0.0
        f.write(f"{nm},{r['price']},{abs(gex) / 1e6:.2f},{'W' if gex > 0 else 'F' if gex < 0 else '-'},{z},{pr}\n")
print(mkt, day, 'asof', last, 'levels', sum(1 for _ in open(out)))
