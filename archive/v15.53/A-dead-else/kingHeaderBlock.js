// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function kingHeaderBlock(){
  var sym=activeSym();   // (v10.55 PART G) keyed by sym like everything else
  var tp=tapeMap(sym);
  var kingK = (tp && typeof tp.king==='number') ? tp.king : null;
  var px = STATE[sym] ? STATE[sym].price : null;
  // (v10.47b) fall back to the model's King (S.king / last-known) so the pill never says "Waiting on tape" while the ladder shows a King
  if(kingK==null){ try{ var _m0=nodeMapModel(sym); if(_m0 && _m0.kingK!=null) kingK=_m0.kingK; }catch(e0){} }
  // (v10.50) step icons \u2460\u2461\u2462 RETIRED from the header pills (their doctrine folded into
  // the King / SUP / RES hovers). stepMini kept as a no-op so nothing downstream breaks.
  var stepMini=function(n){ return ''; };
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
        '<span style="color:'+PAL.ink+';font-weight:700;font-size:10px;font-variant-numeric:tabular-nums">'+fmtLvl(gkK)+'</span>'+gArr+'</span>';
    } else {
      // no gatekeeper => keep the row height, dim placeholder (clear path)
      gkRow='<span title="No gatekeeper \u2014 clear path to the King." style="display:inline-flex;align-items:center;gap:3px;line-height:1;opacity:.45">'+gateSvg(PAL.sub)+
        '<span style="color:'+PAL.sub+';font-size:10px">\u2013</span></span>';
    }
    kingPxBadge=
      '<span title="TOP: 👑 King strike '+fmtLvl(kingK)+' + offset vs price. BELOW: gatekeeper strike + its distance from price (no gatekeeper => clear path)." '+
        'style="display:inline-flex;flex-direction:column;align-items:center;gap:1px;padding:3px 8px;border:1.5px solid '+PAL.gold+';border-radius:14px;background:'+PAL.card+';flex:0 1 auto;min-width:0">'+
        '<span style="display:inline-flex;align-items:center;line-height:1">'+
          '<span style="font-size:12px;line-height:1">\uD83D\uDC51</span>'+
          '<span style="color:'+PAL.gold+';font-weight:800;font-size:13px;font-variant-numeric:tabular-nums;margin-left:3px">'+fmtLvl(kingK)+'</span>'+
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
    var lbl = (isSup?'\u2605 SUP':'\u2605 RES');
    if(!node){ return '<span style="display:inline-flex;flex-direction:column;align-items:center;justify-content:center;padding:3px 6px;min-height:40px;box-sizing:border-box;border-radius:14px;background:'+PAL.card+';border:1.5px solid '+PAL.line+';opacity:.6;flex:0 1 auto;min-width:0"><span style="font-size:7.5px;font-weight:800;line-height:1;color:'+PAL.sub+';letter-spacing:.3px">'+lbl+'</span><span style="width:100%;height:1px;background:'+PAL.line+';margin:2px 0"></span><span style="color:'+PAL.sub+';font-size:10px;line-height:1">\u2013</span></span>'; }
    var tip=(isSup?'Strongest SUPPORT magnet below price':'Strongest RESISTANCE magnet above price')+' \u2014 '+fmtLvl(node.k)+' at '+node.pct+'% of King (size+build-rate+nearness blend). A strong magnet attracts price toward it.';
    return '<span title="'+tip.replace(/"/g,'')+'" style="display:inline-flex;flex-direction:column;align-items:center;justify-content:center;padding:3px 6px;min-height:40px;box-sizing:border-box;border-radius:14px;background:'+PAL.card+';border:1.5px solid '+col+';flex:0 1 auto;min-width:0">'+
      '<span style="font-size:7.5px;font-weight:800;line-height:1;color:'+col+';letter-spacing:.3px">'+lbl+'</span>'+
      '<span style="width:100%;height:1px;background:'+PAL.line+';margin:2px 0"></span>'+
      '<span style="line-height:1;white-space:nowrap"><span style="font-weight:800;font-size:12.5px;color:'+col+';font-variant-numeric:tabular-nums">'+fmtLvl(node.k)+'</span> <span style="color:'+PAL.sub+';font-size:8.5px;font-weight:600;font-variant-numeric:tabular-nums">'+node.pct+'%</span></span>'+
    '</span>';
  }
  var supBadge = sideMagnet(nm?nm.strongSup:null, true);
  var resBadge = sideMagnet(nm?nm.strongRes:null, false);
  // (v10.51.2) STEPS 1-5 restored as a tiny clickable line ABOVE the cluster. Each numeral
  // opens its OWN popover (Skylit's 5-step method) via the existing .gs-ico/data-gstep
  // delegation in wireStepIcons(). Deliberately its own row so the SUP/King/RES pill
  // alignment below is untouched. v10.50 had retired these; the doctrine text (STEP_TEXT)
  // was still in the file but had no caller, i.e. unreachable.
  var stepsLine=(function(){
    var STEP_LBL={1:'Magnets',2:'King',3:'Range',4:'Gatekeepers',5:'Flow'};
    // (v10.56 PART E) CENTRED. It sits above a centred three-badge cluster; left-aligned
    // it read as a stray label rather than the header of what is under it.
    var s='<div style="display:flex;align-items:center;justify-content:center;text-align:center;gap:4px;font-size:8px;line-height:1;color:'+PAL.sub+';margin:1px 0 2px;white-space:nowrap">'+
          '<span style="font-weight:700;letter-spacing:.3px">Steps</span>';
    for(var n=1;n<=5;n++){
      s+='<span class="gs-ico" data-gstep="'+n+'" title="Step '+n+' \u2014 '+STEP_LBL[n]+' (click for the method)" '+
         'style="cursor:pointer;font-size:8px;font-weight:800;color:'+PAL.blue+';padding:0 1px;border-bottom:1px dotted '+PAL.blue+'">'+n+'</span>';
    }
    return s+'</div>';
  })();
  var html=stepsLine+
  '<div style="display:flex;justify-content:space-between;align-items:center;gap:4px;margin:4px 0 3px 0;width:100%;box-sizing:border-box;padding:0 2px">'+
    supBadge+
    '<span title="KING: the strike with the largest absolute dealer exposure \u2014 the day\u2019s EOD settlement magnet." style="display:inline-flex;flex:0 1 auto;min-width:0">'+kingPxBadge+'</span>'+
    resBadge+
  '</div>';
  return html;
}
