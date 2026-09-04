// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function A_renderTop(sym){
  var day=A_day(); var reg=A_regime(day,sym); var kb=reg.kb||A_kingBehavior(day,sym);
  var h='';
  if(!kb || kb.pts<1) return h; // nothing to say yet; legacy block shows the await note
  // --- REGIME CHIP (the headline: what KIND of day is this) ---
  var rc = /Trend/.test(reg.label)?(/\u2193/.test(reg.label)?PAL.shortAccent:PAL.longAccent)
         : reg.label==='Whipsaw'?PAL.amber : reg.label==='Rainbow Road'?PAL.sub : PAL.blue;
  h+='<div style="margin:0 0 12px 0;background:#12161f;border:1px solid '+PAL.line+';border-left:3px solid '+rc+';border-radius:8px;padding:8px 10px"'+
     A_tip('REGIME sets the frame for everything below. Trend=trade pullbacks; Whipsaw=fade the range edges, avoid the middle; Rainbow Road=stand aside; the metrics below are read THROUGH this.')+'>'+
     '<div style="display:flex;justify-content:space-between;align-items:center">'+
       '<span style="font-size:13px;font-weight:800;color:'+rc+'">'+reg.label+'</span>'+
       '<span style="font-size:9px;color:'+PAL.sub+';text-transform:uppercase;letter-spacing:.5px">'+reg.conf+' confidence</span></div>'+
     '<div style="font-size:10px;color:#c8d3df;line-height:1.5;margin-top:3px">'+reg.why+'</div></div>';
  // --- KING BEHAVIOR (the anchor: where the magnet lived + did price obey) ---
  h+=_step(1,'\uD83D\uDC51 King behavior \u2014 the anchor');
  var pullTxt = kb.pullDir==='down'?'pulling DOWN (mostly below price)':kb.pullDir==='up'?'pulling UP (mostly above price)':'balanced';
  var pinTxt;
  if(kb.pinDist==null) pinTxt='\u2013';
  else if(kb.pinned) pinTxt='<span style="color:'+PAL.longAccent+'">PINNED</span> ('+kb.pinDist+'pt, '+kb.pinTiming+')';
  else pinTxt='<span style="color:'+PAL.amber+'">not pinned</span> ('+kb.pinDist+'pt off, '+(kb.pinTiming||'')+')';
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px;font-size:10px;color:#c8d3df;line-height:1.7">'+
    '<div'+A_tip('The King is the settlement magnet \u2014 the direction price is \u201Csupposed\u201D to follow. Its net drift + which side of price it sat is the day\u2019s directional pull.')+'>Path: <b>'+(kb.core?kb.core.join('\u2192'):kb.levels.join('\u2192'))+'</b> \u00b7 net drift <b>'+kb.netDrift+'</b> \u00b7 '+pullTxt+'</div>'+
    '<div'+A_tip('Rolls = King relocations. Mostly one-way rolls that LEAD price = a trend (stair-step). Balanced flip-flop rolls = a range (Whipsaw).')+'>Rolls: <b>'+kb.rolls+'</b> ('+kb.rollUp+'\u2191/'+kb.rollDn+'\u2193, avg '+(kb.avgRollSize==null?'\u2013':kb.avgRollSize)+' strikes)</div>'+
    '<div'+A_tip('Reach = did price actually tag a held King level (the pull WORKING). Doc rule: early reach \u2192 dealers drive price off; late reach \u2192 settlement pin.')+'>Reach: <b>'+(kb.reachRate==null?'\u2013':kb.reachRate+'%')+'</b> ('+kb.reachHit+'/'+kb.reachN+' levels)'+(kb.avgTimeToReach!=null?(' \u00b7 ~'+kb.avgTimeToReach+' bars to tag'):'')+' \u00b7 gap converged '+(kb.convergeRate==null?'\u2013':kb.convergeRate+'%')+'</div>'+
    '<div'+A_tip('The payoff of the pull: did price CLOSE on the King (a settlement pin) or get driven off? Late pin confirms the magnet; a far close = magnet ignored/decoy.')+'>Close vs King: '+pinTxt+' \u00b7 King '+kb.closeK+' / px '+(kb.closePx!=null?(+kb.closePx).toFixed(2):'\u2013')+'</div>'+
  '</div>';
  // --- EDGE FAMILIES (regime-aware; on range days directional edges are noise) ---
  var acc=A_accumEdge(day,sym,'accum'), fade=A_accumEdge(day,sym,'fade'), ce=reg.ce||A_combinedEdge(day,sym);
  var isRange = (reg.label==='Whipsaw'||reg.label==='Rainbow Road');
  h+=_step(2,'\uD83D\uDCC8 Did accumulation mark price?');
  if(isRange) h+=_await('This was a '+reg.label+' \u2014 a range/no-edge day. Directional accumulation edges below are LOW-signal here (the right read was fade-the-edges, not follow-through). Shown for completeness.');
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px">'+
    A_edgeRow('Support building \u2192 up', acc.support, acc.baseline, 'Building support BELOW price should pull price UP toward it. This is the King\u2019s magnet logic at the node level.')+
    A_edgeRow('Resistance building \u2192 dn', acc.resistance, acc.baseline, 'Building resistance ABOVE (a forming ceiling) should cap and press price DOWN.')+
    '<div style="font-size:9px;color:'+PAL.sub+';margin-top:5px">baseline up-rate '+(acc.baseline==null?'\u2013':acc.baseline+'%')+' \u00b7 lift over baseline is the real edge, not raw %</div></div>';
  h+=_step(3,'\uD83D\uDCC9 Did dissipation move price?');
  h+='<div style="margin:0 0 14px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px">'+
    A_edgeRow('Support fading \u2192 dn', fade.support, fade.baseline, 'A floor BLEEDING OUT below price removes support \u2014 price falls THROUGH the vacated level. Doc: rolling floors = bearish evidence.')+
    A_edgeRow('Resistance fading \u2192 up', fade.resistance, fade.baseline, 'A ceiling DISSOLVING above frees price to run UP. Doc: rolling ceilings/unwind = the move accelerating.')+
    '<div style="font-size:9px;color:'+PAL.sub+';margin-top:5px">dissipation is directional \u2014 not just \u201Cabsence of support\u201D</div></div>';
  h+=_step(4,'\uD83E\uDDE9 When both sides move (confluence)');
  h+='<div style="margin:0 0 16px 14px;background:#12161f;border:1px solid '+PAL.line+';border-radius:10px;padding:8px 10px;font-size:10px;color:#c8d3df;line-height:1.7">'+
    '<div'+A_tip('TRAPDOOR = resistance building overhead WHILE support fades below \u2014 the doc\u2019s coherent bearish stack. Should predict DOWN.')+'>Trapdoor (res build + sup fade \u2192 dn): <b>'+(ce.trapdoor.hit==null?'\u2013':ce.trapdoor.hit+'%')+'</b> <span style="color:'+PAL.sub+'">n'+ce.trapdoor.n+'</span></div>'+
    '<div'+A_tip('LIFTOFF = support building below WHILE resistance fades above \u2014 the bullish mirror. Should predict UP.')+'>Liftoff (sup build + res fade \u2192 up): <b>'+(ce.liftoff.hit==null?'\u2013':ce.liftoff.hit+'%')+'</b> <span style="color:'+PAL.sub+'">n'+ce.liftoff.n+'</span></div>'+
    '<div'+A_tip('BOTH sides building = compression \u2192 range/chop, NOT direction. High range-rate here means \u201Cdon\u2019t trade direction.\u201D')+'>Compression (both build \u2192 range): <b>'+(ce.compression.rangeRate==null?'\u2013':ce.compression.rangeRate+'%')+'</b> <span style="color:'+PAL.sub+'">n'+ce.compression.n+'</span></div>'+
    '<div'+A_tip('The whole thesis: does stacking reads (dual-confirmation) beat a single signal? If dual > single, confluence pays.')+'>Dual-confirmation dir hit: <b>'+(ce.dualVsSingle.dualHit==null?'\u2013':ce.dualVsSingle.dualHit+'%')+'</b> <span style="color:'+PAL.sub+'">n'+ce.dualVsSingle.dualN+'</span> \u00b7 net-flow polarity '+(ce.netFlow.dirHit==null?'\u2013':ce.netFlow.dirHit+'%')+'</div></div>';
  return h;
}
