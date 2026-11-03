# render_check.py -- draws render_dump.cpp's output (candles + the plugin's letters as the mock recorded them) to render-bo111.png
import sys
from PIL import Image, ImageDraw, ImageFont
L = [l.split() for l in open(sys.argv[1])]
P = [l for l in L if l[0] == 'PANE'][0]; W, H = int(P[3]) + 20, int(P[4]) + 10
im = Image.new('RGB', (W, H), (0, 0, 0)); d = ImageDraw.Draw(im)
for l in L:
    if l[0] == 'BAR':
        x, o, h, lo, c, m = map(int, l[1:]); col = (34, 197, 94) if c <= o else (239, 68, 68)
        d.line([(x, h), (x, lo)], fill=col); d.rectangle([x - 3, min(o, c), x + 3, max(o, c)], fill=col)
        if m % 30 == 0: d.text((x - 12, H - 22), '%d:%02d' % (m // 60, m % 60), fill=(148, 163, 184))
for l in L:
    if l[0] == 'TXT':
        x0, y0, x1, y1 = map(int, l[1:5]); col = int(l[5], 16); size = int(l[6]); s = l[7]
        f = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', int(size * 1.33))
        rgb = ((col >> 16) & 255, (col >> 8) & 255, col & 255)
        tw = d.textlength(s, font=f); d.text(((x0 + x1) / 2 - tw / 2, y0), s, fill=rgb, font=f)
im.save('render-bo111.png'); print('render-bo111.png', W, H)
