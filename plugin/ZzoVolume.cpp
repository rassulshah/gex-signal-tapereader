/********************************************************************************
 *  ZzoVolume.cpp  --  Investor/RT RTX extension  lsZzoVolume  "ZZO Volume"  1.1.0  (2026-11-02)
 *  (Rassul 22:40 CT "use delta as default ... add the cmx and cob tags for volume"): ZZO Delta's engine in the VOLUME view - each wave's
 *  volume vs the median of the last 5 same-way waves (Weis Wave), green up / red down waves; tags Ab? (delta absorption), Cmx (climax:
 *  volume >= 2.5x at a new high / low), CoB (change of behaviour: the next wave the other way >= 1.5x). Rules: ZzoDeltaLogic.h.
 *  1.1.0 (22:52 CT): CoB renamed AR (Wyckoff automatic rally) + ST (secondary test) after Cmx + AR.
 *  gex-build reads the version from this line:  p->setVersion("1.1.0")
 ********************************************************************************/
#define ZZO_VOLUME_VIEW 1
#define ZZO_VOLUME_VERSION_LITERAL "1.1.0"
#include "ZzoDelta.cpp"
