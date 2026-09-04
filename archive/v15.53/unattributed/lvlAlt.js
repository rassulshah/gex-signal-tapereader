// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function lvlAlt(x){
  if(x==null) return null;
  try{
    var fut=(typeof dispIsFut==='function') && dispIsFut();
    if(fut) return { label:'SPY', txt:(+x).toFixed(2).replace(/\.?0+$/,''), approx:false };
    var R=irtRatio(); if(!R || !R.r) return null;
    return { label:'ES', txt:String(Math.round(mul(x, R.r))), approx:!R.live };
  }catch(e){ return null; }
}
