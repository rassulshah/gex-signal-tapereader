"""Render the plugin's own draw list (the mock SDK's record of every rectangle and text Turn.cpp drew) to a PNG: the render check."""
import sys
from PIL import Image, ImageDraw, ImageFont
src, out = sys.argv[1], sys.argv[2]; W, H, Z = 1400, 300, 2
fr = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"; fb = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"
img = Image.new("RGB", (W * Z, H * Z), "#000000"); d = ImageDraw.Draw(img)
def col(h): h = h.rjust(6, "0"); return "#" + h
for ln in open(src, "rb").read().decode("cp1252").splitlines():
    c = ln.split("|", 10)
    if len(c) < 11: continue
    typ, l, t, r, b, color, border, fsize, fstyle, centred, text = c; l, t, r, b = int(l), int(t), int(r), int(b)
    if typ == "box": d.rectangle([l * Z, t * Z, r * Z - 1, b * Z - 1], fill=col(color))
    elif typ == "text":
        f = ImageFont.truetype(fb if fstyle == "1" else fr, int(8.6 * Z))
        w = d.textlength(text, font=f); x = (l + r) / 2 * Z - w / 2 if centred == "1" else l * Z
        d.text((x, t * Z + 1 * Z), text, font=f, fill=col(color))
img.save(out); print("ok", out)
