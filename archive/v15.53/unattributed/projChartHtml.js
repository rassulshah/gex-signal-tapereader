// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function projChartHtml(A, P){
  if(!A||!A.ok||!P) return '';
  var W=262,H=118,padL=4,padR=46,padT=8,padB=14;
  var barsLeft=Math.max(4, Math.round((A.toClose||30)/3));
  function half(b){ return projTaperHalf(b, P.etaBars, barsLeft, P.cone.half); }
  // ---- (v10.43) FOCUSED Y-DOMAIN: scale to the price<->target ACTION, not to
  // every rail. Rails outside the window collapse into edge tags so a far HOD
  // can no longer squash the projection into the bottom fifth of the chart.
  var ys=[A.px, P.tgt, A.px+half(barsLeft), A.px-half(barsLeft), P.tgt+0.6, P.tgt-0.6];
  if(P.t1!=null) ys.push(P.t1);
  var aLo=Math.min.apply(null,ys), aHi=Math.max.apply(null,ys);
  var inRails=[], edgeHi=[], edgeLo=[];
  for(var rI=0;rI<P.rails.length;rI++){ var R0=P.rails[rI];
    if(R0.k>=aLo-1.2 && R0.k<=aHi+1.2) inRails.push(R0);
    else if(R0.k>aHi) edgeHi.push(R0.label+' '+fmtNum(R0.k));
    else edgeLo.push(R0.label+' '+fmtNum(R0.k));
  }
  for(var rj=0;rj<inRails.length;rj++){ ys.push(inRails[rj].k); }
  var yLo=Math.min.apply(null,ys)-0.5, yHi=Math.max.apply(null,ys)+0.5;
  function Y(k){ return padT+(yHi-k)/(yHi-yLo)*(H-padT-padB); }
  function X(b){ return padL+(b/barsLeft)*(W-padL-padR); }
  function esc(t){ return (''+t).replace(/&/g,'&amp;').replace(/</g,'&lt;'); }
  function tt(t){ return '<title>'+esc(t)+'</title>'; }
  var svg='<svg width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" preserveAspectRatio="xMidYMid meet" style="display:block;width:100%;height:auto">';
  svg+='<line x1="'+(W-padR)+'" y1="0" x2="'+(W-padR)+'" y2="'+H+'" stroke="'+PAL.line+'"/>';
  // edge tags for out-of-window rails
  if(edgeHi.length){ svg+='<g>'+tt('Above this view: '+edgeHi.join(' · ')+'. Rails outside the action window are tagged here instead of stretching the axis.')+
    '<text x="'+(padL+2)+'" y="8" fill="'+PAL.blue+'" opacity="0.9">▲ '+esc(edgeHi.join(' · '))+'</text></g>'; }
  if(edgeLo.length){ svg+='<g>'+tt('Below this view: '+edgeLo.join(' · '))+
    '<text x="'+(padL+2)+'" y="'+(H-3)+'" fill="'+PAL.sub+'" opacity="0.9">▼ '+esc(edgeLo.join(' · '))+'</text></g>'; }
  // rails with LABEL ANTI-COLLISION (nudge to >=9px separation)
  var explain={tgt:'T2 target = the operative King. Targets cap at the King (doctrine — no T3).',
    t1:'T1 = the gatekeeper between price and the King — first structural checkpoint.',
    eva:'Exposure value-area edge (70% of dealer mass). Inside = rotation; outside = imbalance — don’t fade (measured).',
    ref:'Session reference level (HOD/LOD).'};
  var placed=[];
  inRails.sort(function(a,b){ return Y(a.k)-Y(b.k); });
  for(var r2=0;r2<inRails.length;r2++){ var RR=inRails[r2]; var y=Y(RR.k);
    var col=RR.cls==='tgt'?PAL.gold:(RR.cls==='t1'?PAL.sub:(RR.cls==='eva'?PAL.blue:PAL.sub));
    var ly=y-2;
    for(var pQ=0;pQ<placed.length;pQ++){ if(Math.abs(ly-placed[pQ])<9) ly=placed[pQ]+9; }
    ly=Math.max(padT+6, Math.min(H-padB-2, ly)); placed.push(ly);
    svg+='<g>'+tt(RR.label+' '+fmtNum(RR.k)+' — '+(explain[RR.cls]||''))+
      '<line x1="'+padL+'" y1="'+y.toFixed(1)+'" x2="'+(W-padR)+'" y2="'+y.toFixed(1)+'" stroke="'+col+'" stroke-width="1" stroke-dasharray="2,3" opacity="0.6"/>'+
      '<text x="'+(padL+2)+'" y="'+ly.toFixed(1)+'" fill="'+col+'" opacity="0.9">'+RR.label+' '+fmtNum(RR.k)+'</text></g>';
  }
  // current King rail + projected step
  var yK=Y(A.king).toFixed(1);
  if(P.projKing!=null){
    var xStep=X(Math.min(4,barsLeft*0.3)).toFixed(1), yP=Y(P.projKing).toFixed(1);
    svg+='<g>'+tt('Current King '+fmtNum(A.king)+'. Solid = where the crown is now.')+
      '<path d="M '+padL+' '+yK+' H '+xStep+'" stroke="'+PAL.gold+'" stroke-width="2"/></g>';
    svg+='<g>'+tt('PROJECTED King roll → '+fmtNum(P.projKing)+'. Drawn only because Succession ≥ 60% of King mass. Base rate: '+(P.basis.succ||''))+
      '<path d="M '+xStep+' '+yK+' V '+yP+' H '+(W-padR)+'" stroke="'+PAL.gold+'" stroke-width="2" stroke-dasharray="5,4"/></g>';
  } else {
    svg+='<g>'+tt('King rail '+fmtNum(A.king)+' — no hot succession candidate; crown projected to hold.')+
      '<path d="M '+padL+' '+yK+' H '+(W-padR)+'" stroke="'+PAL.gold+'" stroke-width="2" stroke-dasharray="6,4"/></g>';
  }
  // price cone (tapered) + center path + ETA
  var x0=padL, y0=Y(A.px), steps=10, up=[],dn=[],mid=[];
  for(var sI=0;sI<=steps;sI++){ var b=barsLeft*sI/steps; var x=X(b);
    var drift=(P.etaBars!=null)?(P.tgt-A.px)*Math.min(1,b/P.etaBars):0;
    var cph=half(b);
    mid.push([x, Y(A.px+drift)]); up.push([x, Y(A.px+drift+cph)]); dn.push([x, Y(A.px+drift-cph)]);
  }
  function path(pts){ var d0=''; for(var q=0;q<pts.length;q++){ d0+=(q?' L ':'M ')+pts[q][0].toFixed(1)+' '+pts[q][1].toFixed(1);} return d0; }
  var cone=path(up); for(var q2=dn.length-1;q2>=0;q2--){ cone+=' L '+dn[q2][0].toFixed(1)+' '+dn[q2][1].toFixed(1); } cone+=' Z';
  svg+='<g>'+tt('Volatility cone: recent avg 3m bar range ('+P.cone.r.toFixed(2)+') widening with √bars until arrival, then TAPERING into the pin range. An envelope of normal movement — NOT a forecast band. Coverage scored nightly (target ~70%).')+
    '<path d="'+cone+'" fill="rgba(74,144,217,0.12)" stroke="rgba(74,144,217,0.30)" stroke-width="0.6"/></g>';
  svg+='<g>'+tt('Projected price path toward '+fmtNum(P.tgt)+(P.etaBars!=null?' at the measured 3-bar approach rate. '+(P.basis.eta||''):' — no approach in progress; flat projection.'))+
    '<path d="'+path(mid)+'" stroke="'+PAL.blue+'" stroke-width="1.6" stroke-dasharray="4,3" fill="none"/></g>';
  svg+='<circle cx="'+x0+'" cy="'+y0.toFixed(1)+'" r="2.6" fill="'+PAL.blue+'">'+tt('Price now: '+fmtNum(A.px))+'</circle>';
  if(P.etaBars!=null && P.etaBars<=barsLeft){
    var xe=X(P.etaBars).toFixed(1), ye=Y(P.tgt).toFixed(1);
    svg+='<circle cx="'+xe+'" cy="'+ye+'" r="3.2" fill="none" stroke="'+PAL.blue+'" stroke-width="1.4">'+
      tt('ETA at target ≈ '+(P.etaBars*3)+' min at current approach rate. ETA error is scored nightly.')+'</circle>';
  }
  if(P.pin){
    var yT=Y(P.tgt+0.5).toFixed(1), hB=(Y(P.tgt-0.5)-Y(P.tgt+0.5)).toFixed(1);
    svg+='<g>'+tt('PIN BAND ±0.5: POWER phase + proximity — settlement gravity at the King. '+(P.basis.pin||''))+
      '<rect x="'+X(barsLeft*0.72).toFixed(1)+'" y="'+yT+'" width="'+(W-padR-X(barsLeft*0.72)).toFixed(1)+'" height="'+hB+'" fill="rgba(227,195,65,0.10)" stroke="rgba(227,195,65,0.4)" stroke-dasharray="2,2"/></g>';
  }
  svg+='<text x="'+(W-padR+4)+'" y="'+(Y(P.tgt)+3).toFixed(1)+'" fill="'+PAL.gold+'" font-weight="800">'+fmtNum(P.tgt)+'</text>';
  svg+='<text x="'+(W-padR+4)+'" y="'+(H-3)+'" fill="'+PAL.sub+'">close</text>';
  svg+='</svg>';
  return '<div title="Projected path from now to the close. Every element explains itself on hover; every projection is recorded this bar and scored nightly in Analysis ▸ 🎯." '+
    'style="padding:4px 6px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin-top:4px">'+
    '<div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:2px">'+
      '<span style="color:'+PAL.sub+';font-size:8px;font-weight:700;letter-spacing:.5px">🎯 KING PATH · PROJECTED</span>'+
      '<span title="Every projection drawn here is stored with this bar and scored after the fact — reach rate, ETA error, cone coverage, pin hits — in Analysis ▸ 🎯 Projection Scorecard." style="font-size:7.5px;color:'+PAL.sub+'">scored nightly 📊</span>'+
    '</div>'+svg+'</div>';
}
