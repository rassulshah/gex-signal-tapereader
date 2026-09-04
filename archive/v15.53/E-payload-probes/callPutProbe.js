// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function callPutProbe(sym){
  try{
    var f=LASTFEED[sym||'SPY']; if(!f||!f.j) return { err:'no feed captured yet' };
    var snaps=f.j.levels||[]; var last=snaps[snaps.length-1]; var l=(last&&last.l)||[];
    if(!l.length) return { err:'no strike rows in the latest snapshot' };
    var lt=0, eq=0, gt=0, noNet=0, rows=[];
    for(var i=0;i<l.length;i++){
      var n=l[i];
      if(typeof n.v!=='number' || typeof n.net!=='number'){ noNet++; continue; }
      var v=Math.abs(n.v), nt=n.net, an=Math.abs(nt);
      var tol=Math.max(1e-6, v*1e-6);
      if(an < v-tol) lt++; else if(an <= v+tol) eq++; else gt++;
      // (v11.17) same sign correction as cpRows: put=(v+net)/2. Skylit's net is POSITIVE on a
      // put-dominated strike. Verified against a published SPY page — this convention puts the put wall
      // on 760, exactly their number; the other one produced 756 and a call-heavy book that was not real.
      rows.push({ k:n.k, v:v, net:nt, call:+(((v-nt)/2)).toFixed(2), put:+(((v+nt)/2)).toFixed(2) });
    }
    var n=rows.length;
    var decomposable = (n>0 && lt >= Math.max(3, Math.round(n*0.2)) && gt===0);
    var out={ strikes:l.length, withNet:n, noNet:noNet,
              absNetLessThanV:lt, absNetEqualsV:eq, absNetGreaterThanV:gt,
              decomposable:decomposable, sample:rows.slice(0,6) };
    if(gt>0){ out.verdict='|net| EXCEEDS v on '+gt+' strikes — v is not a total, so do NOT decompose. The fields mean something else.'; return out; }
    if(!decomposable){ out.verdict='|net| equals v on every strike — net is just the signed magnitude, carrying no extra information. The call/put split is genuinely absent and the call wall is not computable.'; return out; }
    // The good case: derive call and put per strike and place the walls their way.
    var px=(STATE[sym||'SPY']||{}).price;
    var cw=null,cwm=-1, pw=null,pwm=-1, sumC=0, sumP=0;
    rows.forEach(function(r){
      sumC+=r.call; sumP+=r.put;
      if(px!=null && r.k>px && r.call>cwm){ cwm=r.call; cw=r.k; }
      if(px!=null && r.k<px && r.put>pwm){ pwm=r.put; pw=r.k; } });
    out.callWall=cw; out.callWallGex=(cwm<0?null:Math.round(cwm));
    out.putWall=pw;  out.putWallGex=(pwm<0?null:Math.round(pwm));
    out.totalCall=Math.round(sumC); out.totalPut=Math.round(sumP);
    out.ratio=(sumP>0)?+(sumC/sumP).toFixed(2):null;
    out.px=px;
    out.verdict='DECOMPOSABLE — v is total, net is net. call=(v+net)/2, put=(v-net)/2. The CALL WALL is computable from this feed, and the call/put ratio can be cross-checked against their header.';
    return out;
  }catch(e){ return { err:String(e) }; }
}
