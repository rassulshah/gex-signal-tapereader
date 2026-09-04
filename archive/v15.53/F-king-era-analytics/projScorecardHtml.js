// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function projScorecardHtml(){
  var sc=projScorecard(); var recs=projRecs(sc);
  function row(l,v,tip){ return '<div title="'+(tip||'')+'" style="display:flex;justify-content:space-between;padding:2px 0;border-bottom:1px dashed rgba(255,255,255,0.05);font-size:10px">'+
    '<span style="color:'+PAL.sub+'">'+l+'</span><span style="color:'+PAL.ink+';font-weight:600">'+v+'</span></div>'; }
  function pct(a,b){ return b?Math.round(100*a/b)+'%':'● recording'; }
  var h='<div style="padding:5px 8px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:4px 0">'+
    '<div title="Scores every projection the Dashboard drew, after the fact. This is the sharpening loop: the projected chart’s labels re-read from these live rates once they unlock." style="color:'+PAL.gold+';font-size:9px;font-weight:800;letter-spacing:.5px;margin-bottom:3px">🎯 PROJECTION SCORECARD <span style="color:'+PAL.sub+';font-weight:400">('+sc.days+'d recorded)</span></div>'+
    row('Succession ≥60% → crowned ≤30m', (sc.succN?pct(sc.succ,sc.succN)+' n='+sc.succN:'● recording'), 'Live version of the measured '+SUCC_META.hz30+'% (n='+SUCC_META.n+', '+SUCC_META.sessions+' sessions). ⚠ The withdrawn '+SUCC_META.withdrawn+'% claim was scored against the RAW tape, where a one-snapshot flicker counted as a succession.')+
    row('Target reached by close', pct(sc.reach,sc.reachN)+(sc.reachN?' n='+sc.reachN:''), 'Any later bar within ±0.5 of the projected target.')+
    row('ETA error (median)', (sc.etaMed!=null?(sc.etaMed>0?'+':'')+sc.etaMed+'m n='+sc.etaErrs.length:'● recording'), 'First-touch time minus projected ETA.')+
    row('Cone coverage', pct(sc.cov,sc.covN)+(sc.covN?' n='+sc.covN:''), 'Price within the volatility cone at the projection horizon. Target ~70%.')+
    row('Pin-band hit', pct(sc.pin,sc.pinN)+(sc.pinN?' n='+sc.pinN:''), 'Close within ±0.5 of target when a pin band was flagged.')+
    '<div style="color:'+PAL.gold+';font-size:8.5px;font-weight:700;margin:4px 0 2px 0">🔧 RECOMMENDATIONS (auto)</div>';
  for(var i=0;i<recs.length;i++){ var r=recs[i];
    var col=r.sev==='high'?PAL.shortAccent:(r.sev==='med'?PAL.amber:(r.sev==='ok'?PAL.longAccent:PAL.sub));
    h+='<div style="font-size:9px;color:'+PAL.ink+';padding:1px 0"><span style="color:'+col+'">▪</span> '+r.t+'</div>'; }
  h+='<div style="font-size:7.5px;color:'+PAL.sub+';margin-top:3px;opacity:.8">included in the day export → consumed by the end-of-day LLM review automatically</div></div>';
  return h;
}
