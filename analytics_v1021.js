// ============================================================================
// GEX Analysis-tab analytics core (v10.21)
// Pure functions over a day-export object {snaps:{SPY:[...]}} — NO DOM, NO STATE.
// Splice these into v10.js after unit tests pass. Everything is defensive:
// missing/partial data yields {n:0, ...null} rather than throwing or fabricating.
//
// Causal-flow order these feed (top->bottom of the Analysis tab):
//   1 KING ANCHOR      kingBehavior()      where the magnet lived, how it pulled, rolls, reach
//   2 FOLLOW-THROUGH   (kingBehavior.reach + markup)  did price obey the pull
//   3 STRUCTURE        (existing analysisStats board tilt)
//   4 ACCUMULATION     accumEdge('accum')  did building nodes mark price toward them
//   5 DISSIPATION      accumEdge('fade')   did vacated levels let price through
//   6 COMBINED         combinedEdge()      trapdoor vs compression, confluence lift
//   7 CROSS-SYMBOL     crossKing()         SPY/QQQ/SPX king agreement (data-gated)
//   +  LLM synthesis reads all of the above IN THIS ORDER.
// ============================================================================

function _num(x){ return (typeof x==='number' && isFinite(x)) ? x : null; }
function _pctA(hit,n){ return n>0 ? Math.round(100*hit/n) : null; }

// ---- KING BEHAVIOR --------------------------------------------------------
// Answers: where did the magnet live, how far/long did it pull, how many rolls
// and in which direction, were rolls leading (predictive) or lagging (confirming),
// did the gap converge, did price reach it, and how fast.
function kingBehavior(day, sym){
  sym=sym||'SPY';
  var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  var out={ sym:sym, pts:0, levels:[], distinct:0, firstK:null, lastK:null,
            netDrift:null, rolls:0, rollUp:0, rollDn:0, avgRollSize:null,
            regimes:[], reachRate:null, reachN:0, reachHit:0,
            avgTimeToReach:null, convergeRate:null,
            offsetAvg:null, aboveBars:0, belowBars:0, atBars:0, pullDir:null };
  if(!snaps.length) return out;

  // Clean King path from tking (the real strike); pair with px.
  var path=[]; // {i, bar, k, px}
  snaps.forEach(function(s,i){
    var k=_num(s.tking), p=_num(s.px);
    if(k==null) return;
    path.push({i:i, bar:s.bar, k:k, px:p});
  });
  out.pts=path.length;
  if(!path.length) return out;

  out.firstK=path[0].k; out.lastK=path[path.length-1].k;
  out.netDrift=+(out.lastK-out.firstK).toFixed(2);
  var kset={}; path.forEach(function(p){ kset[p.k]=1; });
  out.levels=Object.keys(kset).map(Number).sort(function(a,b){return a-b;});
  out.distinct=out.levels.length;

  // Rolls (King relocations) + direction + size.
  var rollSizes=[], prev=null;
  path.forEach(function(p){
    if(prev!=null && p.k!==prev){
      out.rolls++; var d=p.k-prev; rollSizes.push(Math.abs(d));
      if(d>0) out.rollUp++; else out.rollDn++;
    }
    prev=p.k;
  });
  out.avgRollSize=rollSizes.length? +(rollSizes.reduce(function(a,b){return a+b;},0)/rollSizes.length).toFixed(2):null;

  // Offset / pull posture: King vs price each bar.
  var offs=[];
  path.forEach(function(p){
    if(p.px==null) return;
    var off=p.k-p.px; offs.push(off);
    if(off>0.001) out.aboveBars++; else if(off<-0.001) out.belowBars++; else out.atBars++;
  });
  if(offs.length){
    out.offsetAvg=+(offs.reduce(function(a,b){return a+b;},0)/offs.length).toFixed(2);
    out.pullDir = out.offsetAvg>0.1?'up':(out.offsetAvg<-0.1?'down':'flat'); // magnet mostly above=pull up
  }

  // Regimes = contiguous spans holding one King strike. For each: bars held,
  // whether price reached it (within 0.25) and how many bars until first reach,
  // and whether the price->King gap converged over the span (pull working).
  var regimes=[], cur=null;
  path.forEach(function(p){
    if(!cur || p.k!==cur.k){
      if(cur) regimes.push(cur);
      cur={ k:p.k, startI:p.i, bars:0, reached:false, reachBars:null,
            gapStart:null, gapEnd:null };
    }
    cur.bars++;
    if(p.px!=null){
      var gap=Math.abs(p.k-p.px);
      if(cur.gapStart==null) cur.gapStart=gap;
      cur.gapEnd=gap;
      if(gap<=0.25 && !cur.reached){ cur.reached=true; cur.reachBars=cur.bars-1; }
    }
  });
  if(cur) regimes.push(cur);
  out.regimes=regimes;

  // Reach rate across regimes that started with a real gap (price was away).
  var reachN=0, reachHit=0, ttr=[], conv=0, convN=0;
  regimes.forEach(function(r){
    if(r.gapStart!=null && r.gapStart>0.25){
      reachN++; if(r.reached){ reachHit++; if(r.reachBars!=null) ttr.push(r.reachBars); }
      if(r.gapEnd!=null){ convN++; if(r.gapEnd<r.gapStart) conv++; }
    }
  });
  out.reachN=reachN; out.reachHit=reachHit;
  out.reachRate=_pctA(reachHit,reachN);
  out.avgTimeToReach=ttr.length? +(ttr.reduce(function(a,b){return a+b;},0)/ttr.length).toFixed(1):null;
  out.convergeRate=_pctA(conv,convN);
  return out;
}

