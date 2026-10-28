import sys
from PIL import Image, ImageDraw, ImageFont
F=lambda s,b=False: ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans%s.ttf'%('-Bold' if b else ''), s)
lines=open(sys.argv[1]).read().splitlines(); im=Image.new('RGB',(1700,760),(11,15,23)); d=ImageDraw.Draw(im)
col=lambda c:((c>>16)&255,(c>>8)&255,c&255)
P=[l.split(' ',10) for l in lines if l.startswith('P')]; C=[list(map(float,l.split()[1:])) for l in lines if l.startswith('C')]
def prim(q):
    k,x0,y0,x1,y1,c,w=map(int,q[1:8]); txt=q[10] if len(q)>10 else ''
    if k==0: d.rectangle([min(x0,x1),min(y0,y1),max(x0,x1),max(y0,y1)],fill=col(c))
    elif k==1: d.rectangle([x0,y0,x1,y1],outline=col(c),width=w)
    elif k==2: d.line([(x0,y0),(x1,y1)],fill=col(c),width=w)
    elif k==3: d.ellipse([x0,y0,x1,y1],outline=col(c),width=w)
    else: d.text((x0,y0),txt,fill=col(c),font=F(11,q[9]=='1'))
for q in P:
    if q[8]=='1': prim(q)
for x,o,h,l,c in C:
    cc=(34,197,94) if c<=o else (239,68,68); d.line([(x,h),(x,l)],fill=cc); d.rectangle([x-5,min(o,c),x+5,max(max(o,c),min(o,c)+1)],fill=cc)
for q in P:
    if q[8]=='0': prim(q)
im.save(sys.argv[2])
