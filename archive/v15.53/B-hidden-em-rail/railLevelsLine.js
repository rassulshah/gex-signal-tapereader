// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function railLevelsLine(EB, RB, RAILPS, SESSL, sym){
  try{
    if(!EB || !EB.ok) return '';
    var frNow=(typeof EB.nowLive==='number')?EB.nowLive:EB.now;
    var LV=[];
    if(SESSL){
      // ⚠ (v15.31) IBH/IBL ARE OUT OF THE LEVEL COLUMN — operator, 2026-09-01: "get rid of the IBH
      // and IBL from the level column. it is not a key level."
      // They are still MEASURED (`sessionLevels` computes them and `secDay` reads them); what goes is
      // their claim on a row in the ladder's level rail, which is reserved for structure he trades
      // off. Removing a label must never remove the measurement behind it (v11.95).
      [['PDH',SESSL.pdh],['PDL',SESSL.pdl],['PDC',SESSL.pdc]].forEach(function(sd){
        if(sd[1]!=null) LV.push({ n:sd[0], at:sd[1] }); });
    }
    try{
      if(CFG.spyFlag!==false && typeof LASTFEED!=='undefined' && LASTFEED.SPY && LASTFEED.SPY.j &&
         (Date.now()-(LASTFEED.SPY.ts||0))<=FEED_STALE_MS*3 && typeof EB.scaleUsed==='number'){
        var ewL=null; try{ ewL=extractWalls(LASTFEED.SPY.j); }catch(eLa){}
        if(ewL && ewL.king!=null) LV.push({ n:'SPY K', at:ewL.king*EB.scaleUsed, spyk:true });
      }
    }catch(eLb){}
    // (v14.42, PHASE A1) THE DARK POOLS hang here like every other structural level. Their prints
    // are in the UNDERLYING's scale (SPY/QQQ), so they cross to the chart by EB.scaleUsed — the same
    // multiplier the SPY King flag uses, and for the same reason: it is the only book that can carry
    // a cash print. NO LIFECYCLE STATE IS CLAIMED YET (phase A2): a level is drawn and named, and
    // what it has DONE is left unsaid rather than guessed. The hover carries the print's size and age
    // so the level is never a bare number.
    try{
      var DPL=darkPoolLevels(sym);
      var DPLC=null; try{ DPLC=dpLifecycle(sym); }catch(eLc){}
      if(DPL && typeof EB.scaleUsed==='number' && EB.scaleUsed>0){
        DPL.prints.forEach(function(pr, iDp){
          // (v14.44) the LIFECYCLE rides the name, so the level says what it has DONE, not just
          // where it is. Underlying scale in, chart scale out — the conversion happens HERE and
          // nowhere else.
          var lc=(DPLC && DPLC[iDp]) ? DPLC[iDp] : null;
          var tag=(lc && lc.st && lc.st!=='UNKNOWN') ? (' '+(DP_ST_SHORT[lc.st]||'')) : '';
          LV.push({ n:'DP'+tag, at:pr.px*EB.scaleUsed, dp:true, size:pr.size, notional:pr.notional,
                    at0:pr.px, when:pr.at, stale:DPL.stale, lc:lc });
        });
      }
    }catch(eDp){}
    // the IF levels, moved here off the main rail (v14.40). Chart scale via the Atlas-anchored
    // basis (ifLadder.dispScale). MP* stays off the line — the read already names max pain on an
    // OPEX day, and our recomputed number beside their published walls would invite confusion.
    try{
      var IFL=ifLadder(sym);
      if(IFL && !IFL.err){
        var IFN={ CR:'CW', PS:'PW', CR0:'CW0', PS0:'PW0', Mag:'T', FLIP:'FLIP', 'FLIP*':'FLIP*' };
        IFL.rows.forEach(function(r){
          var toks=String(r.id).split('\u00b7').map(function(t){ return IFN[t]||null; }).filter(Boolean);
          if(!toks.length) return;
          LV.push({ n:'IF '+toks.join('\u00b7'), at:r.disp, iff:true });
        });
      }
    }catch(eLc){}
    if(!LV.length) return '';
    LV.forEach(function(L){
      L.d=L.at-frNow;
      L.x=emPosRail(EB, L.at, RB);
      L.confl=false;
      try{ for(var cL=0;cL<RAILPS.length;cL++){ if(Math.abs(RAILPS[cL].disp-L.at)<=SESS_CONFL_PTS){ L.confl=true; break; } } }catch(eLd){}
    });
    // ⚠⚠ (v14.43) A CLAMPED POSITION IS A FALSE POSITION, AND THE DARK POOLS PROVED IT ON DAY ONE.
    // emPosRail CLAMPS to 0..100 by design (it protects the rail's own drawing), so any level
    // outside the frame lands ON THE EDGE. The first live capture put three dark pools 115-220 pts
    // below the rail: all three pinned at 0% and MERGED with the SPY King (also off-frame) into one
    // stack reading "SPY K·DP·DP·DP 7632" — four levels named, ONE price shown, and that price
    // belonging to none of the dark pools. A level drawn where it is not is worse than a level not
    // drawn. Off-frame levels therefore LEAVE THE LINE and are DISCLOSED instead: the count and the
    // nearest few ride the bar's hover, which keeps the information without faking a position.
    var OFF=[];
    LV=LV.filter(function(L){
      if(!isFinite(L.x)) return false;
      if(RB && typeof RB.lo==='number' && typeof RB.hi==='number' && (L.at<RB.lo || L.at>RB.hi)){ OFF.push(L); return false; }
      return true;
    });
    if(!LV.length && !OFF.length) return '';
    LV.sort(function(a,b){ return a.x-b.x; });
    var GRP=[];
    LV.forEach(function(L){
      var g=GRP.length?GRP[GRP.length-1]:null;
      if(g && (L.x-g.m[g.m.length-1].x)<=4.5) g.m.push(L);
      else GRP.push({ m:[L] });
    });
    var offTxt='';
    if(OFF.length){
      OFF.sort(function(a,b){ return Math.abs(a.d)-Math.abs(b.d); });
      offTxt=' \u2014 '+OFF.length+' level'+(OFF.length>1?'s':'')+' OFF THIS FRAME, not drawn because a clamped position would be a false one: '+
        OFF.slice(0,4).map(function(L){ return L.n+' '+frameNum(L.at)+' ('+(L.d>0?'+':'')+Math.round(L.d)+')'; }).join(', ')+
        (OFF.length>4?(', and '+(OFF.length-4)+' more'):'')+'.';
    }
    if(!LV.length) return '<div class="g3ll"><i class="g3llbar"'+g3tip('THE LEVELS LINE \u2014 nothing in frame.'+offTxt)+'></i></div>';
    var h='<i class="g3llbar"'+g3tip('THE LEVELS LINE \u2014 session structure (30-min IB, prior day), the SPY King, and the InsiderFinance levels (italics \u2014 THEIR numbers), each hanging under the line with its arrowhead pointing at the spot on the rail below where that price lives. BLUE = a gamma node sits on it. Overlapping levels merge into one stack \u2014 hover for each exact price. The white notch is price now.'+offTxt)+'></i>';
    var pN=emPosRail(EB, frNow, RB);
    if(isFinite(pN)) h+='<i class="g3llpx" style="left:'+pN.toFixed(1)+'%"'+g3tip('Price now \u2014 '+frameNum(frNow))+'></i>';
    GRP.forEach(function(g){
      var pick=g.m[0];
      g.m.forEach(function(L){ if(Math.abs(L.d)<Math.abs(pick.d)) pick=L; });
      var x=0; g.m.forEach(function(L){ x+=L.x; }); x/=g.m.length;
      var name=g.m.map(function(L){ return L.n; }).join('\u00b7');
      var spyk=g.m.some(function(L){ return L.spyk; });
      var confl=g.m.some(function(L){ return L.confl; });
      var iff=g.m.every(function(L){ return L.iff; });
      var isdp=g.m.some(function(L){ return L.dp; });
      var cls='g3llv '+(pick.d>0?'g3llvup':'g3llvdn')+(confl?' g3llvc':'')+(spyk?' g3llvk':'')+(isdp?' g3llvdp':'')+(iff?' g3llvif':'');
      var edge=(x>94)?';transform:translateX(-100%)':((x<6)?';transform:translateX(0)':'');
      var tip=g.m.map(function(L){ return L.n+' '+frameNum(L.at)+' ('+(L.d>0?'+':'')+Math.round(L.d)+')'; }).join(' \u00b7 ')+
              '. \u25b4 The head points at where this price sits on the rail below.'+
              (confl?' A GAMMA NODE sits on this level \u2014 structure and gamma stacked (Garma r22).':'')+
              (spyk?' The OTHER book\'s crown \u2014 price bounces off it even when every dynamic here is SPXW.':'')+
              (iff?' InsiderFinance\'s numbers, not ours \u2014 open-interest gamma, a stock not a flow.':'')+
              (isdp?(' DARK POOL \u2014 Skylit\'s own level set: the top prints over a 45-day lookback, a CASH-EQUITY print (which is why it is quoted on '+g3esc(sym)+' and converted here). '+
                (function(){ var d=null; g.m.forEach(function(L){ if(L.dp) d=L; }); if(!d) return '';
                  return (d.at0!=null?(g3esc(sym)+' '+d.at0+'. '):'')+
                         (d.notional!=null?('Print '+(usdBig(d.notional)||d.notional)+'. '):(d.size!=null?('Print '+d.size+' shares. '):''))+
                         (d.when!=null?('Printed '+Math.max(0,Math.round((Date.now()-d.when)/86400000))+'d ago. '):'')+
                         (d.stale?'\u26a0 capture is over a day old. ':''); })()+
                (function(){ var d2=null; g.m.forEach(function(L){ if(L.dp&&L.lc) d2=L.lc; });
                  if(!d2) return 'Lifecycle unavailable \u2014 no price history to judge it by.';
                  return 'STATE: '+d2.st+' \u2014 '+d2.why+'.'+
                    (d2.touches>0?(' Tested '+d2.touches+'x in the observed window.'):'')+
                    (d2.obsFrom?(' \u26a0 Judged only on bars we actually watched (since '+
                      (new Date(d2.obsFrom)).toISOString().slice(0,10)+') \u2014 their lookback is 45 days and ours is the chart window, so anything before that is unseen, not assumed.'):'')+
                    ' BROKEN needs a CLOSE through, never a wick.'; })()):'');
      h+='<span class="'+cls+'" style="left:'+x.toFixed(1)+'%'+edge+'"'+g3tip(tip)+'>'+
         g3esc(name)+'<b>'+g3esc(frameNum(pick.at))+(confl?' \u00b7node':'')+'</b><i></i></span>';
    });
    return '<div class="g3ll">'+h+'</div>';
  }catch(e){ return ''; }
}
