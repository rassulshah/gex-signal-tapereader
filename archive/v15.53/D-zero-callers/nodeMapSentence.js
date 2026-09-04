// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeMapSentence(m, sym, tagFn){
  // (v10.47 A.2, user-directed) BARE BONES. e.g. "CONT thru Gate 774.50 → King 775.38: Gate Dec ▼8%, King Acm ▲12%. Sup 773.25 Acm."
  var px=m.px, lv=(m.levels||[]).slice();
  var b=_nmB, N=fmtNum;
  if(!lv.length) return {verdict:'NONE', text:''};
  var eng=null, how=null;
  lv.forEach(function(L){ if(L.deflection && L.deflection.bars<=DEFLECT_CONFIRM+EP_DEFL_HANDOFF && (!eng || L.dist<eng.dist)){ eng=L; how='defl'; } });
  if(!eng) lv.forEach(function(L){ var ep=L.ep||{}; if(ep.state==='BOw' && (!eng||L.dist<eng.dist)){ eng=L; how='bow'; } });
  if(!eng) lv.forEach(function(L){ var ep=L.ep||{}; if(ep.state==='Push' && (!eng||L.dist<eng.dist)){ eng=L; how='push'; } });
  if(!eng) lv.forEach(function(L){ var ep=L.ep||{}; if(ep.state==='Pull' && L.dist<=1.5 && (!eng||L.dist<eng.dist)){ eng=L; how='pull'; } });
  if(!eng) return {verdict:'NO NODE IN PLAY', text:''};
  var side=eng.side;
  var above=lv.filter(function(L){return L.k>px;}).sort(function(a,c){return a.k-c.k;});
  var below=lv.filter(function(L){return L.k<px;}).sort(function(a,c){return c.k-a.k;});
  var beyond=(side==='above'?above:below).filter(function(L){ return L!==eng && (side==='above'?L.k>eng.k:L.k<eng.k); })[0]||null;
  var oppEdge = side==='above' ? m.flr : m.ceil;
  function nm(L){ return (L.isKing?'King ':(L.isGatekeeper?'Gate ':(L.isFlr?'Flr ':(L.isCeil?'Ceil ':''))))+b(N(L.k),L.isKing?PAL.gold:PAL.ink); }
  function stt(L){ // 'Acm ▲12%' | 'Dec ▼8%' | 'steady'
    var c=(typeof L.chg==='number')?L.chg:0;
    var acc=_nmIsAcc(L), dec=_nmIsDec(L);
    // (v10.49 I) accumCanon is THE single Acm source — the sentence and the zone rows read
    // the SAME numbers, so "Acm" here and "Dec" there can no longer contradict each other.
    try{ var __a=accumCanon(sym, L.k);
         if(__a && __a.session && typeof __a.session.pct==='number'){ c=__a.session.pct; acc=(__a.session.label==='Acm'); dec=(__a.session.label==='Dec'); } }catch(eAC){}
    if(acc) return 'Acm'+(c>0?(' ▲'+Math.round(c)+'%'):'');
    if(dec) return 'Dec'+(c<0?(' ▼'+Math.round(Math.abs(c))+'%'):'');
    return 'steady';
  }
  var taps=eng.taps||0, third=taps>=2, purple=(eng.pos===false);
  var why=[];
  var verdict, text;
  if(how==='defl'){
    var up=eng.deflection.dir>0;
    verdict='DEFL';
    why.push(nm(eng)+' '+stt(eng)); if(taps) why.push((taps===1?'1st':taps===2?'2nd':taps===3?'3rd':taps+'th')+' tap'); if(purple) why.push('−γ');
    text=(up?'↑':'↓')+' off '+nm(eng)+': '+why.slice(1).concat([stt(eng)]).join(', ')+'.';
    return {verdict:verdict, text:text, node:eng};
  }
  var boft = /FT/.test((typeof tagFn==='function')?(tagFn(eng.k)||''):'');
  var engAcc=_nmIsAcc(eng), engDec=_nmIsDec(eng);
  var cont = boft || engDec || third || (!engAcc && beyond && _nmIsAcc(beyond) && beyond.isKing);   // 3rd+ tap always flips to continuation (Skylit ~33%)
  var rev  = !cont && (engAcc || (taps>=1 && !engDec));
  var oppTxt = oppEdge ? ((side==='above'?'Sup ':'Res ')+b(N(oppEdge.k))+' '+stt(oppEdge)+'.') : '';
  if(cont){
    verdict='CONT';
    why.push(stt(eng)+((third && !engDec)?((engAcc?' but':',')+' held '+taps+'× — 3rd tap usually fails'):''));
    if(beyond) why.push(nm(beyond)+' '+stt(beyond)+(beyond.isKing&&_nmIsAcc(beyond)?' pulling':''));
    if(purple) why.push('−γ fast');
    text='thru '+nm(eng)+(beyond?(' → '+nm(beyond)):'')+': '+why.join(', ')+'.'+(oppTxt?(' '+oppTxt):'');
  } else if(rev){
    verdict='REV';
    why.push(stt(eng)+(taps?(', held '+taps+'×'):''));
    if(beyond && _nmIsAcc(beyond)) why.push(nm(beyond)+' '+stt(beyond)+' behind');
    if(purple) why.push('−γ, counter-character');
    text='at '+nm(eng)+': '+why.join(', ')+'.'+(oppTxt?(' '+oppTxt):'');
  } else {
    verdict='TBD';
    text='at '+nm(eng)+': '+stt(eng)+(beyond?(', '+nm(beyond)+' behind'):'')+'.'+(oppTxt?(' '+oppTxt):'');
  }
  return {verdict:verdict, text:text, node:eng};
}
