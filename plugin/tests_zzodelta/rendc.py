import sys,datetime
from PIL import Image, ImageDraw, ImageFont
L=open(sys.argv[1]).read().split('\n'); W=int(L[0]); C=[];P=[];Z=[];LV=[]
for ln in L[1:]:
    if not ln: continue
    x=ln.split(',',7)
    if x[0]=='C': C.append([float(v) for v in x[1:5]])
    elif x[0]=='Z': Z.append((float(x[1]),float(x[2])))
    elif x[0]=='L': LV.append((x[1],float(x[2])))
    else: P.append(x)
H1=360; H2=int(sys.argv[4]) if len(sys.argv)>4 else 270; img=Image.new('RGB',(W+70,H1+H2+30),'black'); d=ImageDraw.Draw(img)
f=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',9)
hi=max(c[1] for c in C); lo=min(c[2] for c in C); pad=(hi-lo)*0.08; hi+=pad; lo-=pad; Y=lambda v:10+(hi-v)/(hi-lo)*(H1-20)
for nm,v in LV:
    if lo<v<hi: d.line([(0,Y(v)),(W,Y(v))],fill=(250,204,21),width=1); d.text((W+2,Y(v)-6),nm,fill=(250,204,21),font=f)
for x,h,l,t in C: d.line([(x,Y(h)),(x,Y(l))],fill=(148,163,184),width=3)
for k in range(1,len(Z)): d.line([(Z[k-1][0],Y(Z[k-1][1])),(Z[k][0],Y(Z[k][1]))],fill=(229,231,235),width=2)
for i in range(0,len(C),20):
    t=datetime.datetime.utcfromtimestamp(int(C[i][3])-180).strftime('%H:%M'); d.text((C[i][0]-12,H1-12),t,fill=(148,163,184),font=f)
d.text((6,4),sys.argv[3],fill=(229,231,235),font=f)
oy=H1+20; d.rectangle([0,oy,W,oy+H2],fill=(11,15,25))
for k,x0,y0,x1,y1,col,txt in P:
    x0,y0,x1,y1,col=int(x0),int(y0),int(x1),int(y1),int(col); rgb=((col>>16)&255,(col>>8)&255,col&255)
    if k=='F': d.rectangle([x0,oy+y0,x1,oy+y1],fill=rgb)
    else: d.text((x0,oy+y0),txt,fill=rgb,font=f)
img=img.resize((img.width*2,img.height*2),Image.LANCZOS); img.save(sys.argv[2])
