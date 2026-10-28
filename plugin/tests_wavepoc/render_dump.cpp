// render_dump.cpp - the scene WavePOCLogic.h draws for the Oct 22 ES fixture, as text, for the render check (render_check.py)
#include "../WavePOCLogic.h"
#include <fstream>
#include <iostream>
#include <map>
int main(int argc,char**argv){ std::ifstream f(argc>1?argv[1]:"wavepoc_es1022.csv"); int upto=argc>2?atoi(argv[2]):78; std::string line; std::vector<wpc::Bar> B; std::vector<std::vector<wpc::VRow>> V;
 while(std::getline(f,line)){ if(line.empty()||line[0]=='#')continue; auto x=wpc::splitCsv(line);
  if(x[0]=="B"){wpc::Bar b; b.t=atoll(x[2].c_str()); wpc::toTicks(atof(x[3].c_str()),.25,b.o);wpc::toTicks(atof(x[4].c_str()),.25,b.h);wpc::toTicks(atof(x[5].c_str()),.25,b.l);wpc::toTicks(atof(x[6].c_str()),.25,b.c);B.push_back(b);V.resize(B.size());}
  else {wpc::VRow r; wpc::toTicks(atof(x[2].c_str()),.25,r.tick); r.vol=atoll(x[3].c_str()); V[atoi(x[1].c_str())].push_back(r);} }
 wpc::Store S; for(int k=0;k<20;++k){S.add(-1,B[0].t-100000+k*1000,k%2?94:95);S.add(1,B[0].t-100000+k*1000+500,86);}
 std::map<std::pair<int,int>,std::vector<std::pair<long long,std::pair<double,double>>>> LQ;
 if(argc>4){ std::ifstream g(argv[4]); while(std::getline(g,line)){ if(line.empty()||line[0]=='#')continue; auto x=wpc::splitCsv(line); if(x.size()!=6)continue; long long tk; wpc::toTicks(atof(x[4].c_str()),.25,tk); double v=atof(x[5].c_str());
   LQ[{atoi(x[1].c_str()),atoi(x[2].c_str())}].push_back({tk,{x[3]=="0"?v:0.0,x[3]=="1"?v:0.0}}); } }
 wpc::Engine E; E.store=&S;
 if(argc>4) E.liqFn=[&](const wpc::Wave&w,std::vector<double>&rows,std::vector<double>&bids){ rows.assign(w.rows.size(),0); bids.assign(w.rows.size(),0); auto it=LQ.find({w.i0,w.i1}); if(it==LQ.end())return false;
   for(auto&e:it->second){ long long d=e.first-w.e0; if(d<0)continue; long long k=d/w.rowTicks; if(k>=(long long)rows.size())continue; rows[k]+=e.second.first+e.second.second; bids[k]+=e.second.first;} return true; }; wpc::VapFn vap=[&](int i,std::vector<wpc::VRow>&o){o=V[i];return 1;};
 for(int i=0;i<upto;++i) E.addBar(B[i],vap);
 wpc::Wave p; bool pv=E.provisional(p,vap);
 wpc::View v; v.l=0;v.t=0;v.r=1700;v.b=760; v.first=0; v.last=upto; v.lastBar=upto; v.bw=16;
 v.X=[&](int i){return 20+(i+0.5)*16;}; v.Y=[&](double tk){return 740-(tk*0.25-7674)/(7728-7674)*700;};
 v.textW=[](const std::string&s,bool){return (int)s.size()*7;}; v.formH=B[upto].h; v.formL=B[upto].l; v.haveForming=true; v.zigzag=argc>3 && std::string(argv[3])=="z";
 wpc::Scene sc=wpc::layout(E,pv?&p:nullptr,v);
 for(int i=0;i<=upto;i++) std::cout<<"C "<<v.X(i)<<" "<<v.Y(B[i].o)<<" "<<v.Y(B[i].h)<<" "<<v.Y(B[i].l)<<" "<<v.Y(B[i].c)<<"\n";
 for(auto&q:sc.p) std::cout<<"P "<<(int)q.k<<" "<<q.x0<<" "<<q.y0<<" "<<q.x1<<" "<<q.y1<<" "<<q.col<<" "<<q.w<<" "<<q.back<<" "<<q.bold<<" "<<q.text<<"\n";
}
