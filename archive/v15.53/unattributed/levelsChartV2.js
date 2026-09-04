// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function levelsChartV2(sym, U){
  try{
    var cs=[]; try{ cs=(closedCandles(sym)||[]).slice(-90); }catch(e){}
    var px=U.px, lo=null, hi=null;
    cs.forEach(function(c){ var a=(c.l!=null?c.l:c.c), b=(c.h!=null?c.h:c.c);
      if(a!=null){ lo=(lo==null||a<lo)?a:lo; } if(b!=null){ hi=(hi==null||b>hi)?b:hi; } });
    if(px!=null){ lo=(lo==null||px<lo)?px:lo; hi=(hi==null||px>hi)?px:hi; }
    // (v11.19) THE PRICE ACTION OWNS THE SCALE. Letting every level stretch the range is what flattened
    // the live chart: a single wall 30 points above everything pulled the top out, and the price line plus
    // every real level collapsed into the bottom fifth. The range grows for a level only up to a bounded
    // multiple of the price range; anything beyond that is drawn as an edge marker instead, so it is still
    // visible and still labelled, but it can never squash what actually matters.
    var pxLo=lo, pxHi=hi;
    if(pxLo==null||pxHi==null) return '';
    var span=Math.max(pxHi-pxLo, (px!=null?px*0.0015:0.5));
    var maxLo=pxLo-span*1.1, maxHi=pxHi+span*1.1;
    var outside=[];
    U.rows.forEach(function(r){
      if(r.k<maxLo || r.k>maxHi){ outside.push({ id:r.id, k:r.k, above:(r.k>maxHi) }); return; }
      lo=(lo==null||r.k<lo)?r.k:lo; hi=(hi==null||r.k>hi)?r.k:hi; });
    if(lo==null||hi==null||!(hi>lo)) return '';
    var pad=(hi-lo)*0.10; lo-=pad; hi+=pad;
    var W=320,H=132,R=2,T=8,B=6, placed=[];
    function y(v){ return T+(H-T-B)*(1-(v-lo)/(hi-lo)); }
    var svg='<svg viewBox="0 0 '+W+' '+H+'" style="width:100%;height:auto;display:block">';
    svg+='<rect width="'+W+'" height="'+H+'" fill="'+PAL.card+'"/>';
    U.rows.forEach(function(r){
      if(r.k<lo || r.k>hi) return;                     // drawn as an edge marker below instead
      var base=r.id.split('·')[0].replace('0','').toLowerCase();
      var c=(r.id.indexOf('·')>0)?PAL.gold:(LVL_COL[base]||PAL.sub), z=!!r.tag, yy=y(r.k);
      svg+='<line x1="0" y1="'+yy.toFixed(1)+'" x2="'+(W-R)+'" y2="'+yy.toFixed(1)+'" stroke="'+c+'" stroke-width="'+(z?0.8:1.3)+'"'+(z?' stroke-dasharray="4,3"':'')+' opacity="'+(z?0.6:0.95)+'"/>';
      var slot=0; for(var q=0;q<placed.length;q++){ if(Math.abs(placed[q].y-yy)<9) slot=Math.max(slot, placed[q].slot+1); }
      var lx=(W*0.5)+(slot%3)*48-48; placed.push({y:yy, slot:slot});
      var w=r.id.length*6.4+6;
      svg+='<rect x="'+(lx-w/2).toFixed(1)+'" y="'+(yy-6.2).toFixed(1)+'" width="'+w.toFixed(1)+'" height="12" rx="2" fill="'+PAL.card+'" opacity="0.88"/>';
      svg+='<text x="'+lx.toFixed(1)+'" y="'+(yy+3.4).toFixed(1)+'" fill="'+c+'" font-size="10" font-weight="700" text-anchor="middle" font-family="ui-monospace,monospace" opacity="'+(z?0.85:1)+'">'+r.id+'</text>';
    });
    if(cs.length>1){
      var pts=[]; cs.forEach(function(c,i){ if(c.c!=null) pts.push(((W-R)*i/(cs.length-1)).toFixed(1)+','+y(c.c).toFixed(1)); });
      if(pts.length>1) svg+='<polyline points="'+pts.join(' ')+'" fill="none" stroke="'+PAL.ink+'" stroke-width="1.2" opacity="0.95"/>';
    }
    // edge markers for whatever sits outside the price-driven range
    outside.forEach(function(o, oi){
      var base=o.id.split('·')[0].replace('0','').toLowerCase();
      var c=(o.id.indexOf('·')>0)?PAL.gold:(LVL_COL[base]||PAL.sub);
      var yy=o.above?(T+3+oi*11):(H-B-3-oi*11);
      var lbl=(o.above?'▲ ':'▼ ')+o.id+' '+lvlFmt(o.k);
      var w=lbl.length*5.6+6;
      svg+='<rect x="'+(W-R-w-2).toFixed(1)+'" y="'+(yy-6).toFixed(1)+'" width="'+w.toFixed(1)+'" height="11" rx="2" fill="'+PAL.card+'" opacity="0.9"/>';
      svg+='<text x="'+(W-R-4).toFixed(1)+'" y="'+(yy+2.5).toFixed(1)+'" fill="'+c+'" font-size="8.5" font-weight="700" text-anchor="end" font-family="ui-monospace,monospace" opacity="0.85">'+lbl+'</text>';
    });
    // (v11.25) THEIR chain levels — dotted, italic, prefixed. A different measurement, so it must be
    // impossible to confuse with ours at a glance. Out-of-range ones join the edge markers.
    try{
      var CHc=ifChainRows(sym,'toFri');
      if(CHc && CHc.rows.length){
        CHc.rows.forEach(function(r){
          var base=(r.id==='MP'||r.id==='MP*')?'mag':r.id.toLowerCase();
          var c=(r.id==='MP'||r.id==='MP*')?PAL.sub:(LVL_COL[base]||PAL.sub);
          if(r.k<lo || r.k>hi){ outside.push({ id:'IF·'+r.id, k:r.k, above:(r.k>hi) }); return; }
          var yy=y(r.k);
          svg+='<line x1="0" y1="'+yy.toFixed(1)+'" x2="'+(W-R)+'" y2="'+yy.toFixed(1)+'" stroke="'+c+'" stroke-width="0.8" stroke-dasharray="1,4" opacity="0.55"/>';
          var slot3=0; for(var q3=0;q3<placed.length;q3++){ if(Math.abs(placed[q3].y-yy)<9) slot3=Math.max(slot3, placed[q3].slot+1); }
          var lx3=(W*0.5)+(slot3%3)*48-48; placed.push({y:yy, slot:slot3});
          svg+='<text x="'+lx3.toFixed(1)+'" y="'+(yy+3.2).toFixed(1)+'" fill="'+c+'" font-size="8.5" font-style="italic" text-anchor="middle" font-family="ui-monospace,monospace" opacity="0.8">IF·'+r.id+'</text>';
        });
      }
    }catch(eCHc){}
    if(px!=null){ var yp=y(px).toFixed(1);
      svg+='<line x1="0" y1="'+yp+'" x2="'+(W-R)+'" y2="'+yp+'" stroke="'+PAL.time+'" stroke-width="0.5" stroke-dasharray="1,3" opacity="0.5"/>';
      svg+='<circle cx="'+(W-R-2)+'" cy="'+yp+'" r="1.8" fill="'+PAL.time+'"/>'; }
    return svg+'</svg>';
  }catch(e){ return ''; }
}
