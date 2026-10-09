#include "../TapeFlowSupport.h"
#include <cassert>
#include <iostream>
#include <vector>

using namespace tf_support;
static TradeIdentity rec(long long t, int px=100, long long q=1) {
    return TradeIdentity{t,static_cast<double>(t),px,99,101,q};
}
struct Record { long long t,b,s,u; float a; unsigned flags; };
int main() {
    SnapshotCursor cursor;
    std::size_t start=999;
    std::string error;
    std::vector<TradeIdentity> data{rec(10),rec(10),rec(11)};
    assert(cursor.plan(data,&start,&error) && start==0);
    for (const auto& r:data) cursor.commit(r);
    assert(cursor.lastSec==11 && cursor.consumed.size()==1);
    assert(cursor.plan(data,&start,&error) && start==3);
    data.push_back(rec(11)); // identical print, additional legitimate multiplicity
    assert(cursor.plan(data,&start,&error) && start==3);
    cursor.commit(data[start]);
    assert(cursor.plan(data,&start,&error) && start==4);
    std::vector<TradeIdentity> changed{rec(11,102)};
    assert(!cursor.plan(changed,&start,&error));
    changed={rec(11)}; // shrinking the already-consumed prefix must not silently skip
    assert(!cursor.plan(changed,&start,&error));
    changed={rec(12)}; // missing overlap, cannot prove this request was complete
    assert(!cursor.plan(changed,&start,&error));
    changed={rec(11),rec(11),rec(12)};
    assert(cursor.plan(changed,&start,&error) && start==2);
    changed={rec(12),rec(11)};
    assert(!cursor.plan(changed,&start,&error));
    changed={rec(11),rec(11),rec(12,100,0)};
    assert(!cursor.plan(changed,&start,&error));
    cursor.reset(); assert(cursor.lastSec==-1 && cursor.consumed.empty());
    int ticks=0;
    assert(toTicks(6012.25,.25,&ticks) && ticks==24049);
    assert(toTicks(-.01,.01,&ticks) && ticks==-1);
    assert(toTicks(.1+.2,.1,&ticks) && ticks==3);
    assert(!toTicks(1.13,.25,&ticks));
    assert(!toTicks(std::numeric_limits<double>::infinity(),.25,&ticks));
    assert(!toTicks(1,0,&ticks));
    assert(nativeToTicks(70.01f,.01,&ticks) && ticks==7001);
    assert(nativeToTicks(3.001f,.001,&ticks) && ticks==3001);
    assert(nativeToTicks(4.1005f,.0005,&ticks) && ticks==8201);
    assert(nativeToTicks(1.10005f,.00005,&ticks) && ticks==22001);
    assert(!nativeToTicks(1.13f,.25,&ticks));
    assert(eventKey("2026-10-08 09:30:00|12|AW|1|zone|value\n")=="2026-10-08 09:30:00|12|AW|1");
    assert(eventKey("broken")=="broken");
    const std::string journal="2026-10-08 09:30:00|12|AW|1|100|102|1|20|50|10|1|1|20|.5|0|aligned|watch|1|1.1.0|quote-at-trade\n";
    assert(completeEventLine(journal));
    assert(!completeEventLine(journal.substr(0,journal.size()-1)));
    assert(!completeEventLine(journal.substr(0,40)+"\n"));
    assert(safeField("a|b\nc\rd")=="a b c d");
    assert(fingerprint("abc")==fingerprint("abc"));
    assert(fingerprint("abc")!=fingerprint("abd"));
    std::vector<Record> records{{10,10,2,3,1,1},{11,20,4,0,2,1},{12,5,9,0,NAN,0}};
    RenderIndex index; index.build(records,1);
    const auto a=index.end(10),b=index.end(12);
    assert(b-a==2 && index.buys[b]-index.buys[a]==25);
    assert(index.sells[b]-index.sells[a]==13);
    assert(index.quality[b]-index.quality[a]==1);
    assert(index.activityCount[b]-index.activityCount[a]==1);
    assert(index.activity[b]-index.activity[a]==2);
    assert(index.end(9)==0 && index.end(99)==3);
    std::cout << "support regression: all assertions passed\n";
}
