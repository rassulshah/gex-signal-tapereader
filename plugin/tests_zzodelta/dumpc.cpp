#include "../ZzoComboView.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
// usage: dumpc csv mkt tick sessDay height out   (RTH of that session; the forming bar = the last RTH bar re-fed as forming)
int main(int argc,char**argv){ (void)argc; zzd::Engine E; E.setMarket(argv[2],180); double tick=atof(argv[3]); long long day=atoll(argv[4]); int H=atoi(argv[5]);
 std::ifstream f(argv[1]); std::string ln; std::vector<zzd::Bar> all;
 while(std::getline(f,ln)){ if(ln.empty()||ln[0]=='#')continue; std::stringstream s(ln); std::string a; std::vector<std::string> x; while(std::getline(s,a,','))x.push_back(a);
  zzd::Bar b; b.t=atoll(x[0].c_str()); zzd::toTicks(atof(x[1].c_str()),tick,b.h); zzd::toTicks(atof(x[2].c_str()),tick,b.l); b.vol=atoll(x[3].c_str()); b.dlt=atoll(x[4].c_str()); b.vap=true; b.xh=atoll(x[5].c_str()); b.xl=atoll(x[6].c_str()); b.xok=x[7]=="1"; all.push_back(b); }
 // feed up to the last RTH bar of that session; keep the last one as the forming bar
 int lastR=-1; for(size_t i=0;i<all.size();++i){ if(E.segOf(all[i].t)==day*4+2) lastR=(int)i; }
 for(int i=0;i<lastR;++i) E.addBar(all[(size_t)i]);
 zzd::Bar form=all[(size_t)lastR]; form.seg=E.segOf(form.t);
 int first=-1; for(size_t i=0;i<E.B.size();++i) if(E.B[i].seg==day*4+2){ first=(int)i; break; }
 const int last=(int)E.B.size(); // forming bar index
 const int bw=9; zzd::View v; v.l=0;v.t=0;v.r=(last-first+1)*bw+60;v.b=H;v.first=first;v.last=last;v.bw=bw; v.textH=11;
 v.X=[&](int j){return (j-first)*bw+bw/2.0+4;}; v.textW=[](const std::string&s){return (int)s.size()*8;};
 auto w=E.view(&form); auto sc=zzd::layoutCombo(E,w,form.vol,v,std::string("ZZO Combo ")+zzd::COMBO_VERSION);
 std::ofstream o(argv[6]); o<<v.r<<"\n"; for(int i=first;i<last;++i) o<<"C,"<<v.X(i)<<","<<E.B[i].h*tick<<","<<E.B[i].l*tick<<","<<E.B[i].t<<"\n";
 o<<"C,"<<v.X(last)<<","<<form.h*tick<<","<<form.l*tick<<","<<form.t<<"\n";
 for(auto&z:zzd::zigzag(E,&form)) if(z.i>=first) o<<"Z,"<<v.X(z.i)<<","<<z.px*tick<<"\n";
 long long lv[8]; bool ok[8]; E.levels(day,lv,ok); for(int i=0;i<8;++i) if(ok[i]) o<<"L,"<<zzd::LV_NAMES[i]<<","<<lv[i]*tick<<"\n";
 for(auto&p:sc.p) o<<(p.k==zzd::Prim::FILL?"F":"T")<<","<<p.x0<<","<<p.y0<<","<<p.x1<<","<<p.y1<<","<<p.col<<","<<p.text<<"\n"; }
