#include "ReadBox.h"
#include <cstdio>
int main(){ using namespace rbx; int f=0; auto ck=[&](bool c,const char*m){ if(!c){ ++f; printf("FAIL %s\n",m);} };
 BookIn k; k.ok=true; k.b=2.3; k.s=0.3; k.bits=1;
 Line a=bookLine(k,1,"",0,true); ck(a.text=="Book: Pullback in uptrend: Bids defending 2.3x (2.0x)"&&a.col==GREEN,"up def"); puts(a.text.c_str());
 k.b=0.7; k.bits=0; a=bookLine(k,1,"",0,true); ck(a.text=="Book: Pullback in uptrend: Bids not defending (B 0.7x, S 0.3x)"&&a.col==WHITE,"up not"); puts(a.text.c_str());
 k.b=-1.8; k.bits=2; a=bookLine(k,1,"",0,false); ck(a.text=="Book: Uptrend: Bids pulling 1.8x"&&a.col==RED,"up pull"); puts(a.text.c_str());
 k.b=0.4; k.s=2.1; k.bits=4; a=bookLine(k,-1,"",0,true); ck(a.text=="Book: Pullback in downtrend: Offers defending 2.1x (2.0x)"&&a.col==RED,"dn def"); puts(a.text.c_str());
 k.b=2.6; k.s=0.2; k.bits=1; a=bookLine(k,-1,"LonLO",1,false); ck(a.text=="Book: Reversal at LonLO: Bids defending 2.6x (2.0x)"&&a.col==GREEN,"rev"); puts(a.text.c_str());
 a=bookLine(k,-1,"PDH",-1,false); ck(a.text.find("Downtrend: Offers not defending")!=std::string::npos,"level with trend = trend side"); puts(a.text.c_str());
 a=bookLine(k,0,"",0,false); ck(a.text=="Book: Bids defending 2.6x (2.0x)","na"); puts(a.text.c_str());
 k.bits=0; a=bookLine(k,0,"",0,false); ck(a.text=="Book: Even (B 2.6x, S 0.2x)","even");
 k.s=-2.0; k.bits=8; a=bookLine(k,-1,"",0,false); ck(a.col==GREEN&&a.text=="Book: Downtrend: Offers pulling 2.0x","offers pull"); 
 k.ok=false; a=bookLine(k,1,"",0,false); ck(a.text.empty(),"none");
 Out o; o.ok=true; o.n=1; o.l[0].text="Flow: Sellers in control"; o.l[0].col=RED; o.bkTrend=1; o.bkBase=1; BookIn q; q.ok=true; q.b=2.3; q.bits=1;
 applyBook(o,q); ck(o.n==2&&o.l[1].col==WHITE,"disagree white"); o.l[0].col=GREEN; applyBook(o,q); ck(o.l[1].col==GREEN,"agree"); q.ok=false; applyBook(o,q); ck(o.n==1,"gone");
 printf("%s\n",f?"FAILED":"bookline 13 ok"); return f; }
