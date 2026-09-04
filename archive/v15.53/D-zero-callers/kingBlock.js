// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function kingBlock(){
  var sym='SPY';
  var tp=tapeMap(sym);
  var kingK = (tp && typeof tp.king==='number') ? tp.king : null;
  var px = STATE[sym] ? STATE[sym].price : null;
  var kd=kingDay(sym);
  var mv = (kd && kd.moves) ? kd.moves : [];
  var moveCount = kd ? (kd.count||0) : 0;
  var now=Date.now();

  // ---- Stacked King/price badge (v10.15): one pill, King strike on top with a
  // signed offset vs. price, SPY price below. Replaces the old 3-badge cluster
  // (gold King price + distance chip + net-drift chip). Offset = King - price
  // rounded; color: King above price = resistance (red), below = support
  // (green), equal = gold/neutral \u2014 same convention as the sparkline dots.
  // (v10.19) King header redesign: NO "King" title text, badge CENTERED, crown
  // INSIDE the badge next to the King price. Offset shown as an ARROW only
  // (\u2191 King above price = green, \u2193 below = red), NOTHING when King == price.
  // Color fix: above=green (magnet pulls up), below=red (was inverted before).
  var offRaw = (px!=null && kingK!=null) ? (kingK - px) : null;
  var off = (offRaw!=null) ? Math.round(offRaw) : null;
  // (v10.37) King badge REDESIGN: two stacked rows inside ONE gold pill.
  //   TOP  = 👑 crown + King strike + signed offset-vs-price arrow ('−3↓' etc.)
  //   BELOW = white gate icon + GATEKEEPER strike + signed distance FROM PRICE
  //           (was SPY price). No gatekeeper => a dimmed placeholder row (empty space).
  // The standalone Gatekeeper section is removed — its price now lives here.
  function signedOff(delta){                 // delta = level - price (rounded)
    if(delta==null) return null;
    var d=Math.round(delta);
    if(d===0) return {sign:'', num:'0', arr:'', col:PAL.gold};
    var above=d>0;
    return { sign:above?'+':'\u2212', num:String(Math.abs(d)), arr:above?'\u2191':'\u2193',
             col:above?PAL.longAccent:PAL.shortAccent };
  }
  var gkNow = (typeof gatekeeper==='function') ? (function(){ try{return gatekeeper(sym);}catch(e){return null;} })() : null;
  var gkK = (gkNow && gkNow.ok && typeof gkNow.k==='number') ? gkNow.k : null;
  var kingPxBadge='';
  if(kingK!=null){
    var ko=signedOff(offRaw);
    var kArr = (ko && ko.arr) ? ('<span title="King is '+ko.num+' vs price (+ above, \u2212 below)." style="color:'+ko.col+';font-weight:800;font-size:11px;line-height:1;font-variant-numeric:tabular-nums;margin-left:3px">'+ko.sign+ko.num+ko.arr+'</span>') : '';
    // bottom row: gatekeeper strike + signed distance from PRICE
    var gkRow;
    if(gkK!=null){
      var go=signedOff((px!=null)?(gkK-px):null);
      var gArr = (go && go.arr) ? ('<span title="Gatekeeper is '+go.num+' vs price (+ above, \u2212 below)." style="color:'+go.col+';font-weight:800;font-size:10px;line-height:1;font-variant-numeric:tabular-nums;margin-left:3px">'+go.sign+go.num+go.arr+'</span>') : '';
      gkRow='<span style="display:inline-flex;align-items:center;gap:3px;line-height:1">'+gateSvg(PAL.ink)+
        '<span style="color:'+PAL.ink+';font-weight:700;font-size:10px;font-variant-numeric:tabular-nums">'+fmtNum(gkK)+'</span>'+gArr+'</span>';
    } else {
      // no gatekeeper => keep the row height, dim placeholder (clear path)
      gkRow='<span title="No gatekeeper \u2014 clear path to the King." style="display:inline-flex;align-items:center;gap:3px;line-height:1;opacity:.45">'+gateSvg(PAL.sub)+
        '<span style="color:'+PAL.sub+';font-size:10px">\u2013</span></span>';
    }
    kingPxBadge=
      '<span title="TOP: 👑 King strike '+fmtNum(kingK)+' + offset vs price. BELOW: gatekeeper strike + its distance from price (no gatekeeper => clear path)." '+
        'style="display:inline-flex;flex-direction:column;align-items:center;gap:1px;padding:3px 8px;border:1.5px solid '+PAL.gold+';border-radius:14px;background:'+PAL.card+';flex:0 1 auto;min-width:0">'+
        '<span style="display:inline-flex;align-items:center;line-height:1">'+
          '<span style="font-size:12px;line-height:1">\uD83D\uDC51</span>'+
          '<span style="color:'+PAL.gold+';font-weight:800;font-size:13px;font-variant-numeric:tabular-nums;margin-left:3px">'+fmtNum(kingK)+'</span>'+
          kArr+
        '</span>'+
        '<span style="width:100%;height:1px;background:'+PAL.line+';margin:1px 0"></span>'+
        gkRow+
      '</span>';
  } else {
    kingPxBadge='<span style="color:'+PAL.sub+';font-size:11px">Waiting on tape\u2026</span>';
  }
  // ---- Header (v10.25): centered 3-MAGNET CLUSTER — green ★ strongest SUPPORT (left),
  // 👑 King (middle), red ★ strongest RESISTANCE (right). Trend badge REMOVED (the
  // regime/instruction line in the Node Map carries trend now). Below: the ①②③ icons.
  var nm = (typeof nodeMapModel==='function') ? (function(){ try{return nodeMapModel(sym);}catch(e){return null;} })() : null;
  function sideMagnet(node, isSup){
    var col = isSup?PAL.longAccent:PAL.shortAccent;
    var lbl = isSup?'\u2605 SUP':'\u2605 RES';
    if(!node){ return '<span style="display:inline-flex;flex-direction:column;align-items:center;justify-content:center;padding:3px 6px;min-height:40px;box-sizing:border-box;border-radius:14px;background:'+PAL.card+';border:1.5px solid '+PAL.line+';opacity:.6;flex:0 1 auto;min-width:0"><span style="font-size:7.5px;font-weight:800;line-height:1;color:'+PAL.sub+';letter-spacing:.3px">'+lbl+'</span><span style="width:100%;height:1px;background:'+PAL.line+';margin:2px 0"></span><span style="color:'+PAL.sub+';font-size:10px;line-height:1">\u2013</span></span>'; }
    var tip=(isSup?'Strongest SUPPORT magnet below price':'Strongest RESISTANCE magnet above price')+' \u2014 '+fmtNum(node.k)+' at '+node.pct+'% of King (size+build-rate+nearness blend). A strong magnet attracts price toward it.';
    return '<span title="'+tip.replace(/"/g,'')+'" style="display:inline-flex;flex-direction:column;align-items:center;justify-content:center;padding:3px 6px;min-height:40px;box-sizing:border-box;border-radius:14px;background:'+PAL.card+';border:1.5px solid '+col+';flex:0 1 auto;min-width:0">'+
      '<span style="font-size:7.5px;font-weight:800;line-height:1;color:'+col+';letter-spacing:.3px">'+lbl+'</span>'+
      '<span style="width:100%;height:1px;background:'+PAL.line+';margin:2px 0"></span>'+
      '<span style="line-height:1;white-space:nowrap"><span style="font-weight:800;font-size:12.5px;color:'+col+';font-variant-numeric:tabular-nums">'+fmtNum(node.k)+'</span> <span style="color:'+PAL.sub+';font-size:8.5px;font-weight:600;font-variant-numeric:tabular-nums">'+node.pct+'%</span></span>'+
    '</span>';
  }
  var supBadge = sideMagnet(nm?nm.strongSup:null, true);
  var resBadge = sideMagnet(nm?nm.strongRes:null, false);
  var html='<div style="display:flex;justify-content:space-between;align-items:center;gap:4px;margin:4px 0 3px 0;width:100%;box-sizing:border-box;padding:0 2px">'+
    supBadge+
    '<span title="KING: the strike with the largest absolute dealer exposure \u2014 the day\u2019s EOD settlement magnet." style="display:inline-flex;flex:0 1 auto;min-width:0">'+kingPxBadge+'</span>'+
    resBadge+
  '</div>'+
  // ①②③ info icons (Magnets / King / Range \u2014 the header trio)
  '<div style="display:flex;gap:5px;align-items:center;justify-content:center;margin:0 0 5px">'+
    '<span style="font-size:8.5px;color:'+PAL.sub+';font-weight:700;letter-spacing:.3px;margin-right:2px">5-STEP</span>'+
    stepIcon(1)+stepIcon(2)+stepIcon(3)+
  '</div>';
  if(kingK==null){
    html+='<div style="color:'+PAL.sub+';padding:2px 6px;font-size:11px">Waiting on tape\u2026</div>';
    return html;
  }

  // ---- Derived: pinned time still used for the CURRENT chip's hover only ----
  var lastRollT = mv.length ? mv[mv.length-1].t : null;
  var pinnedMs = (lastRollT!=null) ? (now-lastRollT) : null;
  function mins(ms){ return ms==null?null:Math.max(0,Math.round(ms/60000)); }
  var pinnedM = mins(pinnedMs);
  var pinnedTxt = (pinnedM==null)?'\u2013':(pinnedM>=60?((pinnedM/60).toFixed(1)+'h'):(pinnedM+'m'));

  // ---- KING PATH SPARKLINE (v10.8): stepped King-strike-vs-clock-time chart. ----
  // Replaces the old chip row. The staircase's SHAPE is the day's King trend:
  // climbing = bullish migration, descending = bearish, flat = pinned, sawtooth
  // = chop. X is real time (a 2h hold is a long flat left segment), so pace is
  // honest. Faint dashed line = current price; gold dot = current King.
  var sess=sessionBoundsCT();
  // ---- KING VERDICT (v10.10): bull/bear read from drift + magnet + stability ----
  var kv = kingVerdict(mv, kingK, px, now);
  var vCol = kv.cls==='bull'?PAL.longAccent : (kv.cls==='bear'?PAL.shortAccent : PAL.gold);
  var spark=kingSparkline(mv, kingK, px, now, sess, vCol);
  // Session-drift summary words for the title.
  var driftWord = spark.netDir>0?'up':(spark.netDir<0?'down':'flat');
  var driftCol  = spark.netDir>0?PAL.longAccent:(spark.netDir<0?PAL.shortAccent:PAL.sub);
  var nRolls = moveCount;
  var loStrike = fmtNum(Math.ceil(spark.yLo));
  var hiStrike = fmtNum(Math.floor(spark.yHi));
  // ---- Magnet-target read: the single most actionable King output. ----
  var magWord = kv.magnet>0 ? 'up' : (kv.magnet<0?'down':'');
  var magStr = Math.round(Math.abs(kv.gap));
  var magnetRead = kv.atMagnet
    ? ('Price is AT the King \u2014 pinned; expect it to act as a magnet/range center until the King rolls.')
    : ('King magnet pulls price '+magWord+' toward '+fmtNum(kingK)+' ('+magStr+' strike'+(magStr===1?'':'s')+' '+(kv.magnet>0?'above':'below')+').');
  // Verdict tooltip: spell out how drift + magnet + stability combined.
  var vWhy = (kv.word+' \u00b7 '+kv.regime+'. '+
    'Drift '+(kv.drift>0?'+':'')+kv.drift+' strike'+(Math.abs(kv.drift)===1?'':'s')+' ('+(kv.drift>0?'dealers migrating higher = bullish':kv.drift<0?'dealers migrating lower = bearish':'no migration')+'); '+
    'magnet '+(kv.magnet>0?'above price (pull up)':kv.magnet<0?'below price (pull down)':'at price (no pull)')+'; '+
    (kv.regime==='trending'?'actively rolling ('+kv.rolls60+' in 60m) = momentum regime, go WITH the drift':kv.regime==='range-pinned'?'pinned '+(kv.pinnedM||0)+'m with no drift = range regime, fade the extremes toward the King':'settling')+'.'+
    (kv.mixed?' NOTE: drift and magnet disagree \u2014 migration says one way while the immediate magnet pulls the other; treat as mixed until they align.':'')).replace(/"/g,'');
  var sparkTip=('King path today \u2014 strike vs. clock time across the cash session (8:30\u201315:00 CT). '+
    'Staircase steps at each confirmed roll, holds flat between, so a long flat = pinned and a tight cluster = rapid rolling. '+
    'Net drift '+fmtNum(spark.firstK)+' \u2192 '+fmtNum(spark.lastK)+' ('+driftWord+', '+nRolls+' roll'+(nRolls===1?'':'s')+'). '+
    'Line color = King verdict (green bullish, red bearish, yellow neutral/flat). Gold dot = current King; dashed = current price. '+vWhy).replace(/"/g,'');
  html+='<div id="gpts-kingpath" title="'+sparkTip+'" style="padding:5px 8px;background:'+PAL.card+';border:1px solid '+PAL.line+';border-radius:8px;margin:2px 0">'+
    // (v10.40) NARRATIVE FIRST — replaces the old "KING PATH · today · drift ·
    // verdict" header row AND the old bottom magnet-read line. Drift/rolls moved
    // INSIDE the chart (overlay, descriptive-only: tested 50% n=68).
    kingReadHtml(kingAnalyzer(sym), kv)+
    '<div style="display:flex;align-items:stretch;gap:4px">'+
      '<div style="flex:1;min-width:0;position:relative">'+spark.svg+
        '<span title="Y-axis extent of today\'s King path (strikes)." style="position:absolute;top:14px;left:5px;font-size:7.5px;color:'+PAL.sub+'">'+hiStrike+'</span>'+
        '<span style="position:absolute;bottom:14px;left:5px;font-size:7.5px;color:'+PAL.sub+'">'+loStrike+'</span>'+
        '<span id="gpts-kp-drift" title="Session King migration — DESCRIPTIVE ONLY: 3-bar drift tested 50% vs next-30m direction (n=68). Not a prediction." '+
          'style="position:absolute;top:3px;left:5px;font-size:8px;font-weight:700;color:'+driftCol+';background:rgba(11,14,20,.72);border-radius:4px;padding:0 4px">drift '+driftWord+' · '+nRolls+' roll'+(nRolls===1?'':'s')+'</span>'+
        '<span title="'+vWhy+'" style="position:absolute;top:3px;right:5px;font-size:8.5px;font-weight:800;color:'+vCol+';background:rgba(11,14,20,.72);border:1px solid '+vCol+';border-radius:20px;padding:0 6px">'+kv.word+'</span>'+
        '<span style="position:absolute;bottom:3px;left:5px;font-size:7.5px;color:'+PAL.sub+';background:rgba(11,14,20,.72);border-radius:4px;padding:0 4px">'+fmtClock(spark.firstK!=null && mv.length?mv[0].t:sess.start)+' — now · pinned '+pinnedTxt+'</span>'+
      '</div>'+
    '</div>'+
  '</div>';
  // (v10.42) PROJECTED path chart below the history chart.
  try{ var __A2=kingAnalyzer(sym); html+=projChartHtml(__A2, kingProjectionLive(sym,__A2)); }catch(ePJ){}'</div>';
  return html;
}
