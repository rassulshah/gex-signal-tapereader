// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function kingReadHtml(A, kv){
  if(!A || !A.ok) return '';
  // (v10.42) KING CONSOLE: prose blob -> labeled indicator TILES + 2-line
  // STRUCTURE/READ. Every element carries a hover tooltip that explains it.
  function tile(label, value, tip, hot){
    return '<div title="'+(tip||'').replace(/"/g,'&quot;')+'" style="background:'+(hot?'rgba(242,180,90,0.07)':'rgba(255,255,255,0.025)')+
      ';border:1px solid '+(hot?'rgba(242,180,90,0.55)':PAL.line)+';border-radius:6px;padding:3px 6px;min-width:0">'+
      '<div style="font-size:7px;letter-spacing:.6px;color:'+PAL.sub+'">'+label+'</div>'+
      '<div style="font-size:10.5px;font-weight:700;color:'+PAL.ink+';font-variant-numeric:tabular-nums;line-height:1.25">'+value+'</div></div>';
  }
  var g=function(t,c){ return '<b style="color:'+(c||PAL.ink)+'">'+t+'</b>'; };
  var polV=(A.pol===true)?fmtNum(A.king)+' <span style="color:'+PAL.longAccent+';font-size:8.5px">+γ</span>'
         :(A.pol===false)?fmtNum(A.king)+' <span style="color:#b48ce3;font-size:8.5px">−γ</span>'
         :fmtNum(A.king);
  var kdV=(A.kd!=null)?(fmtKd(A.kd)+(A.kdChg!=null?' <span style="color:'+(A.kdChg>=0?PAL.longAccent:PAL.shortAccent)+'">'+fmtChg(A.kdChg)+'</span>':'')):'—';
  var distV=(A.dist===0)?'AT':(Math.abs(Math.round(A.dist))+(A.dist>0?'↑':'↓'))+(A.grav?' <span style="color:'+PAL.sub+';font-size:8px">grav≤3</span>':' <span style="color:'+PAL.amber+';font-size:8px">out</span>');
  var evaV=(A.eva)?(fmtNum(A.eva.lo)+'–'+fmtNum(A.eva.hi)+' <span style="color:'+(A.inVA?PAL.longAccent:PAL.amber)+';font-size:8px">'+(A.inVA?'in':'OUT')+'</span>'):'—';
  var succV=(A.succ)?(fmtNum(A.succ.k)+' · '+A.succ.a+'%'):'—';
  var alignV='QQQ '+(A.qqq==null?'—':(A.qqq?'<span style="color:'+PAL.longAccent+'">✓</span>':'<span style="color:'+PAL.shortAccent+'">✗</span>'))+' <span style="color:'+PAL.sub+';font-size:8px">VIX —</span>';
  var tiles='<div style="display:grid;grid-template-columns:1fr 1fr 1fr 1fr;gap:3px">'+
    tile('KING', polV, 'The King: strike with the LARGEST ABSOLUTE dealer exposure (Academy: settlement anchor, one per session). +γ = dealers long gamma there (friction/pin behavior); −γ = pro-cyclical hedging (fuel/overshoot behavior).')+
    tile('K$ · TODAY', kdV, 'King dollar magnitude parsed from the tape ($K row) and its change vs session open. Growth = real directional intent; bleed = hedge decay, pin thesis weakening (Academy node-lifecycle). Recorded every bar.')+
    tile('DIST', distV, 'Strikes between price and King. Gravity gate: on our recorded data the toward-King edge exists only within 3 strikes (54-60% inside vs 47%/0% beyond) — outside it, pull claims are suppressed.')+
    tile('VALUE 70%', evaV, 'Exposure Value Area: the tightest strike band around the King holding ~70% of total dealer mass (POC concept). Inside = rotation regime (57% toward-King, n=260). OUTSIDE = imbalance — continuation, do NOT fade (n=25).')+
    tile('SUCCESSION'+(A.succHot?' ⚠':''), (A.succHot?'<span style="color:'+PAL.amber+'">'+succV+'</span>':succV), 'Strongest non-King strike as % of King mass \u2014 who is second in line. \u26a0 THIS TILE CLAIMED '+SUCC_META.withdrawn+'% UNTIL 2026-08-29 AND IT DID NOT REPRODUCE. Re-measured on 9 recorded sessions against the crown the panel actually DRAWS (the latched one, not the raw tape that flaps between near-equal strikes): at \u226560% the crown moved to that strike '+SUCC_META.hz30+'% of the time within 30 minutes, '+SUCC_META.hz60+'% within an hour, and never above '+SUCC_META.hzDay+'% at ANY horizon. The original 76% was 4 days scored against the raw tape, where a one-snapshot flicker counted as a succession. \u26a0 What DOES separate is the node\u2019s own STATE, on the same 9 sessions at \u226580%: BUILDING '+SUCC_META.stBuild+'% and FADING '+SUCC_META.stFade+'% within 30m, but STEADY only '+SUCC_META.stSteady+'% \u2014 below the '+SUCC_META.chance+'% base rate for any ranked strike. Motion is the signal, stasis is the anti-signal, and SIZE ALONE is the weakest of the three. Read the state chip on that node\u2019s own row before reading this number.', A.succHot)+
    tile('TAPS · CROSS', A.taps+' · '+A.cross, 'Tap episodes at the King today and side crossings. Academy freshness claim: 1st tap ~80% reaction, 2nd ~66%, 3rd ~33% (unverified). Many crossings = chop axis, not a wall.')+
    tile('PHASE', A.phase+(A.toClose?' · '+A.toClose+'m':''), 'Session phase (CT): OPEN 8:30-9:30 · MID · LUNCH 11:30-1:00 · POWER = last 30m (forced-flow volatility; pin window when near the King). Minutes shown = to the 15:00 close.')+
    tile('ALIGN', alignV, 'Cross-market King agreement: is QQQ\'s King on the same side of its own price as SPY\'s? Aligned = system-wide pull. VIX confirmation chip pending the ladder spike.')+
  '</div>';
  // STRUCTURE / READ
  var polTxt=(A.pol===true)?('+γ — dealers '+g('buy dips / sell rips into it')+' → friction, absorption')
            :(A.pol===false)?('−γ — dealers hedge '+g('pro-cyclically')+' → fuel, overshoot risk')
            :'polarity unknown';
  var P=[];
  if(A.over) P.push(g('OVERSHOOT of +γ King',PAL.amber)+' without a 14-bar high — stretch not break; reversion through '+fmtNum(A.king)+' favored ⚖');
  if(A.succHot && A.succ) P.push(g('SUCCESSION: '+fmtNum(A.succ.k)+' at '+A.succ.a+'%',PAL.amber)+' \u2014 \ud83d\udcca '+SUCC_META.hz30+'% crowned \u226430m, '+SUCC_META.hz60+'% \u226460m (n='+SUCC_META.n+') \u2014 read the node\u2019s STATE');
  if(!A.grav) P.push('outside gravity (>3) — pull '+g('unsupported')+' 📊');
  else if(A.appr && A.appr.approaching) P.push('approaching — 📊 63% continue; ETA ~'+(A.appr.etaBars*3)+'m'+((A.phase==='POWER'&&A.adist<=1.5)?' · '+g('PIN WINDOW',PAL.gold)+' ⚖':''));
  if(A.inVA===false) P.push(g('outside value',PAL.amber)+' — imbalance: don’t fade 📊 (n=25)');
  if(A.kdChg!=null && A.kdChg<=-15) P.push('magnet '+g('bleeding '+fmtChg(A.kdChg),PAL.shortAccent)+' — reshuffle risk ⚖');
  if(A.kdChg!=null && A.kdChg>=15) P.push('magnet '+g('building '+fmtChg(A.kdChg),PAL.longAccent)+' — conviction rising ⚖');
  if(!P.length) P.push('no active King signal — watching');
  return '<div style="padding:4px 8px 5px 8px;border-left:2px solid '+PAL.gold+';margin:3px 0 4px 0">'+
    tiles+
    '<div title="What the structure IS right now — polarity-driven dealer mechanics at the King. Descriptive, not predictive." style="font-size:9.5px;line-height:1.45;color:'+PAL.ink+';margin-top:5px"><span style="color:'+PAL.gold+';font-weight:800">STRUCTURE ▸</span> +'+
      ('γ'==='γ'?'':'')+'King '+g(fmtNum(A.king),PAL.gold)+' ('+polTxt+') · '+(A.dist===0?'AT price':(Math.abs(Math.round(A.dist))+' '+(A.dist>0?'above':'below')))+(A.eva?(A.inVA?' · inside value':' · outside value'):'')+'</div>'+
    '<div title="What is LIKELY next, priority-ordered. ⚖ = Skylit Academy doctrine claim. 📊 = measured on our recorded bars, n shown. These labels re-read from the live Projection Scorecard as it unlocks." style="font-size:9.5px;line-height:1.45;color:'+PAL.ink+';margin-top:2px"><span style="color:'+PAL.gold+';font-weight:800">READ ▸</span> '+P.slice(0,3).join(' · ')+'</div>'+
    '<div title="Provenance: what backs these claims. Drift was demoted to descriptive after testing at 50% (n=68) — a coin flip." style="font-size:7.5px;color:'+PAL.sub+';margin-top:3px;opacity:.85">⚖ Academy · 📊 measured 4d/324 bars — sharpens nightly in Analysis ▸ 🎯 · drift descriptive-only (50%, n=68)</div>'+
  '</div>';
}
