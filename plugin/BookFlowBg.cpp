// BookFlowBg.cpp  --  lsBookFlowBg 1.40.1: lsBookFlow running in the background (same source, BookFlow.cpp + BF_BACKGROUND): added to the PRICE pane it draws
// nothing and takes no room, and keeps writing every BookFlow record. Use ONE of lsBookFlow / lsBookFlowBg per market.
// gex-build reads the version from this line:  p->setVersion("1.40.1")
#define BF_BACKGROUND 1
#include "BookFlow.cpp"
