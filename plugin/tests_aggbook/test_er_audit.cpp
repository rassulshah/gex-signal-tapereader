#include "EffortReward.h"
#include <cstdio>
#include <limits>
static int passes=0, fails=0;
#define CHECK(x) do { if(x) ++passes; else { ++fails; std::printf("FAIL line %d: %s\n",__LINE__,#x); } } while(0)
int main() {
    double x=0;
    const double inf=std::numeric_limits<double>::infinity();
    const double nan=std::numeric_limits<double>::quiet_NaN();
    CHECK(er::rewardX(100,102,99,100,.25,100,0,"ES",x) && x>0);
    CHECK(er::rewardX(100,102,99,100,.25,100,1,"ES",x) && x>0);
    CHECK(!er::rewardX(100,102,99,100,inf,100,0,"ES",x));
    CHECK(!er::rewardX(100,102,99,100,nan,100,0,"ES",x));
    CHECK(!er::rewardX(100,102,99,100,0,100,0,"ES",x));
    CHECK(!er::rewardX(100,102,99,100,.25,100,2,"ES",x));
    CHECK(!er::rewardX(100,99,101,100,.25,100,0,"ES",x));
    CHECK(!er::rewardX(105,102,99,100,.25,100,0,"ES",x));
    CHECK(!er::rewardX(95,102,99,100,.25,100,1,"ES",x));
    CHECK(!er::rewardX(nan,102,99,100,.25,100,0,"ES",x));
    CHECK(!er::rewardX(100,102,99,100,.25,0,0,"ES",x));
    CHECK(!er::rewardX(100,102,99,100,.25,100,0,"UNKNOWN",x));
    CHECK(er::rewardX(100,102,99,nan,.25,100,0,"ES",x)); // existing prior-close fallback
    CHECK(er::effortX(20,std::vector<long long>(20,10),x) && x==2.0);
    CHECK(!er::effortX(20,std::vector<long long>(4,10),x));
    CHECK(!er::effortX(20,std::vector<long long>(20,0),x));
    CHECK(!er::effortX(-1,std::vector<long long>(20,10),x));
    er::Side s; s.effOk=s.rewOk=true;s.eff=nan;s.rew=.1;er::guard(s);
    CHECK(!s.effOk && !s.rewOk);
    s=er::Side();s.effOk=s.rewOk=true;s.eff=2;s.rew=inf;er::guard(s);CHECK(!s.rewOk);
    s=er::Side();s.effOk=s.rewOk=true;s.eff=2;s.rew=-1;er::guard(s);CHECK(!s.rewOk);
    s=er::Side();s.effOk=s.rewOk=true;s.eff=.49;s.rew=.1;er::guard(s);CHECK(!s.rewOk && s.thin && er::rewardCellOf(s)=="-");
    s=er::Side();s.effOk=s.rewOk=true;s.eff=.5;s.rew=.1;er::guard(s);CHECK(s.rewOk);
    er::Side sd[2];sd[0].effOk=sd[0].rewOk=true;sd[0].eff=2;sd[0].rew=.5;sd[0].lost=true;
    br::Read r;CHECK(er::read(true,false,sd,r) && r.impl==br::BULL);
    sd[0].lost=false;CHECK(!er::read(true,false,sd,r));
    sd[0].lost=true;CHECK(!er::read(true,true,sd,r));
    CHECK(er::rewardCell(true,.5,true).find(" Abs")!=std::string::npos);
    CHECK(er::rewardCell(true,.5,false).find(" Abs")==std::string::npos);
    CHECK(er::rewardCell(false,.5,true)=="n/a");
    std::printf("EffortReward audit: %d passed, %d failed\n",passes,fails);
    return fails?1:0;
}
