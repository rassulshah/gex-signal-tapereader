// t_ec110.cpp -- lsEventCal 1.1.0 (Rassul 2026-11-26 08:05-08:31 CT): the contract line (front month, roll day, OLD month) and the 0DTE line.
// Build: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined t_ec110.cpp -o t && ./t
#include "../EventCalLogic.h"
#include <cstdio>
using namespace ecal;
static int fails=0, n=0;
#define CHECK(c, msg) do{ ++n; if(!(c)){ ++fails; std::printf("FAIL %s\n", msg);} }while(0)
static long long D(int y,int m,int d){ return daysFromCivil(y,(unsigned)m,(unsigned)d); }
static std::string md(long long day){ int y,m,d; civil(day,y,m,d); char b[16]; std::snprintf(b,sizeof b,"%d-%02d-%02d",y,m,d); return b; }
int main(){
  long long today=D(2026,11,26), now=today*1440+8*60+1;
  // civil round trip
  for(long long d=D(2025,1,1); d<D(2029,1,1); ++d){ int y,m,dd; civil(d,y,m,dd); if(D(y,m,dd)!=d){ CHECK(false,"civil"); break; } }
  struct T{const char* mk; const char* front; const char* next; const char* roll;};
  T t[]={{"ES","ESZ6","ESH7","2026-12-10"},{"NQ","NQZ6","NQH7","2026-12-10"},{"EU","6EZ6","6EH7","2026-12-07"},
         {"CL","CLF7","CLG7","2026-12-11"},{"NG","NGF7","NGG7","2026-12-22"},{"GC","GCG7","GCJ7","2027-01-27"},{"HG","HGH7","HGK7","2027-02-24"}};
  for(auto& x:t){ Front F=frontOf(x.mk,today); char b[200];
    std::snprintf(b,sizeof b,"%s front %s next %s roll %s (want %s %s %s)",x.mk,F.cur.code(x.mk).c_str(),F.next.code(x.mk).c_str(),md(F.cur.roll).c_str(),x.front,x.next,x.roll);
    std::printf("%s ltd %s\n",b,md(F.cur.ltd).c_str());
    CHECK(F.ok && F.cur.code(x.mk)==x.front && F.next.code(x.mk)==x.next && md(F.cur.roll)==x.roll, b); }
  // known LTDs
  CHECK(md(contractOf("ES",2026,12).ltd)=="2026-12-18","ES ltd");
  CHECK(md(contractOf("EU",2026,12).ltd)=="2026-12-14","6E ltd");
  CHECK(md(contractOf("CL",2027,1).ltd)=="2026-12-21","CL F7 ltd");
  CHECK(md(contractOf("NG",2027,1).ltd)=="2026-12-29","NG F7 ltd");
  CHECK(md(contractOf("NG",2026,12).ltd)=="2026-11-25","NG Z6 ltd");
  CHECK(md(contractOf("GC",2026,12).ltd)=="2026-11-30","GC Z6 fnd");
  // chart lines
  ContractLine c=contractLine("GC","GCZ6",now); std::printf("GC: [%s] [%s] [%s] %d\n",c.label.c_str(),c.text.c_str(),c.count.c_str(),c.state);
  CHECK(c.state==CS_OLD && c.label=="GCZ6 OLD" && c.text=="trade GCG7 now","GC old");
  c=contractLine("NG","NGZ6",now); CHECK(c.state==CS_OLD,"NG old");
  c=contractLine("ES","ESZ6",now); std::printf("ES: [%s] [%s] [%s] %d\n",c.label.c_str(),c.text.c_str(),c.count.c_str(),c.state);
  CHECK(c.state==CS_OK && c.text=="roll to ESH7 Thu 12/10" && c.count=="14d","ES ok");
  c=contractLine("ES","ESH7",now); CHECK(c.state==CS_AHEAD,"ES ahead");
  c=contractLine("EU","6EZ6",now); CHECK(c.state==CS_OK && c.label=="6EZ6","6E ok");
  c=contractLine("ES","ESZ6",D(2026,12,7)*1440+600); std::printf("ES 12/7: [%s] %d\n",c.text.c_str(),c.state); CHECK(c.state==CS_SOON,"ES soon");
  c=contractLine("ES","ESZ6",D(2026,12,10)*1440+600); CHECK(c.state==CS_OLD,"ES old on roll day");
  c=contractLine("ES","",now); CHECK(c.state==CS_OK && c.label=="ESZ6","ES no symbol");
  c=contractLine("XX","XXZ6",now); CHECK(c.state==CS_NONE,"unknown");
  // expiries
  ExpiryLine x=expiryLine("ES",now); std::printf("ES 0DTE today: [%s] %d\n",x.text.c_str(),x.state);
  CHECK(x.state==XS_NEXT && x.text.find("none today - next Fri 12:00 PM weekly")==0,"ES today");
  x=expiryLine("ES",D(2026,12,1)*1440+7*60+48); std::printf("ES 12/1: [%s] [%s] %d\n",x.text.c_str(),x.count.c_str(),x.state);
  CHECK(x.state==XS_LATER && x.text=="Tue 3:00 PM daily" && x.count=="7h 12m","ES 12/1");
  x=expiryLine("ES",D(2026,11,30)*1440+14*60+10); CHECK(x.state==XS_SOON && x.big && x.text=="Mon 3:00 PM MONTH-END","ES month-end amber");
  x=expiryLine("ES",D(2026,12,18)*1440+7*60); std::printf("ES 12/18: [%s]\n",x.text.c_str()); CHECK(x.text=="Fri 8:30 AM QUARTERLY" && x.big,"ES quarterly");
  x=expiryLine("ES",D(2026,12,1)*1440+15*60+12); CHECK(x.state==XS_OUT && x.count=="expired 12m","ES expired");
  x=expiryLine("ES",D(2026,12,1)*1440+15*60+40); std::printf("ES after: [%s]\n",x.text.c_str()); CHECK(x.state==XS_NEXT && x.text.find("none left today - next Wed 3:00 PM daily")==0,"ES none left");
  x=expiryLine("ES",D(2026,12,1)*1440+17*60+5); CHECK(x.state==XS_LATER && x.text=="Wed 3:00 PM daily","ES next session");
  x=expiryLine("GC",D(2026,12,1)*1440+7*60+33); std::printf("GC 12/1: [%s] [%s]\n",x.text.c_str(),x.count.c_str()); CHECK(x.text=="Tue 12:30 PM daily","GC");
  x=expiryLine("HG",D(2026,12,4)*1440+7*60); CHECK(x.text=="Fri 12:00 PM weekly","HG weekly");
  x=expiryLine("EU",D(2026,12,4)*1440+7*60); std::printf("6E 12/4: [%s]\n",x.text.c_str()); CHECK(x.text=="Fri 9:00 AM QUARTERLY","6E quarterly");
  x=expiryLine("CL",D(2026,12,16)*1440+7*60); std::printf("CL 12/16: [%s]\n",x.text.c_str()); CHECK(x.text=="Wed 1:30 PM MONTHLY","CL monthly");
  x=expiryLine("NG",D(2026,12,28)*1440+7*60); std::printf("NG 12/28: [%s]\n",x.text.c_str()); CHECK(x.text=="Mon 1:30 PM MONTHLY","NG monthly");
  x=expiryLine("GC",D(2026,12,24)*1440+7*60); std::printf("GC 12/24: [%s]\n",x.text.c_str());
  x=expiryLine("ES",D(2026,11,27)*1440+7*60); std::printf("ES 11/27: [%s]\n",x.text.c_str()); CHECK(x.text=="Fri 12:00 PM weekly (early close)","ES early");
  // layout
  auto tw=[](const std::string& s,bool){ return (int)s.size()*6; };
  std::vector<Row> R; Row r; r.when="Mon 9:00 AM"; r.name="ISM Mfg"; r.count="1h 12m"; R.push_back(r);
  Layout L=layout(R,tw,contractLine("ES","ESZ6",now),expiryLine("ES",now));
  CHECK(L.rules.size()==1 && L.cells.size()==3+2+3 && L.h==2+3*ROW_H+5+2,"layout");
  Layout L2=layout({},tw,contractLine("ES","ESZ6",now),expiryLine("ES",now)); CHECK(L2.rules.empty() && L2.h==2+2*ROW_H+2,"layout no events");
  Layout L3=layout({},tw); CHECK(L3.w==0,"empty");
  for(auto& cc:L.cells) CHECK(cc.x+cc.w<=L.w,"inside");
  std::string st=statusText("ES","x",R,"top left",contractLine("ES","ESZ6",now),expiryLine("ES",now),"ESZ6"); std::printf("%s",st.c_str());
  // every day 2026-2028: frontOf ok for all markets, roll < ltd, expiries only on business days
  const char* M[]={"ES","NQ","CL","GC","HG","NG","EU"};
  for(long long d=D(2026,1,1); d<D(2028,10,1); ++d) for(auto mk:M){ Front F=frontOf(mk,d); if(!F.ok || F.cur.roll>=F.cur.ltd+1 || d>=F.cur.roll){ CHECK(false,"front sweep"); std::printf("%s %s\n",mk,md(d).c_str()); goto done; } }
  done:
  std::printf("%d checks, %d failed\n",n,fails); return fails?1:0;
}
