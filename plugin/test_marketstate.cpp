#define HB_FAKE 1
// test_marketstate.cpp -- MarketState.h 1.0.0: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. test_marketstate.cpp && ./a.out
#include "MarketState.h"
#include <cassert>
#include <iostream>
int main(){
  std::map<std::string,std::string> kv={{"spike_side","sell"},{"ratio","3.1"},{"bad;key","a=b|c"}};
  mst::publish("AGG","ESZ6",180,"chart1",1000,555,kv);
  auto F=mst::read("AGG","ESZ6",180,"chart1",2000); assert(F.ok && F.bar==555 && mst::getD(F,"ratio")==3.1 && mst::getS(F,"spike_side")=="sell");
  assert(F.kv.count("bad key") && F.kv["bad key"]=="a b c");
  assert(!mst::read("AGG","ESZ6",180,"chart1",1000+mst::FRESH_MS+1).ok);
  assert(!mst::read("AGG","ESZ6",180,"chart2",2000).ok);
  // long value > 511 chars round-trips (the old bbGet cap)
  std::map<std::string,std::string> big; for(int i=0;i<60;i++) big["k"+std::to_string(i)]="value_"+std::to_string(i)+"_xxxxxxxxxx";
  mst::publish("LOTS","ESZ6",180,"chart1",1500,556,big);
  auto B=mst::read("LOTS","ESZ6",180,"chart1",2000); assert(B.ok && B.kv.size()==60);
  auto all=mst::readAll("ESZ6",180,"chart1",2000); assert(all.size()==2);
  std::string j=mst::snapshotJson("ES","ESZ6",180,"2026-10-18 09:12:00",2000,all); assert(j.find("\"AGG\":{\"age_ms\":1000")!=std::string::npos);
  mst::clear("AGG","ESZ6",180,"chart1"); assert(!mst::read("AGG","ESZ6",180,"chart1",2000).ok);
  std::cout<<"ok "<<j.size()<<"\n";
}
