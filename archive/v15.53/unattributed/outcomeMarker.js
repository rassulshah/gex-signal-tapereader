// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function outcomeMarker(o){
  if(o==='up')   return {txt:'broke \u2191', col:PAL.longAccent};
  if(o==='dn')   return {txt:'broke \u2193', col:PAL.shortAccent};
  if(o==='held') return {txt:'held', col:PAL.gold};
  if(o==='false')return {txt:'FBO', col:PAL.amber};   // (v10.34) false breakout
  return null;
}
