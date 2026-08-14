var fs=require('fs');
var src=fs.readFileSync('v10.js','utf8');
// crude extraction: pull each A_ function by name via regex slice.
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0) throw 'missing '+name;
  // find matching close brace by scanning
  var depth=0, started=false, j=i;
  for(; j<src.length; j++){ var c=src[j]; if(c==='{'){depth++;started=true;} else if(c==='}'){depth--; if(started&&depth===0){ j++; break; }} }
  return src.slice(i,j); }
var LOADED_DAY=null;
var buildDayExport=function(){ return {snaps:{}}; };
eval(grab('A_day')); eval(grab('A_num')); eval(grab('A_pct')); eval(grab('A_sideOf'));
eval(grab('A_kingBehavior')); eval(grab('A_accumEdge')); eval(grab('A_combinedEdge')); eval(grab('A_regime'));
var day=require('./day_811.json');
var kb=A_kingBehavior(day,'SPY');
console.log('KING: pts',kb.pts,'net',kb.netDrift,'rolls',kb.rolls,'(',kb.rollUp,'up/',kb.rollDn,'dn) pull',kb.pullDir,'reach',kb.reachRate+'% pinned',kb.pinned,'dist',kb.pinDist,'timing',kb.pinTiming,'closeK',kb.closeK,'closePx',kb.closePx);
var reg=A_regime(day,'SPY');
console.log('REGIME:',reg.label,'('+reg.conf+') heavy',reg.heavy,'spread',reg.spread);
console.log('  why:',reg.why);
var fade=A_accumEdge(day,'SPY','fade');
console.log('FADE support dirHit',fade.support.dirHit+'% (n'+fade.support.n+') swing',fade.support.swingHit+'% base',fade.baseline+'%');
var ce=A_combinedEdge(day,'SPY');
console.log('COMBINED trap',ce.trapdoor.hit+'%('+ce.trapdoor.n+') lift',ce.liftoff.hit+'%('+ce.liftoff.n+') netFlow',ce.netFlow.dirHit+'%');
// assertions
var f=0; function A(n,c){ console.log((c?'PASS':'FAIL')+': '+n); if(!c)f++; }
A('king pts 67',kb.pts===67);
A('pull down',kb.pullDir==='down');
A('8/11 classifies Whipsaw',reg.label==='Whipsaw');
A('fade-support edge computed',fade.support.dirHit!=null);
A('pin fields present',kb.pinDist!=null);
console.log(f===0?'INJECTED-OK':f+' FAIL');
process.exit(f?1:0);
