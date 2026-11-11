"""render.py - draws.txt (render_dump) -> PNG: the candles and exactly what the plugin drew (the render check before delivery)"""
import sys
from PIL import Image, ImageDraw
L = [l.split() for l in open(sys.argv[1])]; W, H, bw = map(int, L[0][1:4]); S = 2
im = Image.new("RGB", (W * S, (H + 20) * S), (16, 19, 22)); d = ImageDraw.Draw(im)
for a in L[1:]:
    if a[0] == "C":
        x = int(a[1]); o, h, l, c = map(float, a[2:6]); col = (38, 120, 70) if c <= o else (150, 50, 50)
        d.line([(x * S, h * S), (x * S, l * S)], fill=col, width=1); d.rectangle([((x - 2) * S, min(o, c) * S), ((x + 2) * S, max(o, c) * S + 1)], fill=col)
for a in L[1:]:
    if a[0] == "L":
        x0, y0, x1, y1 = map(int, a[1:5]); col = tuple(int(a[5][i:i + 2], 16) for i in (0, 2, 4)); w = int(a[6])
        if int(a[7]) == 2 and y0 == y1:
            for x in range(min(x0, x1), max(x0, x1), 8): d.line([(x * S, y0 * S), (min(x + 3, max(x0, x1)) * S, y0 * S)], fill=col, width=w * S // 2 or 1)
        else: d.line([(x0 * S, y0 * S), (x1 * S, y1 * S)], fill=col, width=w * S)
    elif a[0] == "O":
        x, y = int(a[3]), int(a[4]); col = tuple(int(a[5][i:i + 2], 16) for i in (0, 2, 4)); d.ellipse([((x - 4) * S, (y - 4) * S), ((x + 4) * S, (y + 4) * S)], fill=col)
im.save(sys.argv[2])
