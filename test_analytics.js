var A=require('./analytics_v1021.js');
var day=require('./day_811.json');

function line(){ console.log('----------------------------------------'); }

console.log('=== KING BEHAVIOR (SPY) ===');
var kb=A.kingBehavior(day,'SPY');
console.log('pts',kb.pts,'| distinct levels',kb.levels,'| firstK',kb.firstK,'lastK',kb.lastK,'| netDrift',kb.netDrift);
console.log('rolls',kb.rolls,'(up',kb.rollUp,'dn',kb.rollDn,') avgRollSize',kb.avgRollSize);
console.log('offsetAvg',kb.offsetAvg,'pullDir',kb.pullDir,'| above/at/below bars',kb.aboveBars,kb.atBars,kb.belowBars);
console.log('reachRate',kb.reachRate,'% ('+kb.reachHit+'/'+kb.reachN+') avgTimeToReach',kb.avgTimeToReach,'bars | convergeRate',kb.convergeRate,'%');
console.log('regimes:',kb.regimes.map(function(r){return r.k+'x'+r.bars+(r.reached?('(reach@'+r.reachBars+')'):'(no-reach)');}).join('  '));
line();

console.log('=== ACCUMULATION EDGE (SPY) ===');
console.log(JSON.stringify(A.accumEdge(day,'SPY','accum'),null,0));
line();
console.log('=== DISSIPATION EDGE (SPY) ===');
console.log(JSON.stringify(A.accumEdge(day,'SPY','fade'),null,0));
line();
console.log('=== COMBINED EDGE (SPY) ===');
console.log(JSON.stringify(A.combinedEdge(day,'SPY'),null,0));
line();
console.log('=== CROSS-SYMBOL KING ===');
console.log(JSON.stringify(A.crossKing(day),null,0));
line();

// ---- assertions: honest behavior on this flawed day ----
var fails=0;
function assert(name,cond){ console.log((cond?'PASS':'FAIL')+': '+name); if(!cond) fails++; }
assert('king path found (67 pts)', kb.pts===67);
assert('distinct king levels = [769,770,771,775]', JSON.stringify(kb.levels)===JSON.stringify([769,770,771,775]));
assert('net drift +4 (rolled up into 775 pin overall)', kb.netDrift===4);
assert('rolls detected (>0)', kb.rolls>0);
assert('king mostly BELOW price (belowBars>aboveBars)', kb.belowBars>kb.aboveBars);
assert('cross-symbol correctly gated (QQQ/SPX pending)', A.crossKing(day).available===false);
var ae=A.accumEdge(day,'SPY','accum');
assert('accum edge has a baseline', ae.baseline!=null);
assert('accum edge produced some sample (support or resistance n>0)', (ae.support.n+ae.resistance.n)>0);
var ce=A.combinedEdge(day,'SPY');
assert('combined edge computes baseline', ce.baseline!=null);
line();
console.log(fails===0?'ALL ANALYTICS TESTS PASS':(fails+' TEST(S) FAILED'));
process.exit(fails===0?0:1);
