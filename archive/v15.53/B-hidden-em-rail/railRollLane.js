// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function railRollLane(EB, RB, rolls, piles){
  try{
    if(!rolls || !rolls.length) return '';
    var show=rolls.slice(0, 3), h='', svg='', i;
    var dsc=1; try{ dsc=ifDispScale()||1; }catch(e0){}
    // (v14.23, operator-caught: "one of the red arrows shows a blank source") a roll's SOURCE has
    // usually drained below the node threshold — that is what a roll IS — so it has no post and no
    // label, and the arrow rose out of blank track. The origin now names itself: a small tag at
    // the rise, drawn ONLY when no post stands there (the v14.0 no-lane-text rule holds elsewhere).
    var pileAt={}; try{ (piles||[]).forEach(function(pp){ pileAt[pp.k]=1; }); }catch(ePA){}
    // (v14.30) TWO TIERS. KING-CLASS arrows (source or destination is the latched crown) draw
    // bold — the succession chain is the one migration that moves the DESTINATION itself
    // (successor >=60% -> 76% crown roll within 20 bars, n=148). FIELD arrows under $5M do not
    // draw at all (latched and recorded, just not worth eyes); the rest draw dim. Pairing
    // quality — how much of the shed mass the destination accounts for — rides every hover.
    var latchK=null; try{ var lkJ=JSON.parse(localStorage.getItem('gpts_kinglatch_v1')||'null'); if(lkJ) latchK=lkJ.k; }catch(eLK){}
    for(i=0;i<show.length;i++){
      var r=show[i];
      var kingClass=(latchK!=null && (r.from===latchK || r.to===latchK));
      if(!kingClass && typeof r.amt==='number' && Math.abs(r.amt)<5e6) continue;   // ⚖ field floor
      var pq=null; try{ if(r.got&&r.lost){ var g2=Math.abs(r.got), l2=Math.abs(r.lost);
        if(g2>0&&l2>0) pq=Math.round(100*Math.min(g2,l2)/Math.max(g2,l2)); } }catch(ePq){}
      var xFrom=emPosRail(EB, r.from*dsc, RB), xTo=emPosRail(EB, r.to*dsc, RB);
      if(!isFinite(xFrom)||!isFinite(xTo)) continue;
      if(!pileAt[r.from]){
        h+='<span style="position:absolute;left:'+xFrom.toFixed(1)+'%;top:-1px;transform:translateX(-50%);'+
             'font-size:6px;font-weight:800;color:'+((r.dir==='up')?'#7cc7ff':'#e0645f')+';opacity:.8;white-space:nowrap"'+
           g3tip('The roll\'s SOURCE — '+frameNum(r.from*dsc)+' (SPXW '+r.from+'). It shed '+(function(){try{return usdBig(Math.abs(r.lost))||'its mass';}catch(eU){return 'its mass';}})()+
                 ' into '+frameNum(r.to*dsc)+' and now sits below the node threshold, which is why no post stands here: a completed roll empties its origin.'+
                 // ⚠⚠ (v14.83) THIS CAVEAT MOVED HERE RATHER THAN DYING WITH ladderRolls. It was the
                 // roll-amount hover's, and that drawer was retired when its lane became the King
                 // columns — which would have silently deleted the one sentence saying a roll is an
                 // INFERENCE. Nobody publishes that a position moved between strikes; we pair a fall
                 // at one node with a rise at another and call it a roll. Losing an honesty caveat
                 // as a side effect of deleting a drawing is exactly the failure this project keeps
                 // catching, so it is re-homed to the surviving roll hover, not re-typed shorter.
                 ' \u26a0 INFERRED from paired changes, never an observed transfer: nobody publishes that a position moved between strikes, so this is a fall at one node matched to a rise at another. Pairing quality is what decides whether that match is trustworthy.')+
           '>'+g3esc(frameNum(r.from*dsc))+'</span>';
      }
      var col=(r.dir==='up')?'#7cc7ff':'#e0645f';
      // (v13.9) IN FLIGHT vs STUCK, the same grammar as the node list: a live roll moves (dashes), a
      // latched roll that stuck is a solid calm line with its age on the label.
      // (v14.29, operator-caught: "how is a solid arrow different from a moving arrow —
      // something is wrong") the moving dashes CLAIM live flow, so they key STRICTLY off r.live.
      // The old test (conf && !live) animated a signal-level roll that had already stopped —
      // motion asserted where none was happening. Solid = latched, whatever its count.
      var stuck=!r.live;
      var yTop=16-i*5;                                   // each roll gets its own lane height
      var x1=xFrom*10, x2=xTo*10;                        // viewBox is 0..1000 so percent maps directly
      var d='M'+x1.toFixed(1)+' 22 L'+x1.toFixed(1)+' '+yTop+' L'+x2.toFixed(1)+' '+yTop+' L'+x2.toFixed(1)+' 20';
      // (v14.24, operator-directed) the SOURCE gets a marker like the destination gets a head:
      // a small circle at the rise, so both ends of every roll are anchored to the eye.
      var wBase=kingClass?(stuck?2.2:1.8):(stuck?1.4:1);
      var oBase=kingClass?(stuck?.95:.6):(stuck?.55:.28);
      svg+='<circle cx="'+x1.toFixed(1)+'" cy="20" r="'+(kingClass?'3':'2.4')+'" fill="'+col+'" opacity="'+(stuck?'.85':'.55')+'"/>'+
           '<path d="'+d+'" fill="none" stroke="'+col+'" stroke-width="'+wBase+'" opacity="'+oBase+'" vector-effect="non-scaling-stroke"/>'+
           (stuck?'':('<path class="fl" d="'+d+'" fill="none" stroke="'+col+'" stroke-width="1.6" stroke-linecap="round" '+
             'stroke-dasharray="5 27" vector-effect="non-scaling-stroke"/>'));
      var amt=''; try{ amt=usdBig(Math.abs(r.amt))||''; }catch(e1){}
      var ageL=(stuck && typeof r.ageMin==='number')?(' · '+(r.ageMin<60?(r.ageMin+'m'):(Math.floor(r.ageMin/60)+'h'+(r.ageMin%60)+'m'))):'';
      // (v14.0, user-directed) NO TEXT ON THE LANE. The v13.7 edge-clamp fought label collisions the
      // user still saw ("it gets overlapped and i get the idea from just looking at the arrows"), so
      // the label is GONE, not repositioned - direction and endpoints carry the meaning, and every
      // number the label held lives in the arrowhead's hover, age included. The hover target is a
      // padded 14x12px box around the head, because a 6px triangle alone is unhittable.
      h+='<span style="position:absolute;left:'+xTo.toFixed(1)+'%;top:11px;transform:translateX(-50%);'+
           'width:14px;height:12px;display:flex;align-items:flex-end;justify-content:center"'+
         g3tip('ROLL '+(r.dir==='up'?'↑':'↓')+' '+frameNum(r.to*dsc)+(amt?(' · '+amt):'')+ageL+
               (kingClass?' · KING-CLASS — the crown itself is part of this migration; the succession chain is the one roll that moves the session destination.':'')+
               (pq!=null?(' · '+pq+'% paired — the share of shed mass the destination accounts for; low pairing is evaporation in a roll costume.'):'')+
               ' — '+r.from+' shed '+(usdBig(Math.abs(r.lost))||'')+' while '+
               r.to+' took '+(usdBig(Math.abs(r.got))||'')+', seen on '+(r.count||'?')+' bars'+
               (stuck?'. The window has slid past but the destination still holds what it received: the roll STUCK.':(r.live?'. Still in flight.':'.'))+
               ' ⚠ Measured on our own data, a roll destination holds no better than a node that is simply GROWING — the pairing tells you WHERE size went, which is a story, not independent evidence. See FINDINGS F6.')+
         '><i style="width:0;height:0;border-left:3px solid transparent;border-right:3px solid transparent;'+
           'border-top:4px solid '+col+'"></i></span>';
    }
    if(!svg) return '';
    return '<div class="g3rl"><svg viewBox="0 0 1000 22" preserveAspectRatio="none">'+svg+'</svg>'+h+'</div>';
  }catch(e){ return ''; }
}
