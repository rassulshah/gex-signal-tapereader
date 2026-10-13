#include "../DeltaProfile.cpp"
#include <iostream>
#include <fstream>
#include <functional>
namespace mock { Host* current=NULL; }
int main(int argc,char** argv){
 if(argc<4)return 2;setenv("TZ","UTC",1);tzset();setenv("USERPROFILE",argv[2],1);
 std::ifstream in(argv[1]);mock::Host h;h.seconds=180;h.root=argv[3];h.symbol=h.root+"Z26";
 double origin,scale;int count;in>>h.tick>>origin>>scale>>count;
 h.priceOrigin=origin;h.priceScale=scale;h.paneBottom=1100;h.paneRight=2200;h.scaleLeft=2180;
 std::vector<mock::Bar> all;
 for(int k=0;k<count;++k){mock::Bar b;int rows;in>>b.time>>b.open>>b.high>>b.low>>b.close>>b.volume>>rows;
  for(int j=0;j<rows;++j){mock::Price r;in>>r.price>>r.buy>>r.sell>>r.total;b.rows.push_back(r);}all.push_back(b);
 }
 bool restart=argc>4;std::unique_ptr<DeltaProfile> p(new DeltaProfile());
 std::cout<<"frame,t_end,events,confirmed_a,physical_bubbles,offscreen,suppressed,current_a,paired_a,violations,frame_hash\n";
 for(int n=32;n<=count;++n){
  mock::current=&h;if(restart){p->destroy();latchBooks().clear();p.reset(new DeltaProfile());}
  h.bars.assign(all.begin(),all.begin()+n);h.draws.clear();h.barOffset=std::max(0,n-170);
  if(p->draw()!=RTX_OK)return 3;
  ChartState& s=*static_cast<ChartState*>(h.data);const LatchBook& bk=latchBooks()[s.book+"|180"];
  int violations=0;
  for(const dp::Node& node:s.snapshot.nodes){
   const Latch* L=eventById(bk,node.state.eventId);
   if(node.state.code=="A"){
    if(!L||!confirmedAbsorption(*L)||node.state.support!=L->support||node.price!=L->price)++violations;
    auto q=s.profileState.find(node.state.eventId);auto b=s.bubbleState.find(node.state.eventId);
    if(q!=s.profileState.end()&&q->second=="shown:A"){
     if(b==s.bubbleState.end()||(b->second!="drawn"&&b->second!="shared"&&b->second!="offscreen"))++violations;
    }
   }
  }
  for(const auto& pair:s.bubbleState)if(pair.second=="drawn"||pair.second=="shared"){
   const Latch* L=eventById(bk,pair.first);if(!L||!confirmedAbsorption(*L))++violations;
  }
  unsigned long long hash=1469598103934665603ULL;
  for(const mock::Draw& d:h.draws){std::string x=d.type+"|"+d.text+"|"+std::to_string(d.x)+"|"+std::to_string(d.y)+"|"+std::to_string(d.color);for(unsigned char c:x){hash^=c;hash*=1099511628211ULL;}}
  std::cout<<n<<','<<h.bars.back().time<<','<<bk.v.size()<<','<<s.absorptionEvents<<','<<s.bubblesDrawn<<','<<s.bubblesOffscreen<<','<<s.bubblesSuppressed<<','<<s.activeAbsorptions<<','<<s.pairedAbsorptions<<','<<violations<<','<<hash<<'\n';
 }
 mock::current=&h;p->destroy();return 0;
}
