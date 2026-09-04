// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function confColour(nConf, nLive){
  if(!(nLive>0)) return '#8b98a9';          // nothing live: no verdict to colour
  if(nConf===nLive) return '#2ec27e';       // everything that could confirm, did
  if(nConf===0)     return '#f0616d';       // nothing confirming
  return '#8b98a9';
}
