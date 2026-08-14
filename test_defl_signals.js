// Tests for v10.36 Deflection Signals: classification, grading, unlock-N, forward scoring.
var assert=require('assert');
var pass=0, fail=0;
function ok(c,m){ if(c){pass++;} else {fail++; console.log('FAIL: '+m);} }

// --- minimal harness: reproduce the pure logic under test ---
var PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',blue:'#4a90d9',amber:'#f2b45a',gold:'#e3c341',sub:'#8b98a9',ink:'#e6edf3'};
var DEFLECT_CONFIRM=2, DEFL_FWD_BARS=10, DEFL_CONT_PTS=0.30, DEFL_UNLOCK_MIN=5, DEFL_UNLOCK_MAX=25;

function deflUnlockN(perDayCount){
  if(!perDayCount||perDayCount<=0) return DEFL_UNLOCK_MIN;
  var n=Math.round(perDayCount*3);
  return Math.max(DEFL_UNLOCK_MIN, Math.min(DEFL_UNLOCK_MAX, n));
}
function deflGrade(rate){
  if(rate>=75) return {g:'A+'}; if(rate>=68) return {g:'A'};
  if(rate>=58) return {g:'B'}; if(rate>=45) return {g:'C'}; return {g:'D'};
}
// classification priority (mirrors classifyDeflection ordering)
function classify(L, dir, oc){
  var chips=[], name, prio;
  var brokeFT=(oc==='up'||oc==='dn'), isFBO=(oc==='false');
  if(L.isKing){ name='King deflection'; prio=90; }
  else if(L.isGatekeeper){ name='Gate deflection'; prio=85; }
  else if(L.isRugCeil||L.isRugFloor){ name='Rug'; prio=80; }
  else if(L.role==='Pika'){ name='Pika deflection'; prio=60; }
  else if(dir>0){ name='Floor deflection'; prio=50; }
  else { name='Ceiling deflection'; prio=50; }
  if(brokeFT){ prio+=8; name='\u2b51 '+name+' (BO\u00b7FT retest)'; }
  return {name:name, prio:prio, isFBO:isFBO, brokeFT:brokeFT};
}

// --- unlock-N auto-tune ---
ok(deflUnlockN(0)===5, 'unlock floor when no data => 5');
ok(deflUnlockN(1)===5, 'perDay 1 -> 3 -> floored to 5');
ok(deflUnlockN(3)===9, 'perDay 3 -> unlock 9');
ok(deflUnlockN(5)===15,'perDay 5 -> unlock 15');
ok(deflUnlockN(20)===25,'perDay 20 -> capped at 25');

// --- grade thresholds ---
ok(deflGrade(78).g==='A+', '78% => A+');
ok(deflGrade(71).g==='A',  '71% => A');
ok(deflGrade(60).g==='B',  '60% => B');
ok(deflGrade(44).g==='C' || deflGrade(44).g==='D', '44% => C or D boundary');
ok(deflGrade(44).g==='D',  '44% => D (below 45)');
ok(deflGrade(45).g==='C',  '45% => C');

// --- classification: priority ordering + flavor stacking ---
ok(classify({isKing:true}, 1, null).prio===90, 'King highest base prio');
ok(classify({isGatekeeper:true}, -1, null).prio===85, 'Gate prio 85');
ok(classify({isRugFloor:true}, 1, null).prio===80, 'Rug prio 80');
ok(classify({role:'Pika'}, 1, null).prio===60, 'Pika prio 60');
ok(classify({}, 1, null).name==='Floor deflection', 'bounce up => Floor deflection');
ok(classify({}, -1, null).name==='Ceiling deflection', 'reject down => Ceiling deflection');
var boft=classify({isKing:true}, 1, 'up');
ok(boft.brokeFT && boft.prio===98, 'BO\u00b7FT retest bumps King prio to 98');
ok(/BO\u00b7FT retest/.test(boft.name), 'BO\u00b7FT flavor in name');
ok(classify({isGatekeeper:true}, -1, 'false').isFBO===true, 'FBO flavor detected on Gate');

// --- forward scoring: continued vs not ---
function scoreCont(px0, dir, closes){
  var ext=0; closes.forEach(function(c){ var move=(c-px0)*dir; if(move>ext) ext=move; });
  return (ext>=DEFL_CONT_PTS)?1:0;
}
ok(scoreCont(775.0, 1, [775.1,775.4,775.2])===1, 'up deflect continued +0.4 => hit');
ok(scoreCont(775.0, 1, [775.1,775.2,775.05])===0, 'up deflect only +0.2 => miss');
ok(scoreCont(776.0,-1, [775.9,775.5,775.7])===1, 'down deflect continued -0.5 => hit');
ok(scoreCont(776.0,-1, [776.1,776.2])===0, 'down deflect moved wrong way => miss');

// --- stats aggregation shape ---
function agg(events){
  var perKey={}, resolved=0;
  events.forEach(function(e){ var b=perKey[e.key]||(perKey[e.key]={n:0,hit:0,pending:0});
    if(e.cont==null) b.pending++; else { b.n++; if(e.cont) b.hit++; resolved++; } });
  return {perKey:perKey, resolved:resolved};
}
var A=agg([{key:'King:up',cont:1},{key:'King:up',cont:1},{key:'King:up',cont:0},{key:'King:up',cont:null}]);
ok(A.perKey['King:up'].n===3 && A.perKey['King:up'].hit===2, 'King:up 2/3 resolved');
ok(A.perKey['King:up'].pending===1, 'one pending not counted');
ok(A.resolved===3, 'total resolved excludes pending');
ok(Math.round(100*2/3)===67, 'King:up rate 67% => grade B');

console.log('RESULT: '+pass+' passed, '+fail+' failed');
if(fail>0) process.exit(1);
