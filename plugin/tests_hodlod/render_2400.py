"""render_2400.py - draws t_hl2400's layout dump (HL2400_DUMP=<file> ./t2400) to PNGs: the 2.40.0 box exactly as the logic lays it out.
Run: python -I render_2400.py <dump> <out prefix>"""
import sys
from PIL import Image, ImageDraw, ImageFont
R = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf"; B = "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf"
lays = {}; cur = None
for ln in open(sys.argv[1], encoding="utf-8"):
    ln = ln.rstrip("\n")
    if ln.startswith("LAYOUT"):
        f = ln.split(); cur = int(f[1]); lays[cur] = {"box": list(map(int, f[2:6])), "items": []}; continue
    p = ln.split("|", 9); lays[cur]["items"].append(p)
for k, L in lays.items():
    l, t, r, b = L["box"]; W, H = r - l + 40, b - t + 40
    im = Image.new("RGB", (W * 2, H * 2), (11, 15, 25)); d = ImageDraw.Draw(im)
    X = lambda x: (x - l + 20) * 2; Y = lambda y: (y - t + 20) * 2
    col = lambda h: tuple(int(h[i:i + 2], 16) for i in (0, 2, 4))
    for it in L["items"]:
        kk, il, itp, ir, ib = map(int, it[:5]); c1, c2, fs, bold, s = it[5], it[6], int(it[7]), it[8] == "1", it[9]
        if kk == 0: d.rectangle([X(il), Y(itp), X(ir) - 1, Y(ib) - 1], fill=col(c1))
        elif kk == 1: d.rectangle([X(il), Y(itp), X(ir) - 1, Y(ib) - 1], fill=col(c2), outline=col(c1), width=2)
        elif kk == 2: d.line([X(il), Y(itp), X(ir), Y(ib)], fill=col(c1), width=2)
        elif kk == 3 and s:
            fnt = ImageFont.truetype(B if bold else R, int(fs * 2 * 96 / 72)); bb = d.textbbox((0, 0), s, font=fnt)
            d.text((X(il), (Y(itp) + Y(ib)) / 2 - (bb[1] + bb[3]) / 2), s, fill=col(c1), font=fnt)
    im.save(f"{sys.argv[2]}-{k}.png")
