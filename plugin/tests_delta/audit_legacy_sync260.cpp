#include "../DeltaProfile.cpp"
#include <iostream>
#include <set>
namespace mock { Host* current=NULL; }
int main(int argc,char**argv){
 if(argc<2)return 2;std::string text;bool big=false;if(!readWholeFile(argv[1],text,big))return 3;
 std::set<std::pair<std::string,double>> roots;std::istringstream in(text);std::string line;
 while(std::getline(in,line)){auto f=fields(line);if(f.size()==22&&f[0]!="t_first"){double tick=0;if(parseReal(f[7],tick))roots.insert(std::make_pair(f[13],tick));}}
 for(const auto& root:roots){LatchBook bk;seedFromText(text,root.first,root.second,bk);int a=0,i=0,p=0;
  for(const Latch& L:bk.v){if(confirmedAbsorption(L))++a;else if(L.code=="I"||L.code=="Acc"||L.code=="Dst")++i;else ++p;}
  std::cout<<root.first<<','<<bk.v.size()<<','<<a<<','<<i<<','<<p<<','<<bk.rejected<<'\n';
 }
}
