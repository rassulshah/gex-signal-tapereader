// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function deflectionBlock(){
  var sym='SPY';
  var m=nodeMapModel(sym); if(!m||!m.ok) return '';
  var stats=deflStats(sym);
  var unlockN=deflUnlockN(stats.perDayCount);

  // collect LIVE confirmed deflections right now
  var rows=[];
  (m.levels||[]).forEach(function(L){
    var d=deflectionAt(sym, L.k); if(!d) return;
    var cls=classifyDeflection(sym, L, d.dir);
    var key=deflSetupKey(cls, d.dir);
    var b=stats.perKey[key]||{n:0,hit:0};
    rows.push({L:L, d:d, cls:cls, key:key, n:b.n, hit:b.hit, bars:d.bars});
  });
  // NEWEST-LEFT: fewest bars-since-tap = most recent deflection => leftmost.
  rows.sort(function(a,b){ if(a.bars!==b.bars) return a.bars-b.bars; return b.cls.prio-a.cls.prio; });

  // single-line header (no right-side unlock text)
  var html='<div style="display:flex;align-items:center;gap:6px;margin:2px 2px 4px">'+
    '<span title="A deflection = price taps a node then reverses away (\u2265'+DEFLECT_CONFIRM+' bars). Newest is leftmost. Grades appear once a setup has enough recorded outcomes." '+
      'style="font-size:11px;font-weight:800;color:'+PAL.ink+';letter-spacing:.2px">\u26a1 Deflections</span>'+
    '<span style="font-size:9px;color:'+PAL.sub+'">'+rows.length+' live</span></div>';

  if(!rows.length){
    html+='<div style="color:'+PAL.sub+';font-size:9px;padding:0 2px 3px">none</div>';
    return html;
  }

  // horizontal-scroll strip; direction:ltr so scrollLeft:0 shows the leftmost (newest) card.
  var cards='';
  rows.forEach(function(r){
    var up=(r.d.dir>0);
    var dirTxt=up?'\u25b2':'\u25bc';
    var dirCol=up?PAL.longAccent:PAL.shortAccent;
    // (v10.47) abbreviated card name "Defl · Gate" + Step-5 context chips at the moment of the event
    var star=/^\u2b51/.test(r.cls.name)?'\u2b51 ':'';
    var roleAb = r.L.isKing?'King':(r.L.isGatekeeper?'Gate':(r.L.isRugCeil||r.L.isRugFloor||r.L.isRugTarget)?((r.cls.name.indexOf('Reverse')>=0)?'RRug':'Rug'):(r.L.role==='Pika'?'Pika':(r.L.role==='Barn'?'Barn':(up?'Flr':'Ceil'))));
    var name=star+'Defl \u00b7 '+roleAb+(r.cls.brokeFT?' \u00b7 BO\u00b7FT':'')+(r.cls.isFBO?' \u00b7 FBO':'');
    var ctx=[];
    var stW=_nmAcc(r.L); if(stW!=='steady') ctx.push(stW.replace('accumulating','Acm').replace('decreasing','Dec').replace(/[()]/g,'')); else ctx.push('Steady');
    if(r.L.pos===true) ctx.push('+\u03B3'); else if(r.L.pos===false) ctx.push('\u2212\u03B3');
    var tp_=r.L.taps||0; if(tp_) ctx.push((tp_===1?'1st':tp_===2?'2nd':tp_===3?'3rd':tp_+'th')+' tap');
    var chipsHtml=ctx.map(function(t){ var red=/^(3rd|[4-9]th|\d\dth) tap$/.test(t); return '<span style="font-size:8px;border:1px solid '+(red?PAL.shortAccent:PAL.line)+';color:'+(red?PAL.shortAccent:PAL.sub)+';border-radius:9px;padding:0 4px">'+t+'</span>'; }).join(' ');
    // grade: HIDDEN until unlock; else earned letter
    var gradeHtml;
    if(r.n>=unlockN){
      var rate=Math.round(100*r.hit/r.n);
      var g=deflGrade(rate);
      gradeHtml='<span style="font-size:14px;font-weight:800;color:'+g.col+'">'+g.g+'</span> <span style="font-size:8.5px;color:'+PAL.sub+'">'+rate+'% n'+r.n+'</span>';
    } else {
      gradeHtml='<span style="font-size:8.5px;font-weight:700;color:'+PAL.sub+';border:1px dashed '+PAL.sub+';border-radius:999px;padding:0 5px">\u25cf rec n'+r.n+'</span>';
    }
    cards+='<div style="flex:0 0 auto;min-width:120px;background:'+PAL.card+';border:1px solid '+dirCol+';border-radius:8px;padding:5px 8px;scroll-snap-align:start">'+
      '<div style="display:flex;align-items:baseline;gap:5px"><span style="font-size:14px;font-weight:800;color:'+PAL.ink+';font-variant-numeric:tabular-nums">'+fmtNum(r.L.k)+'</span>'+
        '<span style="font-size:10px;color:'+dirCol+';font-weight:800">'+dirTxt+'</span></div>'+
      '<div style="font-size:10px;font-weight:700;color:'+PAL.ink+';white-space:nowrap;margin:1px 0 3px">'+name+'</div>'+
      '<div style="display:flex;gap:3px;white-space:nowrap;margin-bottom:3px">'+chipsHtml+'</div>'+
      '<div>'+gradeHtml+'</div>'+
    '</div>';
  });
  html+='<div style="display:flex;gap:8px;overflow-x:auto;overflow-y:hidden;padding:0 2px 6px;scroll-snap-type:x proximity;direction:ltr">'+cards+'</div>';
  return html;
}
