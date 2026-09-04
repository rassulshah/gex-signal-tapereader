// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function A_kingBehavior(day, sym){
  sym=sym||'SPY';
  var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  var out={ sym:sym, pts:0, levels:[], distinct:0, firstK:null, lastK:null, netDrift:null,
            rolls:0, rollUp:0, rollDn:0, avgRollSize:null, regimes:[],
            reachRate:null, reachN:0, reachHit:0, avgTimeToReach:null, convergeRate:null,
            offsetAvg:null, aboveBars:0, belowBars:0, atBars:0, pullDir:null,
            pinned:null, pinDist:null, pinTiming:null, closeK:null, closePx:null };
  if(!snaps.length) return out;
  var path=[]; snaps.forEach(function(s,i){ var k=A_num(s.tking), p=A_num(s.px); if(k==null) return; path.push({i:i,bar:s.bar,k:k,px:p}); });
  out.pts=path.length; if(!path.length) return out;
  out.firstK=path[0].k; out.lastK=path[path.length-1].k; out.netDrift=+(out.lastK-out.firstK).toFixed(2);
  var kset={}, dwell={}; path.forEach(function(p){ kset[p.k]=1; dwell[p.k]=(dwell[p.k]||0)+1; });
  out.levels=Object.keys(kset).map(Number).sort(function(a,b){return a-b;}); out.distinct=out.levels.length;
  out.dwell=dwell;
  // CORE = strikes the King actually HELD (>=2 bars), ignoring 1-bar outliers like
  // an EOD pin-roll. Range/tightness for regime classification uses the core.
  var core=out.levels.filter(function(k){ return dwell[k]>=2; }); if(!core.length) core=out.levels.slice();
  out.core=core; out.coreRange=core.length?(core[core.length-1]-core[0]):0; out.coreCount=core.length;
  var rollSizes=[], prev=null;
  path.forEach(function(p){ if(prev!=null && p.k!==prev){ out.rolls++; var d=p.k-prev; rollSizes.push(Math.abs(d)); if(d>0) out.rollUp++; else out.rollDn++; } prev=p.k; });
  out.avgRollSize=rollSizes.length? +(rollSizes.reduce(function(a,b){return a+b;},0)/rollSizes.length).toFixed(2):null;
  var offs=[]; path.forEach(function(p){ if(p.px==null) return; var off=p.k-p.px; offs.push(off); if(off>0.001) out.aboveBars++; else if(off<-0.001) out.belowBars++; else out.atBars++; });
  if(offs.length){ out.offsetAvg=+(offs.reduce(function(a,b){return a+b;},0)/offs.length).toFixed(2); out.pullDir=out.offsetAvg>0.1?'up':(out.offsetAvg<-0.1?'down':'flat'); }
  var regimes=[], cur=null;
  path.forEach(function(p){ if(!cur||p.k!==cur.k){ if(cur) regimes.push(cur); cur={k:p.k,startI:p.i,bars:0,reached:false,reachBars:null,gapStart:null,gapEnd:null}; } cur.bars++; if(p.px!=null){ var gap=Math.abs(p.k-p.px); if(cur.gapStart==null)cur.gapStart=gap; cur.gapEnd=gap; if(gap<=0.25&&!cur.reached){ cur.reached=true; cur.reachBars=cur.bars-1; } } });
  if(cur) regimes.push(cur); out.regimes=regimes;
  var reachN=0,reachHit=0,ttr=[],conv=0,convN=0;
  regimes.forEach(function(r){ if(r.gapStart!=null&&r.gapStart>0.25){ reachN++; if(r.reached){ reachHit++; if(r.reachBars!=null)ttr.push(r.reachBars);} if(r.gapEnd!=null){ convN++; if(r.gapEnd<r.gapStart)conv++; } } });
  out.reachN=reachN; out.reachHit=reachHit; out.reachRate=A_pct(reachHit,reachN);
  out.avgTimeToReach=ttr.length? +(ttr.reduce(function(a,b){return a+b;},0)/ttr.length).toFixed(1):null;
  out.convergeRate=A_pct(conv,convN);
  // PIN: where did price CLOSE vs the day's final King (doc: late reach=pin, early=drive-off)
  var lastWithPx=null; for(var i=path.length-1;i>=0;i--){ if(path[i].px!=null){ lastWithPx=path[i]; break; } }
  if(lastWithPx){ out.closeK=lastWithPx.k; out.closePx=lastWithPx.px; out.pinDist=+Math.abs(lastWithPx.k-lastWithPx.px).toFixed(2);
    out.pinned=out.pinDist<=1.0; // ~0.5-1.0pt SPY zone (docs: ~5-10pt SPX)
    var lastReg=regimes[regimes.length-1]; var frac=lastReg&&path.length? (lastReg.startI/(path.length-1)) : 1;
    out.pinTiming=(frac>=0.66)?'late':(frac<=0.33?'early':'mid');
  }
  return out;
}
