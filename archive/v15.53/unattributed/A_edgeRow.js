// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function A_edgeRow(label, edge, baseline, tip){
  var dh=edge.dirHit, sw=edge.swingHit, n=edge.n;
  var lift=(dh!=null&&baseline!=null)?(dh-baseline):null;
  var col = dh==null?PAL.sub:(dh>=60?PAL.longAccent:(dh>=50?PAL.amber:PAL.shortAccent));
  var liftTxt = lift==null?'':(' <span style="color:'+(lift>0?PAL.longAccent:PAL.sub)+'">('+(lift>0?'+':'')+lift+' vs base)</span>');
  var payoff = (edge.avgMFE!=null&&edge.avgMAE!=null)? (' \u00b7 MFE '+edge.avgMFE+' / MAE '+edge.avgMAE) : '';
  return '<div style="display:flex;align-items:center;gap:8px;padding:4px 0;border-bottom:1px solid #161b26"'+A_tip(tip)+'>'+
    '<span style="flex:0 0 128px;font-size:10px;color:#c8d3df">'+label+'</span>'+
    '<span style="flex:1;font-size:10px;color:'+col+';font-weight:700">'+(dh==null?'\u2013 (insufficient)':dh+'%')+liftTxt+
      '<span style="color:'+PAL.sub+';font-weight:400"> \u00b7 swing '+(sw==null?'\u2013':sw+'%')+payoff+' \u00b7 n'+n+'</span></span></div>';
}
