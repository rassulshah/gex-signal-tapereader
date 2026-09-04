// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function feedShape(sym){
  try{
    var f=LASTFEED[sym||'SPY']; if(!f || !f.j) return { err:'no feed captured yet for '+(sym||'SPY') };
    var j=f.j;
    function laneShape(lane){
      try{
        var snaps=lane.levels||[]; var last=snaps[snaps.length-1]; var l=(last&&last.l)||[];
        var ks=l.map(function(x){ return x.k; }).filter(function(x){ return typeof x==='number'; });
        return { snapshots:snaps.length, strikes:l.length,
                 kMin:ks.length?Math.min.apply(null,ks):null, kMax:ks.length?Math.max.apply(null,ks):null,
                 strikeStep:(ks.length>2)?+(Math.abs(ks[1]-ks[0])).toFixed(2):null,
                 rowKeys:l.length?Object.keys(l[0]):null,
                 sample:l.slice(0,2),
                 snapKeys:last?Object.keys(last):null };
      }catch(e){ return { err:String(e) }; }
    }
    var out={
      topKeys:Object.keys(j),
      url:LASTFEEDURL||null,
      native:laneShape(j),
      derivedLanes:(j.derived||[]).map(function(d){
        return { source:d.source, ratio:d.ratio, keys:Object.keys(d), shape:laneShape(d) }; }),
      // THE decisive question: is gamma ever split call vs put anywhere in this payload?
      callPutKeys:feedKeyScan(j, /call|put|cGex|pGex|cOi|pOi/i),
      expiryKeys:feedKeyScan(j, /exp|dte|maturity/i)
    };
    out.verdict = out.callPutKeys.length
      ? 'call/put fields PRESENT — check whether they are per-strike or totals'
      : 'NO call/put fields anywhere in the payload — their Call Wall is not reproducible from this feed';
    return out;
  }catch(e){ return { err:String(e) }; }
}
