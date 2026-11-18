#include "SdmoLogic.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
int main(int,char**argv){
    std::ifstream f(argv[1]); std::string ln; sdmo::Engine E; int fails=0, i=0;
    std::ifstream py(argv[2]); std::string pl;
    while(std::getline(f,ln)){ sdmo::Bar b; long long s; std::sscanf(ln.c_str(),"%lld|%lf|%lf|%lf",&s,&b.h,&b.l,&b.c); b.t=i; b.local=s; E.add(b);
        sdmo::Read r=sdmo::readAt(E,i); std::getline(py,pl); std::string mine = r.ok? std::to_string(i)+"|"+r.head+"|"+r.l1+"|"+r.l2 : std::to_string(i)+"|-";
        // python formats -0 as '-0' and uses '->' arrows the same; compare heads strictly, lines loosely
        std::string ph=pl.substr(0,pl.find('|',pl.find('|')+1)), mh=mine.substr(0,mine.find('|',mine.find('|')+1));
        if (ph!=mh){ ++fails; if(fails<15) std::printf("DIFF %s\n   py %s\n  cpp %s\n", ph.c_str(), pl.c_str(), mine.c_str()); }
        ++i; }
    // repaint: a replay gives the same read for every closed bar (pure function of closed bars)
    std::printf("%d bars, %d head diffs\n", i, fails); return fails?1:0; }
