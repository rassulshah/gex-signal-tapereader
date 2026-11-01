# render_check.py - draws render_dump's scene (candles + the plugin's primitives) to a PNG: python3 render_check.py scene.txt out.png
import sys
from PIL import Image, ImageDraw, ImageFont
L=[l.split(' ',8) for l in open(sys.argv[1]).read().splitlines()]
W,H=1500,820; im=Image.new('RGB',(W,H),(11,15,23)); d=ImageDraw.Draw(im)
try: F=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',12)
except Exception: F=ImageFont.load_default()
for x in L:
    if x[0]!='C': continue
    xc,o,h,l,c,bw=map(float,x[1:7]); col=(34,197,94) if c<o else (239,68,68)   # y down: close above open = smaller y
    d.line([(xc,h),(xc,l)],fill=col); d.rectangle([xc-bw/2,min(o,c),xc+bw/2,max(o,c)+0.5],fill=col)
    if x[7].endswith(':00') or x[7].endswith(':30'): d.text((xc-14,H-16),x[7],fill=(120,130,150),font=F)
for x in L:
    if x[0]!='P': continue
    k,x0,y0,x1,y1,col=map(int,x[1:7]); rgb=((col>>16)&255,(col>>8)&255,col&255)
    if k==0: d.rectangle([x0,y0,x1,y1],fill=rgb)
    else: d.text((x0,y0),x[7] if len(x)>7 else '',fill=rgb,font=F)
im.save(sys.argv[2])
