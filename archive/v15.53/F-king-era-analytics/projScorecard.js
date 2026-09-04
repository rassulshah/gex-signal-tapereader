// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function projScorecard(){
  var db=recorderLoad(); var days=(db&&db.days)||{};
  var sc={reachN:0,reach:0, etaErrs:[], covN:0,cov:0, succN:0,succ:0, pinN:0,pin:0, days:0};
  for(var d in days){ if(!days.hasOwnProperty(d)) continue;
    var spy=((days[d]||{}).snaps||{}).SPY||[]; var had=false;
    for(var i=0;i<spy.length;i++){ var pr=spy[i].proj; if(!pr) continue; had=true;
      // target reached by close?
      var reached=null, firstAt=null;
      for(var j=i+1;j<spy.length;j++){ var pj=spy[j].px;
        if(typeof pj==='number' && Math.abs(pj-pr.tgt)<=0.5){ reached=true; if(firstAt==null) firstAt=j-i; break; } }
      if(spy.length-i>=5){ sc.reachN++; if(reached) sc.reach++; }
      if(pr.eta!=null && firstAt!=null){ sc.etaErrs.push((firstAt-pr.eta)*3); }
      // cone coverage at eta (or 10 bars)
      var hb=(pr.eta!=null?pr.eta:10);
      if(i+hb<spy.length && typeof spy[i+hb].px==='number'){
        sc.covN++; var drift=pr.tgt-pr.px, frac=1; var exp=pr.px+drift*Math.min(1,hb/(pr.eta||hb));
        if(Math.abs(spy[i+hb].px-exp)<=pr.ch) sc.cov++; }
      // succession scoring
      if(pr.sa!=null && pr.sa>=60 && pr.sk!=null){
        var hit=false; for(var q=i+1;q<Math.min(i+21,spy.length);q++){ if(spy[q].tking===pr.sk){ hit=true; break; } }
        if(spy.length-i>=5){ sc.succN++; if(hit) sc.succ++; } }
      // pin scoring: only score the LAST projection of the day that flagged pin
      if(pr.pin && i>=spy.length-3){ var lastPx=spy[spy.length-1].px;
        if(typeof lastPx==='number'){ sc.pinN++; if(Math.abs(lastPx-pr.tgt)<=0.5) sc.pin++; } }
    }
    if(had) sc.days++;
  }
  sc.etaErrs.sort(function(a,b){return a-b;});
  sc.etaMed=sc.etaErrs.length?sc.etaErrs[Math.floor(sc.etaErrs.length/2)]:null;
  return sc;
}
