// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function driftLineHtml(sym){
  sym=sym||'SPY';
  var d;
  try{ d=driftRead(sym); }catch(e){ return ''; }
  if(!d) return '';
  var up=(d.dir>0), dn=(d.dir<0);
  var arrow = up?'↗':(dn?'↘':'→');
  var acol  = up?PAL.longAccent:(dn?PAL.shortAccent:PAL.sub);
  var chipCol = (d.verdict==='NONE')?PAL.sub:(up?PAL.longAccent:(dn?PAL.shortAccent:PAL.amber));
  var chipBg  = (d.verdict==='NONE')?'transparent':(up?'rgba(46,194,126,.14)':(dn?'rgba(240,97,109,.14)':'rgba(242,180,90,.14)'));
  // (v10.50) SIMPLE, question-first hover — no confidence clause.
  var gA=(d.gvwap!=null && d.px!=null && d.gvwap>d.px), gB=(d.gvwap!=null && d.px!=null && d.gvwap<d.px);
  var vA=(d.vvwap!=null && d.px!=null && d.vvwap>d.px), vB=(d.vvwap!=null && d.px!=null && d.vvwap<d.px);
  var lead='Which way do GEX & VEX lean? ';
  var tip;
  if(d.verdict==='NONE' || d.vvwap==null) tip=lead+'Waiting on VEX.';
  else if(gA && vA) tip=lead+'Both above price — supporting higher prices.';
  else if(gB && vB) tip=lead+'Both below price — pressuring lower prices.';
  else tip=lead+'They disagree, no clean lean, expect chop.';
  tip=tip.replace(/"/g,'');
  var line='<div title="'+tip+'" style="display:flex;align-items:center;gap:5px;white-space:nowrap;font-size:10px;margin:5px 0 3px;overflow:hidden">'+
    '<span style="color:'+acol+';font-weight:800">'+arrow+'</span>'+
    '<span style="color:'+PAL.sub+';font-weight:700">Drift</span>'+
    '<span style="padding:0 5px;border-radius:3px;font-size:9px;font-weight:700;color:'+chipCol+';background:'+chipBg+'">'+d.label+'</span>'+
    '<span style="color:'+PAL.line+'">·</span>'+
    // (v10.56 PART E) EACH CENTRE SAYS WHICH SIDE OF PRICE IT IS ON. "G768.7" alone made
    // the reader hold the price in their head to know whether gamma was pulling up or
    // down; the arrow answers it on the face — ↓ below price (red), ↑ above (green).
    '<span style="color:'+PAL.gold+';font-weight:700">G'+(d.gvwap!=null?fmtLvl(d.gvwap):'–')+driftSideArrow(d.gvwap,d.px)+'</span>'+
    '<span style="color:'+PAL.line+'">·</span>'+
    '<span style="color:#a371f7;font-weight:700">V'+(d.vvwap!=null?fmtLvl(d.vvwap):'–')+driftSideArrow(d.vvwap,d.px)+'</span>'+
  '</div>';
  // (v10.51.1 FIX) THIN BAR — TWO LANES: gold GEX band on top, purple VEX band below,
  // each with a brighter centre tick (GVWAP / VVWAP), and a WHITE price line spanning both.
  // SCALE BUG (v10.50): the domain was the Flr..Ceil range, which does NOT contain the ±1σ
  // bands — on 2026-08-18 that clamped VEX to 0-100% and GEX to 66%, so the two smeared into
  // one wash and only the price tick was legible. The domain is now the UNION of both bands
  // and price (padded), so the bands always fit and their true positions are comparable.
  // Separate lanes (not overlaid at 50% opacity) keep both readable when they overlap.
  var bar='';
  try{
    var xs=[d.px,d.gLo,d.gHi,d.vLo,d.vHi].filter(function(x){return typeof x==='number';});
    if(xs.length>=2){
      var lo=Math.min.apply(null,xs), hi=Math.max.apply(null,xs);
      var pad=Math.max(0.15,(hi-lo)*0.08);
      lo-=pad; hi+=pad;
      var span=hi-lo;
      var P=function(x){ return Math.max(0,Math.min(100,(x-lo)/span*100)); };
      function laneHtml(bLo,bHi,ctr,col,top,label){
        if(bLo==null||bHi==null) return '';
        var l=P(bLo), w=Math.max(2,P(bHi)-l);
        var h='<div title="'+label+'" style="position:absolute;top:'+top+'px;height:3px;border-radius:2px;opacity:.55;background:'+col+';left:'+l.toFixed(1)+'%;width:'+w.toFixed(1)+'%"></div>';
        if(ctr!=null) h+='<div style="position:absolute;top:'+top+'px;height:3px;width:2px;border-radius:1px;background:'+col+';left:'+P(ctr).toFixed(1)+'%"></div>';
        return h;
      }
      var lanes=laneHtml(d.gLo,d.gHi,d.gvwap,PAL.gold,1,'GEX (gamma) band '+(d.gLo!=null?d.gLo+'-'+d.gHi:'')+', centre '+(d.gvwap!=null?d.gvwap:'-'))
               +laneHtml(d.vLo,d.vHi,d.vvwap,'#a371f7',6,'VEX (vanna) band '+(d.vLo!=null?d.vLo+'-'+d.vHi:'')+', centre '+(d.vvwap!=null?d.vvwap:'-'));
      var pxl=(d.px!=null)?('<div title="price '+d.px+'" style="position:absolute;top:-1px;height:12px;width:2px;background:#ffffff;border-radius:1px;left:'+P(d.px).toFixed(1)+'%"></div>'):'';
      bar='<div title="'+tip+'" style="position:relative;height:10px;border-radius:3px;background:#161b22;margin:0 2px 6px">'+lanes+pxl+'</div>';
    }
  }catch(eBar){}
  return line+bar;
}
