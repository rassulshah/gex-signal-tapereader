// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function feedKeyScan(o, want, depth, path, hits){
  depth=depth||0; path=path||''; hits=hits||[];
  if(!o || typeof o!=='object' || depth>4 || hits.length>=12) return hits;
  var ks=Object.keys(o);
  for(var i=0;i<ks.length && hits.length<12;i++){
    var k=ks[i], p=path?(path+'.'+k):k;
    if(want.test(k)) hits.push({ path:p, type:typeof o[k], sample:(typeof o[k]==='object'?'[obj]':o[k]) });
    var v=o[k];
    if(!v || typeof v!=='object') continue;
    if(Array.isArray(v)){
      // FIRST AND LAST, not just [0]. The feed's `levels` array is a time series whose first snapshot is
      // routinely empty — scanning index 0 alone reported "no call/put fields" off a payload that had them.
      if(!v.length) continue;
      var idxs=(v.length>1)?[0, v.length-1]:[0];
      for(var z=0;z<idxs.length;z++) feedKeyScan(v[idxs[z]], want, depth+1, p+'['+idxs[z]+']', hits);
    } else {
      feedKeyScan(v, want, depth+1, p, hits);
    }
  }
  return hits;
}
