// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeRoleBadge(L){
  var label=null, col=PAL.sub, tip='';
  if(L.isKing){
    label='King'+(L.isFlr?' \u00b7 Flr':(L.isCeil?' \u00b7 Ceil':'')); col=PAL.gold; tip='King \u2014 the strongest magnet on the board (largest |dealer exposure|); settlement anchor. \uD83D\uDCCA pulls price 55% at 30m (n=299) \u2014 69% at 2 strikes (n=59), 74% in the 11am CT hour (n=42); inside 1 strike price ORBITS (50%).'+(L.isFlr?' Also the range Floor.':(L.isCeil?' Also the range Ceiling.':''));
  } else if(L.isGatekeeper){
    label='Gate'; col=PAL.amber; tip='Gatekeeper \u2014 deflection zone where trend shifts often begin; can block the path to the King.';
  } else if(L.isFlr){   // (v10.47b) range edges outrank rug/cluster labels (those ride in the hover + cards)
    label='\u26F0 Flr'; col=PAL.longAccent; tip='FLOOR \u2014 the LARGEST magnet BELOW price (Skylit: the range edge is where price consistently reverses); lower boundary of the live range (Step 3).'+(L.isRugFloor?' Also a Rug floor.':'');
  } else if(L.isCeil){
    label='\u2594 Ceil'; col=PAL.shortAccent; tip='CEILING \u2014 the LARGEST magnet ABOVE price (Skylit: the range edge is where price consistently reverses); upper boundary of the live range (Step 3).'+(L.isRugCeil?' Also a Rug ceiling.':'');
  } else if(L.isRugCeil){
    var rr=(L.rugType==='Reverse-Rug'); label=rr?'RRugF':'RugC'; col='#b58bff';
    // (v10.29) live geometry + state + targets folded into the hover (callout removed).
    var rd=L.rugDetail||{};
    var geo=(rd.ceilK!=null&&rd.floorK!=null)?(fmtNum(rd.ceilK)+' over '+fmtNum(rd.floorK)):'';
    var st=rd.shown?(rd.confirmed?'forming':'forming (unconfirmed \u2014 flow not yet aligned)'):'candidate \u2014 verify polarity';
    var tg=(rd.targets&&rd.targets.length)?(' \u00b7 targets '+rd.targets.map(fmtNum).join(', ')):'';
    tip=(rr?'Reverse-Rug':'Rug')+' ceiling'+(geo?(' \u00b7 '+geo):'')+' \u00b7 '+st+tg+' \u2014 positive-gamma cap over a negative-gamma floor; '+(rr?'bullish squeeze.':'bearish nosedive risk.');
  } else if(L.isRugFloor){
    var rr2=(L.rugType==='Reverse-Rug'); label=rr2?'RRugC':'RugF'; col='#b58bff';
    var rd2=L.rugDetail||{};
    var geo2=(rd2.ceilK!=null&&rd2.floorK!=null)?(fmtNum(rd2.ceilK)+' over '+fmtNum(rd2.floorK)):'';
    var st2=rd2.shown?(rd2.confirmed?'forming':'forming (unconfirmed \u2014 flow not yet aligned)'):'candidate \u2014 verify polarity';
    var tg2=(rd2.targets&&rd2.targets.length)?(' \u00b7 targets '+rd2.targets.map(fmtNum).join(', ')):'';
    tip=(rr2?'Reverse-Rug':'Rug')+' floor'+(geo2?(' \u00b7 '+geo2):'')+' \u00b7 '+st2+tg2+' \u2014 negative-gamma target below the positive-gamma cap.';
  } else if(L.isStack){
    label='DStk'; col=PAL.longAccent;
    var sd=L.stackDetail;
    var sspan=sd?(sd.lo===sd.hi?fmtNum(sd.lo):(fmtNum(sd.lo)+'\u2013'+fmtNum(sd.hi))):'';
    tip='Double-Stack'+(sspan?(' '+sspan):'')+(sd?(' ('+sd.n+' nodes)'):'')+' \u2014 adjacent stacked nodes forming a strong-BOUNCE shelf; price tends to bounce cleanly off it (fade into it).';
  } else if(L.isCluster){
    label='Pika'; col=PAL.blue;
    var cd=L.clusterDetail;
    var cspan=cd?(fmtNum(cd.lo)+'\u2013'+fmtNum(cd.hi)):'';
    tip='Pika Cloud'+(cspan?(' '+cspan):'')+(cd?(' ('+cd.n+' +gamma nodes)'):'')+' \u2014 dense cluster of POSITIVE-gamma nodes; acts as a gravity well, price PINS / ROTATES / CHOPS here (poor R:R \u2014 avoid the middle, fade the edges).';
  } else if(L.isBarney){
    label='Barn'; col=PAL.shortAccent;
    var bd=L.barneyDetail;
    var bspan=bd?(fmtNum(bd.lo)+'\u2013'+fmtNum(bd.hi)):'';
    tip='Barney'+(bspan?(' '+bspan):'')+(bd?(' ('+bd.n+' \u2212gamma nodes)'):'')+' \u2014 dense cluster of NEGATIVE-gamma nodes (NOT a Pika Cloud); an instability / acceleration zone: moves amplify, wicky/violent, overshoots common. Trade with, not against.';
  } else if(L.isFlr){
    label='\u26F0 Flr'; col=PAL.longAccent; tip='FLOOR \u2014 the LARGEST magnet BELOW price (Skylit: the range edge is where price consistently reverses): the lower boundary of the live range (Step 3). A magnet, not a promise: it pulls, holds, or repels \u2014 see ACTIVITY. \uD83D\uDCCA non-King mass repelled price 57% of the time (n=350).';
  } else if(L.isCeil){
    label='\u2594 Ceil'; col=PAL.shortAccent; tip='CEILING \u2014 the LARGEST magnet ABOVE price (Skylit: the range edge is where price consistently reverses): the upper boundary of the live range (Step 3). A magnet, not a promise: see ACTIVITY for pull/push. \uD83D\uDCCA non-King mass repelled price 57% (n=350).';
  } else if(L.isStrongMag && L.isNext){
    label='\u2605 Mag \u00b7 next'; col=PAL.sub; tip='Next target \u2014 a strong magnet BEYOND the range edge: where price goes if the ceiling/floor breaks.';
  } else if(L.isStrongMag){
    label='\u2605 Mag'; col=PAL.sub; tip='Strong magnet (>= '+FLRCEIL_MIN_PCT+'% of King mass) inside the range. No directional word by design \u2014 which side of price it sits on is the ladder; what it is doing is ACTIVITY.';
  } else {
    label='Mag'; col=PAL.sub; tip='Minor magnet (< '+FLRCEIL_MIN_PCT+'% of King mass).';
  }
  if(!label) return '';
  // (v10.44) Skylit convention: NEGATIVE-gamma identity renders PURPLE (incl. a -γ King).
  if(L.pos===false){ col='#b58bff'; tip+=' \u2212\u03B3 (purple): dealers hedge WITH the move here \u2014 wicky contact, overshoot-prone accelerant.'; }
  else if(L.pos===true){ tip+=' +\u03B3: dealers hedge AGAINST the move \u2014 sticky contact, pin-prone.'; }
  // secondary-role note in tooltip (e.g. a King that is also a rug participant)
  var extra=[];
  if(!L.isKing && L.isGatekeeper===false){}
  if(L.isRugTarget && !L.isRugCeil && !L.isRugFloor) extra.push('rug-target');
  var fullTip = tip + (extra.length?(' Also: '+extra.join(', ')+'.'):'');
  return '<span title="'+fullTip.replace(/"/g,'')+'" style="color:'+col+';font-size:8px;font-weight:800;border:1px solid '+col+';border-radius:10px;padding:0 5px;white-space:nowrap">'+label+'</span>';
}
