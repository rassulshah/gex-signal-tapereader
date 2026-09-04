// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function levelsChartSvg(sym, L){
  try{
    var cs=[]; try{ cs=(closedCandles(sym)||[]).slice(-90); }catch(e){}
    var px=L.px;
    var rows=lvlRows(L); if(!rows.length) return '';
    var lo=null, hi=null;
    cs.forEach(function(c){ var a=(c.l!=null?c.l:c.c), b=(c.h!=null?c.h:c.c);
      if(a!=null){ lo=(lo==null||a<lo)?a:lo; } if(b!=null){ hi=(hi==null||b>hi)?b:hi; } });
    if(px!=null){ lo=(lo==null||px<lo)?px:lo; hi=(hi==null||px>hi)?px:hi; }
    rows.forEach(function(r){ lo=(lo==null||r.k<lo)?r.k:lo; hi=(hi==null||r.k>hi)?r.k:hi; });
    // Their walls sit far out by nature (SPX 7900 against 7642 spot). Including them in the range would
    // squash the whole session's price action into a few pixels, so the SCALE stays ours and any of their
    // levels outside it simply is not drawn — the row list still carries the number and its distance.

    if(lo==null||hi==null||!(hi>lo)) return '';
    var pad=(hi-lo)*0.06; lo-=pad; hi+=pad;
    // (v11.9, user) Labels sit ON the line, centred, at a legible size — a 7px tag in a right-hand gutter
    // was unreadable and the gutter cost chart width. Levels that coincide get their labels staggered
    // horizontally so one never hides another (CR and CR0 on the same strike is a normal case, not a rare one).
    var W=320, H=132, R=2, T=8, B=6;
    function y(v){ return T+(H-T-B)*(1-(v-lo)/(hi-lo)); }
    function x(i,n){ return (n<2)?0:((W-R)*i/(n-1)); }
    var placed=[];
    var svg='<svg viewBox="0 0 '+W+' '+H+'" style="width:100%;height:auto;display:block">';
    svg+='<rect x="0" y="0" width="'+W+'" height="'+H+'" fill="'+PAL.card+'"/>';
    // level lines first, so price draws over them
    rows.forEach(function(r){
      var yy=y(r.k), c=LVL_COL[r.id]||PAL.sub, z=(r.id==='cr0'||r.id==='ps0');
      svg+='<line x1="0" y1="'+yy.toFixed(1)+'" x2="'+(W-R)+'" y2="'+yy.toFixed(1)+'" stroke="'+c+'" stroke-width="'+(z?0.8:1.3)+'"'+(z?' stroke-dasharray="4,3"':'')+' opacity="'+(z?0.6:0.95)+'"/>';
      // stagger when this label would land on top of one already drawn
      var slot=0; for(var q=0;q<placed.length;q++){ if(Math.abs(placed[q].y-yy)<9) slot=Math.max(slot, placed[q].slot+1); }
      var lx=(W*0.5)+(slot%3)*46-46, nm=LVL_NAME[r.id];
      placed.push({y:yy, slot:slot});
      var wBox=nm.length*6.2+6;
      svg+='<rect x="'+(lx-wBox/2).toFixed(1)+'" y="'+(yy-6.2).toFixed(1)+'" width="'+wBox.toFixed(1)+'" height="12" rx="2" fill="'+PAL.card+'" opacity="0.88"/>';
      svg+='<text x="'+lx.toFixed(1)+'" y="'+(yy+3.4).toFixed(1)+'" fill="'+c+'" font-size="10" font-weight="700" text-anchor="middle" font-family="ui-monospace,monospace" opacity="'+(z?0.85:1)+'">'+nm+'</text>';
    });
    // (v11.12) THEIR levels, dotted and dimmer than anything of ours, label prefixed so the two can never
    // be read as one set. Drawn under the price line like ours.
    try{
      var IFC=ifManLevels();
      if(IFC && !IFC.err){
        [['CR',IFC.cw,LVL_COL.cr],['FLIP',IFC.zg,LVL_COL.hvl],['Mag',IFC.mag,LVL_COL.mag],['PS',IFC.pw,LVL_COL.ps]]
          .filter(function(r){ return r[1]!=null && r[1]>=lo && r[1]<=hi; })
          .forEach(function(r){
            var yy=y(r[1]);
            svg+='<line x1="0" y1="'+yy.toFixed(1)+'" x2="'+(W-R)+'" y2="'+yy.toFixed(1)+'" stroke="'+r[2]+'" stroke-width="0.7" stroke-dasharray="1,4" opacity="0.5"/>';
            var slot2=0; for(var q2=0;q2<placed.length;q2++){ if(Math.abs(placed[q2].y-yy)<9) slot2=Math.max(slot2, placed[q2].slot+1); }
            var lx2=(W*0.5)+(slot2%3)*46-46, nm2='IF·'+r[0];
            placed.push({y:yy, slot:slot2});
            svg+='<text x="'+lx2.toFixed(1)+'" y="'+(yy+3.2).toFixed(1)+'" fill="'+r[2]+'" font-size="8" font-style="italic" text-anchor="middle" font-family="ui-monospace,monospace" opacity="0.72">'+nm2+'</text>';
          });
      }
    }catch(eIFC){}
    // price
    if(cs.length>1){
      var pts=[]; cs.forEach(function(c,i){ if(c.c!=null) pts.push(x(i,cs.length).toFixed(1)+','+y(c.c).toFixed(1)); });
      if(pts.length>1) svg+='<polyline points="'+pts.join(' ')+'" fill="none" stroke="'+PAL.ink+'" stroke-width="1.2" opacity="0.95"/>';
    }
    if(px!=null){
      var yp=y(px).toFixed(1);
      svg+='<circle cx="'+(W-R-1)+'" cy="'+yp+'" r="1.8" fill="'+PAL.time+'"/>';
      svg+='<line x1="0" y1="'+yp+'" x2="'+(W-R)+'" y2="'+yp+'" stroke="'+PAL.time+'" stroke-width="0.5" stroke-dasharray="1,3" opacity="0.5"/>';
    }
    svg+='</svg>';
    return svg;
  }catch(e){ return ''; }
}
