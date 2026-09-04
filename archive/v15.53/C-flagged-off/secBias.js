// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function secBias(sym){
  var B=biasVotes(sym);
  var col=B.dir>0?'g3up':(B.dir<0?'g3dn':'');
  var h='<div class="g3b">';
  // (v13.1) THE TREND WORD STAYS ON ONE LINE. `UPTREND BRK` wrapping to two lines cost a whole row of
  // vertical space and made the most important word on the panel read as two half-words.
  h+='<div class="g3vd2"'+g3tip('Which way, and on whose authority?\nThe 50-SMA decides. Nothing below can overrule it.\n⚠ A new trend needs 15 of 20 bars; a reversal out of a broken trend needs 11.'+
     (B.whyLong?('\n⚠ This one is BROKEN'+String(B.whyLong).replace(/^ — /,': ')+'.'):'')+
     '\n⚠ FLAT means the SMA has no side — it is never assembled out of the confirms.')+'>'+
     '<b class="'+col+'"'+g3tip('Which way, and on whose authority?\nThe 50-SMA decides. The reads below confirm it or they do not.\n⚠ The old tally let them outvote it and printed NEUTRAL on a 2-2 split while the trend was plainly on the chart.')+'>'+
     arrow(B.dir)+' '+B.verdict+'</b>'+
     '<span class="g3why">'+g3esc(B.why)+'</span></div>';
  h+='<div class="g3cf">';
  B.confirms.forEach(function(c){
    var cls='', mark='—';
    // (v11.94) a read that THREW is not a read that abstained
    var err=(B.errs&&B.errs[c.k])||null;
    // (v11.95) EVERY BADGE READS ITS OWN DIRECTION — up, down or sideways — instead of agreement with
    // the call. A tick said "this agrees with the SMA" and hid WHAT the read actually said; two reads
    // could both show ✓ while pointing opposite ways on a flat call. The COLOUR still carries
    // agreement, so the count is unchanged: green agrees, red disagrees, grey has no side.
    if(err){ cls=' n'; mark='!'; }
    else if(c.d==null){ cls=''; mark='—'; }                       // no reading at all
    else if(c.d>0){ mark='↑'; cls=(B.dir>0)?' y':((B.dir<0)?' n':''); }
    else if(c.d<0){ mark='↓'; cls=(B.dir<0)?' y':((B.dir>0)?' n':''); }
    else { mark='→'; cls=''; }                                    // sideways: a real reading, no side
    h+='<span class="g3chip'+cls+'"'+g3tip(err?('This read FAILED — '+g3esc(err)+'.\nIt is not abstaining, it is broken, and a dash would have hidden that.\n\n'+c.tip):c.tip)+'>'+c.k+' '+mark+'</span>';
  });
  // (v11.90) DRIFT MOVES ONTO THE CONFIRM ROW AS A BADGE — BUT IT STILL DOES NOT VOTE.
  // Two things are true at once and both have to survive this change:
  //  1. v11.44 pulled drift OUT of the confirm row because its tick meant "the two books agree with
  //     EACH OTHER" — and the live face read "↑ BULLISH" beside "DRIFT ✓ DN·conf": they agreed on DOWN,
  //     against an up call, and the tick said everything was fine. Agreement is only confirmation when
  //     it points the SAME WAY as the SMA, and that test is still applied below.
  //  2. The user shadowed drift on 2026-08-18 — "remove it until it is tested and proven" — and
  //     DRIFT_LIVE is still false. Measured 2026-08-24: AGREE-UP 25% on effN 10 against a 21% baseline
  //     over 2 sessions. It has NOT earned promotion.
  // So it is drawn as a badge for consistency and to save a row, but OUTLINED rather than filled and
  // behind a divider, and it is NOT in `nConf`. Drawn identically it would read as a fifth confirm and
  // inflate the very count that only started being recorded at v11.88.
  var dr=B.drift, vd=(dr&&dr.verdict)?dr.verdict:'NONE';
  var dDir=(dr&&typeof dr.dir==='number')?dr.dir:0;
  var books=/^AGREE/.test(vd)?'agree':(/^LEAN/.test(vd)?'lean':(vd==='SPLIT'?'split':'none'));
  var withCall=(B.dir!==0 && dDir!==0) ? (dDir===B.dir) : null;
  var agree, mark2, gtxt;
  if(books==='none'){ agree=null; mark2='·'; gtxt='both books not in yet'; }
  else if(books==='split'){ agree=false; mark2='✗'; gtxt='gamma and vanna lean opposite ways — nothing confirming'; }
  else if(withCall===false){ agree=false; mark2='✗'; gtxt='books '+books+' '+(dDir>0?'UP':'DOWN')+' — against the call'; }
  else if(withCall===true){ agree=true; mark2=(books==='agree')?'✓':'~'; gtxt=(dr.label||vd)+' · '+(dr.overlap?'bands overlap':'bands apart'); }
  else { agree=null; mark2='·'; gtxt=(dr.label||vd)+' · no side to confirm'; }
  var dcls=(agree===true)?' y':((agree===false)?' n':'');
  h+='<i class="g3sepv"></i>';
  h+='<span class="g3chip gate'+dcls+'"'+g3tip('Is anything structurally confirming the call?\nGamma and vanna either lean the same way relative to price, or they split. '+g3esc(gtxt)+'.\n⚠ A GATE, not a vote — it is not in the count. Gamma says HOW price moves, never which way.\n⚠ A tick means the books agree AND point with the call. Agreeing with each other on the wrong side is a cross.')+'>DRIFT '+mark2+'</span>';

  var cnt, ccol;
  if(B.broke){ cnt='confirms unavailable'; ccol='#f0616d'; }   // (v11.94) never a tally off a partial assembly
  else if(B.dir===0){ cnt='no side to confirm'; ccol='#8b98a9'; }
  else { cnt=B.nConf+' of '+B.confirms.length+' confirm'; ccol=confColour(B.nConf, B.nLive); }
  h+='<span class="g3cnt" style="color:'+ccol+'"'+g3tip('How much conviction is behind this call?\nHow many live confirms agree with the 50-SMA that owns the direction.\n⚠ Green only when EVERY live one agrees. A dash means that read had nothing to give, which is not the same as disagreeing.\n⚠ TREND alone measured 34%. Whether the count improves on that has never been tested — it only started being recorded at v11.88.')+'>'+cnt+'</span>';
  h+='</div>';
  h+='</div>';
  return h;
}
