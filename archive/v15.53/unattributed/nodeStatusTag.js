// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeStatusTag(L){
  // (v10.44) STATE = Acm / Dec / Steady (Diss RENAMED Dec; Acm kept — Step 5 doctrine)
  // + the node's ▲/▼% vs its own session open (same convention as %KCH). Threshold-
  // colored: |chg|>=15 bright, else dim. Measured note in the hover: Building walls within
  // 1.5 strikes were reached only 15% of the time (n=167) vs Fading 23% (📊 4d study).
  var lab = L.state==='Building' ? 'Acm' : (L.state==='Fading' ? 'Dec' : 'Steady');
  var col = L.state==='Building' ? PAL.longAccent : (L.state==='Fading' ? PAL.shortAccent : PAL.sub);
  var reshuf='';
  if(L.rapid){
    var ic = (L.rapidDir>0) ? '\uD83D\uDD25' : '\u2744';
    reshuf=' <span title="Reshuffling \u2014 rapid exposure change (new structure forming)." style="font-size:9px">'+ic+'</span>';
  }
  var tip = (lab==='Acm'?'Accumulation \u2014 dealers building here \u2192 stronger magnet. \uD83D\uDCCA Acm walls within 1.5 strikes were reached only 15% of the time in 30m (n=167) \u2014 the sturdiest walls.':
            (lab==='Dec'?'Decumulation \u2014 dealers closing here \u2192 weakening magnet. \uD83D\uDCCA Fading walls reached 23% (n=99) \u2014 the leakiest.':
            'Steady \u2014 holding, no net build/decay. \uD83D\uDCCA reached 20% (n=508).'));
  var chgHtml='';
  if(typeof L.chg==='number' && L.chg!==0){
    var big=Math.abs(L.chg)>=15;
    var ccol=L.chg>0?PAL.longAccent:PAL.shortAccent;
    chgHtml=' <span title="'+(L.chg>0?'\u25b2':'\u25bc')+Math.abs(L.chg)+'% \u2014 this node\u2019s magnitude vs its first reading today (quote-page %change convention). \u00b115% = big.'+'" style="color:'+ccol+';font-size:8px;font-weight:'+(big?'800':'600')+';opacity:'+(big?'1':'.65')+'">'+(L.chg>0?'\u25b2':'\u25bc')+Math.abs(L.chg)+'%</span>';
  }
  // (v10.47 A.3, approved mockup) plain text, no pill: "Acm ▲12%" / "Dec ▼8%" / "Steady"
  return '<span title="'+tip+'" style="color:'+col+';font-size:8.5px;font-weight:800;white-space:nowrap">'+lab+'</span>'+chgHtml+reshuf;
}
