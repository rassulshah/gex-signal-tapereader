// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function rollDetect(sym){
  sym=sym||'SPY';
  try{
    var f=LASTFEED[sym]; if(!f||!f.j) return null;
    // (v11.53) A feed that EXISTS but is too short must SAY SO. Returning bare null made it
    // indistinguishable from "no feed at all", which is the one case null is reserved for — and
    // saying why is the refusal doctrine everywhere else in this file. accumAsym keeps its own
    // null: it is a value, not a read, and has no refusal channel to speak through.
    var snaps=f.j.levels||[]; if(snaps.length<12) return { err:'not enough session yet' };
    var last=snaps.length-1, tL=snaps[last].t;
    if(typeof tL!=='number') return null;
    // `t` is SECONDS in this payload. Walk back by time, not by index, so a cadence change cannot
    // silently turn a 30-minute window into a 5-minute one.
    var target=tL-ROLL_WIN_MIN*60, a=-1;
    for(var i=last;i>=0;i--){ if(typeof snaps[i].t==='number' && snaps[i].t<=target){ a=i; break; } }
    if(a<0 || last-a<5) return { err:'not enough session yet' };
    function mapOf(ix){ var m={}, l=(snaps[ix]&&snaps[ix].l)||[];
      for(var q=0;q<l.length;q++){ var r=l[q]; if(typeof r.v==='number') m[r.k]=Math.abs(r.v); } return m; }
    var m0=mapOf(a), m1=mapOf(last);
    var px=(STATE[sym]||{}).price;
    if(typeof px!=='number'){ px=(typeof snaps[last].s==='number')?snaps[last].s:null; }
    if(px==null) return { err:'no price' };
    var cand=[], pcs=[];
    for(var k in m1){
      var kk=parseFloat(k); if(!isFinite(kk)) continue;
      if(Math.abs(kk-px)>ROLL_REACH) continue;
      var v0=m0[k], v1=m1[k];
      if(v0==null||v1==null||v0<ROLL_MIN_V) continue;
      var pc=(v1-v0)/v0;
      cand.push({k:kk, pc:pc, v0:v0, v1:v1}); pcs.push(pc);
    }
    if(cand.length<4) return { err:'book too thin to compare' };
    var base=rollMedian(pcs);
    var out={ base:+(base*100).toFixed(0), n:cand.length, winMin:ROLL_WIN_MIN, ceil:null, flr:null, err:null };
    ['ceil','flr'].forEach(function(side){
      var up=(side==='ceil');
      var pool=cand.filter(function(c){ return up ? c.k>px : c.k<px; });
      var diss=null, acc=null;
      pool.forEach(function(c){
        var rel=c.pc-base;
        if(rel<=-ROLL_TH && (!diss||rel<diss.rel)) diss={k:c.k, rel:rel, v0:c.v0, v1:c.v1};
        if(rel>= ROLL_TH && c.v1>=ROLL_MIN_V && (!acc||rel>acc.rel)) acc={k:c.k, rel:rel, v0:c.v0, v1:c.v1};
      });
      // A node dying with nothing growing is a wall EVAPORATING, not a roll. It gets no arrow —
      // an arrow to nowhere is a claim we cannot support.
      if(!diss || !acc || Math.abs(diss.k-acc.k)<0.5) return;
      out[side]={ from:diss.k, to:acc.k,
                  fromPct:Math.round(diss.rel*100), toPct:Math.round(acc.rel*100),
                  toward: up ? (acc.k<diss.k) : (acc.k>diss.k) };
    });
    return out;
  }catch(e){ return { err:String(e&&e.message||e) }; }
}
