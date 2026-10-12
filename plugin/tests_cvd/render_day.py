"""render_day.py frame.csv bars.csv out.png first_end last_end title -- draws the dumped lsCVD frame (plus the price candles above it, for context)."""
import sys, pandas as pd
from PIL import Image, ImageDraw, ImageFont
fr, barsf, outp, t0, t1, title = sys.argv[1:7]
LV = [(x.split(':')[0], float(x.split(':')[1])) for x in (sys.argv[7].split(',') if len(sys.argv) > 7 else [])]
F = pd.read_csv(fr, keep_default_na=False)
B = pd.concat([pd.read_csv(x) for x in barsf.split('+')]); B['end'] = pd.to_datetime(B.t_ct) + pd.Timedelta(minutes=3)
B = B[B.end <= t1].tail(200).reset_index(drop=True)   # the plugin's view: the last 200 chart bars
W, PH, CH, TOP = 1500, 470, 300, 46
img = Image.new('RGB', (W, TOP + CH + PH + 30), (11, 18, 32)); d = ImageDraw.Draw(img)
try:
    f = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 9); fb = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 16)
except Exception:
    f = fb = ImageFont.load_default()
col = lambda c: ((int(c) >> 16) & 255, (int(c) >> 8) & 255, int(c) & 255)
d.text((10, 10), title, fill=(230, 235, 245), font=fb)
# price candles (context only - the chart's own price pane in IRT)
n = len(B); bp = 1460 // 200; x0 = 50 + (200 - n) * bp
lo, hi = B.l.min(), B.h.max(); y = lambda p: TOP + 8 + (CH - 16) * (hi - p) / (hi - lo)
d.rectangle([0, TOP, W, TOP + CH], fill=(15, 23, 42))
for i, r in B.iterrows():
    x = x0 + i * bp; c = (52, 211, 153) if r.c >= r.o else (248, 113, 113)
    d.line([x, y(r.h), x, y(r.l)], fill=c); d.rectangle([x - 2, y(max(r.o, r.c)), x + 2, y(min(r.o, r.c)) + 1], fill=c)
d.text((6, TOP + 4), 'price (IRT price pane, for context)', fill=(120, 130, 150), font=f)
for name, p in LV:
    if lo <= p <= hi:
        d.line([0, y(p), W, y(p)], fill=(234, 140, 50), width=1); d.text((W - 90, y(p) - 12), f'{name} {p:g}', fill=(234, 140, 50), font=f)
# the CVD pane exactly as the plugin drew it
oy = TOP + CH + 10
d.rectangle([0, oy, W, oy + PH], fill=(17, 24, 39))
for _, r in F.iterrows():
    if r.type == 'box':
        fill = col(r.color); border = col(int(r.text)) if r.text else fill
        d.rectangle([r.x, oy + r.y, r.r - 1, oy + r.b - 1], fill=fill, outline=border)
    elif r.type == 'line':
        d.line([r.x, oy + r.y, r.r, oy + r.b], fill=col(r.color), width=int(r.text or 1))
for _, r in F.iterrows():
    if r.type == 'text':
        d.text((r.x, oy + r.b), r.text.replace(';', ','), fill=col(r.color), font=f)
img.save(outp)
