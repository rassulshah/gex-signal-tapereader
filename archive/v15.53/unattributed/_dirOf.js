// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function _dirOf(sig){
  // net directional lean of a snapshot's read: prefer confluence, else King+trend blend
  if(!sig) return 0;
  if(sig.conf && typeof sig.conf.dir==='number' && sig.conf.dir!==0) return sig.conf.dir>0?1:-1;
  var s=0;
  if(sig.king && typeof sig.king.drift==='number') s+=(sig.king.drift>0?1:(sig.king.drift<0?-1:0));
  if(sig.trend){ if(sig.trend.state==='up')s+=1; else if(sig.trend.state==='down')s-=1; }
  return s>0?1:(s<0?-1:0);
}
