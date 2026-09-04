// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function recoTestsHtml(){
  var themes={}; RECO_TESTS.forEach(function(r){ (themes[r.th]=themes[r.th]||[]).push(r); });
  var readyN=RECO_TESTS.filter(function(r){return r.ready;}).length;
  var h='<div style="font-size:9.5px;font-weight:700;color:'+PAL.gold+';margin:7px 0 2px">⑥ Recommended tests <span style="font-size:8px;font-weight:400;color:'+PAL.sub+'">— '+readyN+'/'+RECO_TESTS.length+' runnable on recorded data · 📗 evidenced · 📙 plausible · 📕 folklore</span></div>';
  Object.keys(themes).forEach(function(th){
    h+='<div style="font-size:8px;letter-spacing:.4px;color:'+PAL.sub+';font-weight:700;margin:4px 0 1px;text-transform:uppercase">'+th+'</div>';
    themes[th].forEach(function(r){
      var col=r.ready?PAL.ink:PAL.sub; var badge=r.ready?'<span style="color:'+PAL.longAccent+'">✅</span>':'<span style="color:'+PAL.amber+'" title="needs data recorded at market open">⏳ '+r.need+'</span>';
      h+='<div style="font-size:9px;line-height:1.45;color:'+col+';padding:1px 0;display:flex;gap:5px"><span>'+r.s+'</span><span style="flex:1">'+r.t+'</span><span style="white-space:nowrap;font-size:8px">'+badge+'</span></div>';
    });
  });
  h+='<div style="font-size:7.5px;color:'+PAL.sub+';margin-top:3px;opacity:.85">⏳ rows unlock when the market-open session records VEX / multi-symbol / VIX-term fields. Sources: Barbon–Buraschi (gamma fragility), Ni–Pearson–Poteshman (pinning), SpotGamma/MenthorQ level defs, Skylit VEX. VEX/vanna directional claims are mechanistic but unbacktested — this tab is how we measure them.</div>';
  return h;
}
