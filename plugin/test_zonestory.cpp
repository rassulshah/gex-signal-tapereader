#define HB_FAKE 1
#include <algorithm>
// test_zonestory.cpp -- ZoneStory.h 1.0.0: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. test_zonestory.cpp && ./a.out
#include "ZoneStory.h"
#include <cassert>
#include <iostream>
using namespace zst;
static std::map<std::string,mst::Facts> F(std::map<std::string,std::map<std::string,std::string>> m){ std::map<std::string,mst::Facts> o; for(auto&p:m){mst::Facts f; f.ok=true; f.src=p.first; f.kv=p.second; o[p.first]=f;} return o;}
int main(){
  Zone z{"PDL 61.42",61.40,61.44,1};
  Story s; s.z=z; long long t=100;
  auto bar=[&](double l,double h,double c){ Bar b; b.t=(t+=180); b.l=l; b.h=h; b.c=c; b.o=c; return b; };
  // arrive
  Bar b=bar(61.41,61.50,61.45); step(s,b,observe(z,b,F({}))); assert(s.step==ARRIVED);
  // sellers attack 3.1x into the zone, close inside -> absorbing
  b=bar(61.39,61.46,61.42); step(s,b,observe(z,b,F({{"AGG",{{"spike","sell"},{"sell_x","3.1"}}}}))); assert(s.step==ABSORBING && pathSoFar(s)=="S0");
  // retest attack 1.2x? (< 2x is not a spike) -> counts as no attack; 3 bars -> fading
  for(int i=0;i<3;i++){ b=bar(61.41,61.47,61.44); step(s,b,observe(z,b,F({}))); }
  assert(s.step==FADING && pathSoFar(s)=="S1");
  // same bar again ignored
  step(s,b,observe(z,b,F({{"AGG",{{"spike","buy"},{"buy_x","2.5"}}}}))); assert(s.step==FADING);
  // buyers counter 2.5x, close above the zone -> held S1
  b=bar(61.43,61.55,61.53); step(s,b,observe(z,b,F({{"AGG",{{"spike","buy"},{"buy_x","2.5"}}}}))); assert(s.step==COUNTER && s.path=="S1" && s.done);
  // failure path: defenders pulling then break
  Story f; f.z=z; t=100;
  b=bar(61.41,61.50,61.45); step(f,b,observe(z,b,F({})));
  b=bar(61.39,61.46,61.42); step(f,b,observe(z,b,F({{"AGG",{{"spike","sell"},{"sell_x","2.4"}}}})));
  b=bar(61.39,61.45,61.41); step(f,b,observe(z,b,F({{"BOOK",{{"bid_state","leaving"}}}}))); assert(f.step==DEF_PULLING && pathSoFar(f)=="F");
  b=bar(61.30,61.42,61.33); step(f,b,observe(z,b,F({}))); assert(f.step==BREAK && f.path=="F" && f.done);
  // final DP A at the zone starts absorbing; tentative does not
  Story d; d.z=z; t=100; b=bar(61.41,61.50,61.45); step(d,b,observe(z,b,F({})));
  b=bar(61.40,61.46,61.43); step(d,b,observe(z,b,F({{"DP",{{"a","tentative"},{"a_side","bull"},{"a_px","61.41"}}}}))); assert(d.step==ARRIVED);
  b=bar(61.40,61.46,61.43); step(d,b,observe(z,b,F({{"DP",{{"a","final"},{"a_side","bull"},{"a_px","61.41"}}}}))); assert(d.step==ABSORBING);
  std::cout<<"ok "<<words(s)<<" | "<<words(f)<<"\n";
}