// ---- ACCUMULATION / DISSIPATION EDGE --------------------------------------
// mode 'accum': does a node BUILDING (net>0) on a side predict price moving
//   toward that side over out10?  support-below building -> up ; resistance-above building -> down
// mode 'fade':  does a node FADING (net<0) predict price moving THROUGH it?
//   support-below fading -> down ; resistance-above fading -> up
// Scored two ways per your call: by out10.net SIGN (close) and by MFE-hit (swing).
// Baseline = that day's unconditional up-rate, so lift-over-baseline is visible.
function _sideOf(node, px){
  // returns 'below' or 'above' relative to price (support vs resistance zone)
  var k=_num(node.k); if(k==null||px==null) return null;
  if(k<px-0.001) return 'below';
  if(k>px+0.001) return 'above';
  return null;
}
function accumEdge(day, sym, mode){
  sym=sym||'SPY';
  var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  // sub-buckets: support-side event, resistance-side event
  var B={ sup:{n:0,upNet:0,mfeHit:0,mfe:0,mae:0}, res:{n:0,upNet:0,mfeHit:0,mfe:0,mae:0} };
  var baseN=0, baseUp=0;
  snaps.forEach(function(s){
    var o=s.out10; if(!o) return;
    var px=_num(s.px); if(px==null) return;
    var netUp=(o.net>0)?1:0;
    baseN++; baseUp+=netUp;
    (s.nodes||[]).forEach(function(nd){
      var side=_sideOf(nd,px); if(!side) return;
      var net=_num(nd.net); if(net==null) return;
      var building=net>0, fading=net<0;
      var want = (mode==='accum')?building:fading;
      if(!want) return;
      // expected direction for THIS event:
      //  accum: support-below -> up(+1) ; resistance-above -> down(-1)
      //  fade:  support-below -> down(-1); resistance-above -> up(+1)
      var expUp;
      if(mode==='accum') expUp = (side==='below');   // toward the building wall
      else               expUp = (side==='above');   // through the vacated ceiling = up
      var bucket = (side==='below')?B.sup:B.res;
      bucket.n++;
      // close-sign hit: did out10.net go the expected way?
      var hit = expUp ? (o.net>0) : (o.net<0);
      if(hit) bucket.upNet++;   // reuse upNet as "expected-direction hit count"
      // swing hit: did the expected-direction excursion dominate?
      var mfe=_num(o.mfe)||0, mae=_num(o.mae)||0;
      bucket.mfe+=mfe; bucket.mae+=mae;
      var swingHit = expUp ? (mfe > -mae) : (-mae > mfe);
      if(swingHit) bucket.mfeHit++;
    });
  });
  function pack(b){ return { n:b.n, dirHit:_pctA(b.upNet,b.n), swingHit:_pctA(b.mfeHit,b.n),
    avgMFE:b.n? +(b.mfe/b.n).toFixed(2):null, avgMAE:b.n? +(b.mae/b.n).toFixed(2):null }; }
  return { mode:mode, sym:sym, baseline:_pctA(baseUp,baseN), baseN:baseN,
           support:pack(B.sup), resistance:pack(B.res) };
}

