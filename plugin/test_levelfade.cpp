#include "LevelFade.h"
#include "Top10Stable.h"
#include <cstdio>
static int F=0,P=0; static void check(bool c,const char*m){ if(c)++P; else {++F; std::printf("FAIL %s\n",m);} }
int main(){
  using namespace lfd; Tracker t; long long k=1000000000LL+31000;
  auto r=t.update(0,{{k,500,0}}); check(!r[0].quoting,"fresh level not quoting");
  r=t.update(1000,{{k,500,2}}); check(r[0].quoting,"2 re-quotes -> quoting ~");
  // the order is cancelled and the SAME size re-added at the same price under a new id (LP state 0)
  r=t.update(2000,{}); r=t.update(4000,{{k,505,0}}); check(r[0].quoting && r[0].ageMs==4000,"same-size re-add = same level: age and ~ kept, not a move");
  r=t.update(4000+300001,{{k,505,0}}); check(!r[0].quoting,"~ ends 5 min after the last move");
  // gone and back with a different size = a new level
  t.update(310000,{}); r=t.update(320000,{{k,900,0}}); check(r[0].ageMs==0,"different size back = new level");
  // a single move dims (moving order kept size; quoteMoves 1)
  r=t.update(321000,{{k+4,900,1}}); check(r[0].quoting,"1x moved -> ~");
  check(forMarket("ESZ6").quoteMoves==1 && forMarket("6EZ6").windowMs==300000,"params table per market");
  // never more than 10 rows from the board, king = rank 1 = biggest even when moving
  t10::Params pp; pp.persistMs=0; t10::Board b(pp); std::vector<t10::In> in; for(int i=0;i<25;++i) in.push_back(t10::In{1000000000LL+i,(double)i,(long long)(100+i*10),true,true});
  auto o=b.update(0,in); check(o.size()==10 && o[0].lots==340,"10 rows max, rank 1 = biggest");
  std::printf("RESULT %d passed, %d failed\n",P,F); return F?1:0; }
