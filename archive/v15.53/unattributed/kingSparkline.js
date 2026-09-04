// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function kingSparkline(mv, kingK, px, now, sess, verdictCol){
  // (v10.23 Issue A) +33% taller so the drift shape + always-on price line are legible.
  // (v10.40) padR widened into a reserved RIGHT GUTTER: price + King labels live
  // there, OUTSIDE the plot, so the dashed price line can never run under its own
  // label again (the old collision). Plot ends at W-padR; labels start after it.
  var W=262, H=112, padL=4, padR=46, padT=10, padB=10;
  var t0=sess.start, t1=sess.end;
  var firstT = mv.length ? mv[0].t : now;
  var xMin = Math.max(t0, firstT - 3*60000);
  if(xMin > now) xMin = t0;
  var xMax = Math.min(Math.max(now, xMin+60000), (t1>now?t1:now));
  if(xMax<=xMin) xMax=xMin+60000;
  var pts=[];
  for(var i=0;i<mv.length;i++){ var m=mv[i]; if(pts.length && Math.abs(pts[pts.length-1].k-m.k)<0.001) continue; pts.push({t:m.t,k:m.k}); }
  if(!pts.length) pts.push({t:xMin,k:(kingK!=null?kingK:0)});
  if(pts[0].t>xMin) pts.unshift({t:xMin,k:pts[0].k});
  var ks=pts.map(function(p){return p.k;});
  var yLo=Math.min.apply(null,ks), yHi=Math.max.apply(null,ks);
  // (v10.23 Issue A) GUARANTEE price sits inside the axis with real headroom, so the
  // dashed price line NEVER falls outside the padded window and gets dropped. Fold px
  // in BEFORE padding AND force >=1 strike of margin beyond it on the relevant side.
  var pxRef = (px!=null) ? px : (STATE.SPY? STATE.SPY.price : null);   // last-known fallback if null
  var pxStale = (px==null);
  if(pxRef!=null){
    yLo=Math.min(yLo, pxRef-1);
    yHi=Math.max(yHi, pxRef+1);
  }
  if(yHi-yLo<1){ yLo-=1; yHi+=1; }
  var yPad=(yHi-yLo)*0.15; yLo-=yPad; yHi+=yPad;
  function X(t){ return padL+(t-xMin)/(xMax-xMin)*(W-padL-padR); }
  function Y(k){ return padT+(yHi-k)/(yHi-yLo)*(H-padT-padB); }
  var netDir=pts[pts.length-1].k-pts[0].k;
  // (v10.11) PER-SEGMENT coloring: each staircase step is colored by ITS OWN
  // local direction \u2014 green where the King stepped UP, red where it stepped DOWN,
  // gray for the flat baseline holds \u2014 so the line shows that the King was, say,
  // bearish (red) most of the session and only turned flat recently, instead of
  // painting the whole path one current-verdict color. (verdictCol is no longer
  // used for the stroke; kept in the signature for compatibility.)
  function segColor(dv){ return dv>0?PAL.longAccent : (dv<0?PAL.shortAccent : PAL.sub); }
  // (v10.18) HYSTERESIS regime: the LINE color reflects the King's running trend,
  // and only flips when the King reclaims the prior pivot (>=1 strike beyond it),
  // so minor 1-strike wiggles inside a clear trend don't recolor the line. The
  // regime per vertex is precomputed here; segments below use it instead of the
  // raw local jump. (Dots keep their own local up/down color.)
  var BUF=2;   // strikes beyond the prior pivot required to flip regime (>=2 = ignore single-strike wiggles)
  var regimeAt=new Array(pts.length);
  (function(){
    var reg=0, pivotHi=pts[0].k, pivotLo=pts[0].k;
    regimeAt[0]=0;
    for(var q=1;q<pts.length;q++){
      var k=pts[q].k;
      if(k>pivotHi) pivotHi=k;
      if(k<pivotLo) pivotLo=k;
      if(reg>=0 && k <= pivotHi-BUF){ reg=-1; pivotLo=k; pivotHi=k; }        // fell a strike below the run's high -> down regime
      else if(reg<=0 && k >= pivotLo+BUF){ reg=1; pivotHi=k; pivotLo=k; }    // reclaimed a strike above the run's low -> up regime
      else if(reg===0){ reg = k>pts[0].k?1:(k<pts[0].k?-1:0); }
      regimeAt[q]=reg;
    }
  })();
  function regColor(r){ return r>0?PAL.longAccent : (r<0?PAL.shortAccent : PAL.sub); }
  var svg='<svg width="'+W+'" height="'+H+'" viewBox="0 0 '+W+' '+H+'" preserveAspectRatio="xMidYMid meet" style="display:block;width:100%;height:auto">';
  svg+='<line x1="'+(W-padR)+'" y1="0" x2="'+(W-padR)+'" y2="'+H+'" stroke="'+PAL.line+'" stroke-width="1"/>';
  // faint neutral fill under the whole staircase (context only, direction-agnostic)
  var baseY=(H-padB).toFixed(1);
  var fullD='M '+X(pts[0].t).toFixed(1)+' '+Y(pts[0].k).toFixed(1);
  for(var j=1;j<pts.length;j++){ fullD+=' H '+X(pts[j].t).toFixed(1)+' V '+Y(pts[j].k).toFixed(1); }
  fullD+=' H '+X(xMax).toFixed(1);
  var fillD=fullD+' L '+X(xMax).toFixed(1)+' '+baseY+' L '+X(pts[0].t).toFixed(1)+' '+baseY+' Z';
  svg+='<path d="'+fillD+'" fill="'+PAL.sub+'" opacity="0.05" stroke="none"/>';
  // (v10.23 Issue A) ALWAYS draw the price line. Clamp to the axis edge (with a small
  // caret) if price is beyond the padded range; dim it when price is stale (px null).
  var priceLineY=null;
  if(pxRef!=null){
    var clampedPx = Math.max(yLo, Math.min(yHi, pxRef));
    var beyond = (pxRef<yLo) ? -1 : (pxRef>yHi ? 1 : 0);
    var yp = Y(clampedPx);
    priceLineY = yp;
    var lineOp = pxStale ? 0.35 : 0.85;
    // (v10.27) price line made more distinct so it reads separately from the gold
    // King staircase even when they sit within a strike of each other: thicker,
    // longer dash, higher opacity, + a small blue price marker on the right end.
    svg+='<line x1="'+padL+'" y1="'+yp.toFixed(1)+'" x2="'+(W-padR)+'" y2="'+yp.toFixed(1)+'" stroke="'+PAL.blue+'" stroke-width="1.4" stroke-dasharray="4 3" opacity="'+lineOp+'"/>';
    if(beyond===0){ svg+='<circle cx="'+padL+'" cy="'+yp.toFixed(1)+'" r="2.2" fill="'+PAL.blue+'" opacity="'+lineOp+'"/>'; }
    if(beyond!==0){ // caret marking the line is clamped (real price is off-axis)
      var cX=(W-padR-10), cY=yp, dir=(beyond<0?1:-1);
      svg+='<path d="M '+cX+' '+(cY+dir*3).toFixed(1)+' L '+(cX+4)+' '+cY.toFixed(1)+' L '+(cX+8)+' '+(cY+dir*3).toFixed(1)+'" fill="none" stroke="'+PAL.blue+'" stroke-width="1" opacity="'+lineOp+'"/>'; }
  }
  // Draw each segment as its own colored stroke. Segment s (from vertex s-1 to s)
  // = a flat hold at pts[s-1].k then a vertical jump to pts[s].k; color by the
  // jump direction. The final hold (last vertex -> now) keeps the last direction.
  function strokeSeg(x1,y1,x2,y2,col){ return '<path d="M '+x1.toFixed(1)+' '+y1.toFixed(1)+' L '+x2.toFixed(1)+' '+y2.toFixed(1)+'" fill="none" stroke="'+col+'" stroke-width="1.8" stroke-linejoin="round" stroke-linecap="round"/>'; }
  var lastSegCol=PAL.sub;
  for(var s=1;s<pts.length;s++){
    var col=regColor(regimeAt[s]);                // (v10.18) color by hysteresis regime, not local dv
    var xPrev=X(pts[s-1].t), xCur=X(pts[s].t), yPrev=Y(pts[s-1].k), yCur=Y(pts[s].k);
    svg+=strokeSeg(xPrev,yPrev,xCur,yPrev,col);   // flat hold at previous strike
    svg+=strokeSeg(xCur,yPrev,xCur,yCur,col);     // vertical jump to new strike
    lastSegCol=col;
  }
  // trailing flat from the last vertex to 'now', in the last move's color
  var xl=X(pts[pts.length-1].t), yl=Y(pts[pts.length-1].k), xn=X(xMax);
  svg+=strokeSeg(xl,yl,xn,yl,lastSegCol);
  // roll vertices + gold current-King dot
  for(var v=1;v<pts.length;v++){ var cx=X(pts[v].t).toFixed(1), cy=Y(pts[v].k).toFixed(1); var up=pts[v].k>pts[v-1].k;
    svg+='<circle cx="'+cx+'" cy="'+cy+'" r="1.6" fill="'+(up?PAL.longAccent:PAL.shortAccent)+'"/>'; }
  // (v10.43) price labels at SIGNIFICANT moves so the path reads without hovering.
  var sig=kingPathSigMoves(pts, xMax);
  var lastLabX=-99;
  for(var g=0; g<sig.length; g++){ var sv=sig[g];
    var gx=X(pts[sv.i].t), gy=Y(pts[sv.i].k);
    if(gx-lastLabX<22) continue;                          // x-collision: skip crowded
    lastLabX=gx;
    var above=sv.up;                                       // label on the far side of the step
    var ty=above? Math.max(padT+7, gy-4) : Math.min(H-padB-2, gy+9);
    svg+='<text x="'+gx.toFixed(1)+'" y="'+ty.toFixed(1)+'" text-anchor="middle" fill="'+(sv.up?PAL.longAccent:PAL.shortAccent)+'" style="font-size:7.5px;font-weight:700;font-variant-numeric:tabular-nums">'+fmtNum(sv.k)+'</text>';
  }
  var lx=X(xMax).toFixed(1), ly=Y(pts[pts.length-1].k).toFixed(1);
  svg+='<circle cx="'+lx+'" cy="'+ly+'" r="3" fill="'+PAL.gold+'" stroke="'+PAL.card+'" stroke-width="1"/>';
  // (v10.27) King + price labels: place them so they NEVER collide even when King and
  // price are within a strike of each other (the case that stacked them before).
  //   - King label rides at the gold dot's Y (right-anchored, gold, crown).
  //   - Price label rides at the price line's Y (right-anchored, blue).
  //   - If the two Ys are within a min gap, push them APART vertically (King toward
  //     its own line side, price the other way) so both stay legible and separate.
  var curK = pts[pts.length-1].k;
  function clampY(y){ return Math.max(padT+8, Math.min(H-padB-2, y)); }
  var kingLineY = parseFloat(ly);
  var kingLabelY = clampY(kingLineY);
  var priceLabelY = (priceLineY!=null) ? clampY(priceLineY) : null;
  var MINGAP=15;   // (v10.43) raised: 13px price pill was overlapping the King label
  if(priceLabelY!=null && Math.abs(kingLabelY - priceLabelY) < MINGAP){
    // Too close -> separate around their midpoint: whichever sits HIGHER on the axis
    // (smaller Y) gets pinned MINGAP/2 above the midpoint, the other MINGAP/2 below.
    // Guarantees exactly >=MINGAP px between the two labels regardless of order.
    var mid = (kingLineY + priceLineY)/2;
    if(kingLineY <= priceLineY){ kingLabelY = clampY(mid - MINGAP/2); priceLabelY = clampY(mid + MINGAP/2); }
    else                       { priceLabelY = clampY(mid - MINGAP/2); kingLabelY = clampY(mid + MINGAP/2); }
  }
  // (v10.40) King label in the GUTTER at the King's Y, with a tick into the plot.
  svg+='<line x1="'+(W-padR-3)+'" y1="'+kingLineY.toFixed(1)+'" x2="'+(W-padR+2)+'" y2="'+kingLineY.toFixed(1)+'" stroke="'+PAL.gold+'" stroke-width="1.4"/>';
  svg+='<text x="'+(W-padR+5)+'" y="'+(kingLabelY+3).toFixed(1)+'" text-anchor="start" fill="'+PAL.gold+'" style="font-size:9px;font-weight:800;font-variant-numeric:tabular-nums">\uD83D\uDC51'+fmtNum(curK)+'</text>';
  // Price label (blue) at the RIGHT end: value + signed offset vs King, e.g. '777.1 (+1.2)'.
  if(pxRef!=null && priceLabelY!=null){
    // (v10.40) price PILL in the gutter at the price line's Y — never overdrawn.
    var pOp=pxStale?0.5:1;
    svg+='<line x1="'+(W-padR-3)+'" y1="'+priceLineY.toFixed(1)+'" x2="'+(W-padR+2)+'" y2="'+priceLineY.toFixed(1)+'" stroke="'+PAL.blue+'" stroke-width="1.4" opacity="'+pOp+'"/>';
    svg+='<rect x="'+(W-padR+3)+'" y="'+(priceLabelY-6.5).toFixed(1)+'" width="'+(padR-6)+'" height="13" rx="3" fill="rgba(74,144,217,0.12)" stroke="rgba(74,144,217,0.45)" opacity="'+pOp+'"/>';
    svg+='<text x="'+(W-padR+5)+'" y="'+(priceLabelY+3).toFixed(1)+'" text-anchor="start" fill="'+PAL.blue+'" style="font-size:9px;font-weight:700;font-variant-numeric:tabular-nums" opacity="'+pOp+'">'+fmtNum(pxRef)+'</text>';
    // signed offset vs King under the pill (kept from v10.27 — tested behavior)
    if(kingK!=null){ var offv=pxRef-kingK;
      svg+='<text x="'+(W-padR+5)+'" y="'+(priceLabelY+13.5).toFixed(1)+'" text-anchor="start" fill="'+PAL.sub+'" style="font-size:7.5px;font-variant-numeric:tabular-nums" opacity="'+pOp+'">('+(offv>=0?'+':'\u2212')+Math.abs(offv).toFixed(1)+')</text>'; }
  }
  svg+='</svg>';
  return { svg:svg, yLo:yLo, yHi:yHi, netDir:netDir, firstK:pts[0].k, lastK:pts[pts.length-1].k };
}