// ---- COMBINED / INTERACTION ----------------------------------------------
// trapdoor  = resistance-above building AND support-below fading -> expect DOWN
// liftoff   = support-below building  AND resistance-above fading -> expect UP
// compress  = both sides building -> expect RANGE (low |net|, both revUp&revDn)
// Also a net-flow polarity score per bar and its dir accuracy vs out10.
function combinedEdge(day, sym){
  sym=sym||'SPY';
  var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  var trap={n:0,hit:0}, lift={n:0,hit:0}, comp={n:0,range:0}, single={n:0,hit:0};
  var flow={n:0,hit:0};
  var baseN=0, baseUp=0;
  snaps.forEach(function(s){
    var o=s.out10; if(!o) return; var px=_num(s.px); if(px==null) return;
    baseN++; if(o.net>0) baseUp++;
    var resBuild=false,resFade=false,supBuild=false,supFade=false;
    var polar=0;
    (s.nodes||[]).forEach(function(nd){
      var side=_sideOf(nd,px); var net=_num(nd.net); if(!side||net==null) return;
      if(side==='above'){ if(net>0){resBuild=true; polar-=Math.abs(net);} else if(net<0){resFade=true; polar+=Math.abs(net);} }
      else             { if(net>0){supBuild=true; polar+=Math.abs(net);} else if(net<0){supFade=true; polar-=Math.abs(net);} }
    });
    // trapdoor
    if(resBuild&&supFade){ trap.n++; if(o.net<0) trap.hit++; single.n++; if(o.net<0) single.hit++; }
    // liftoff
    if(supBuild&&resFade){ lift.n++; if(o.net>0) lift.hit++; single.n++; if(o.net>0) single.hit++; }
    // compression
    if(supBuild&&resBuild){ comp.n++; var ranged=(Math.abs(o.net)<0.15)|| (o.revUp&&o.revDn); if(ranged) comp.range++; }
    // net-flow polarity dir accuracy
    if(Math.abs(polar)>0.001){ flow.n++; var pu=polar>0; if((pu&&o.net>0)||(!pu&&o.net<0)) flow.hit++; }
  });
  return {
    sym:sym, baseline:_pctA(baseUp,baseN), baseN:baseN,
    trapdoor:{n:trap.n, hit:_pctA(trap.hit,trap.n)},
    liftoff:{n:lift.n, hit:_pctA(lift.hit,lift.n)},
    compression:{n:comp.n, rangeRate:_pctA(comp.range,comp.n)},
    dualVsSingle:{ dualN:trap.n+lift.n, dualHit:_pctA(trap.hit+lift.hit,trap.n+lift.n) },
    netFlow:{n:flow.n, dirHit:_pctA(flow.hit,flow.n)}
  };
}

// ---- CROSS-SYMBOL KING (data-gated) ---------------------------------------
// Only reports when >1 symbol actually has King data; otherwise returns
// available:false with the reason, so the tab shows "capture pending" not fake numbers.
function crossKing(day){
  var syms=(day&&day.syms)?day.syms:Object.keys((day&&day.snaps)||{});
  var have=[];
  syms.forEach(function(sym){
    var snaps=(day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
    var kn=snaps.filter(function(s){return _num(s.tking)!=null;}).length;
    if(kn>0) have.push({sym:sym, kingPts:kn});
  });
  if(have.length<2){
    return { available:false, reason:'Only '+(have.length?have[0].sym:'SPY')+' has King data this day; QQQ/SPX capture pending.', have:have };
  }
  // Agreement of King pull direction (offset sign) bar-by-bar across symbols
  // (aligned by nearest bar timestamp). Kept simple: per-symbol pullDir from kingBehavior.
  var dirs={};
  have.forEach(function(h){ dirs[h.sym]=kingBehavior(day,h.sym).pullDir; });
  var vals=Object.keys(dirs).map(function(k){return dirs[k];});
  var agree = vals.every(function(v){return v===vals[0] && v!=null;});
  return { available:true, have:have, pullDirs:dirs, allAgree:agree };
}

module.exports={ kingBehavior:kingBehavior, accumEdge:accumEdge, combinedEdge:combinedEdge, crossKing:crossKing };
