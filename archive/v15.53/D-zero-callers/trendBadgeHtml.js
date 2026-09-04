// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function trendBadgeHtml(){
  var info=trendStateInfo();
  var v=info.verdict||{};
  var counter = v.win ? (v.up+'/'+v.win) : '';
  var slopeTxt = (typeof v.slope==='number') ? ((v.slope>0?'+':'')+(Math.round(mul(v.slope,100))/100)) : 'n/a';
  var tip='Trend state from closes vs SMA'+CFG.trendMA.SPY+' over the last '+(v.win||0)+' closed 3m bars plus SMA slope. Counter = closes on the dominant side ('+counter+'). Slope '+slopeTxt+'.';
  return '<span title="'+tip.replace(/"/g,'')+'" style="color:'+info.color+';font-weight:700;font-size:10px;padding:1px 8px;border:1px solid '+info.color+';border-radius:20px;background:rgba(255,255,255,0.02)">'+info.label+'</span>';
}
