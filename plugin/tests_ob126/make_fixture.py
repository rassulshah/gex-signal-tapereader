# make_fixture.py <MKT> <session> <out>: 3-min bars (bar END stamps, CT) with every price's bid / ask lots from the raw MBO trades (T)
import duckdb, sys, pandas as pd
m, s, out = sys.argv[1], sys.argv[2], sys.argv[3]
f = f'/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store/raw/m={m}/session={s}/part.parquet'
c = duckdb.connect()
d = c.execute(f"select ts, side, px, sz from read_parquet('{f}') where act='T' and side in ('A','B') order by ts").df()
t = pd.to_datetime(d.ts).dt.tz_localize('UTC').dt.tz_convert('America/Chicago').dt.tz_localize(None)
d['end'] = t.dt.floor('3min') + pd.Timedelta(minutes=3)
rows = []
for end, g in d.groupby('end', sort=True):
    o, h, l, cl = g.px.iloc[0], g.px.max(), g.px.min(), g.px.iloc[-1]
    v = int(g.sz.sum())
    cells = g.groupby('px').apply(lambda x: (int(x.loc[x.side == 'B', 'sz'].sum()), int(x.loc[x.side == 'A', 'sz'].sum())))
    cs = ';'.join(f'{p:.2f}:{a}:{b}' for p, (a, b) in cells.items())   # price:ask(bought):bid(sold)
    rows.append(f"{end:%Y-%m-%d %H:%M}|{o:.2f}|{h:.2f}|{l:.2f}|{cl:.2f}|{v}|{cs}")
open(out, 'w').write('end|open|high|low|close|volume|cells(px:ask:bid)\n' + '\n'.join(rows) + '\n')
print(out, len(rows), rows[0][:60], rows[-1][:40])
