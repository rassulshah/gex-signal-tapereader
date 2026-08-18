// (v10.55 PART A) THE TREND / MAGNET / PULLBACK-NODE ENGINE.
//
// The user's own model, in their words: a trend is an ALTERNATION. Price "rallies down"
// to the MAGNET (the heavy node below, in a downtrend), a PULLBACK NODE forms ABOVE it
// on the counter-move, price deflects off that, and the next pullback node forms LOWER
// than the last. Lower-low (magnet) / lower-high (pullback node), each governed by a
// node. The Academy's "rolling ceilings" ARE those successive pullback nodes.
//
// THE FIXTURE IS SYNTHETIC. No 2026-08-17 day file or FCHIST export exists anywhere in
// this repo (daily-data/ has only gex_2026-08-11.json), so daily-data/fixture_2026-08-17_
// synthetic.json is a REPLAY of the sequence the user described and circled: downtrend
// under the SMA, magnet 773 below, ceilings appearing at 776 -> 775.5 -> 775. The test
// replays it through the SHIPPED legStep(), so it is the engine being checked, not a
// re-implementation of it. Swap in the real export and the test reads that instead.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- constants + display mocks (SPY cash mode: fmtLvl === fmtNum) -------
global.PB_MIN_PCT=20; global.PB_LEG_PTS=0.5; global.PB_REACH=5;
global.LEG_ROLL_SIGNAL=2; global.LEG_ROLL_CONFIRM=3;
global.FUTMODE={ chart:'SPY', fam:null, underlying:'SPY', r:1, live:true, approx:false, ok:true };
eval(['mul','fmtNum','fmtFut','dispIsFut','dispR','futMark','fmtLvl',
      'legStepWord','legBlank','legClone','legStep','legSentence','legDecisionLine','legZoneTag',
      'nodeHistRow','rollRun'].map(ex).join('\n'));

function replay(bars, seed){
  var st=seed||null, trail=[];
  bars.forEach(function(b){
    st=legStep(st, { dirIn:b.dirIn, px:b.px, close:(b.close!=null?b.close:b.px),
                     levels:b.levels, kingK:b.kingK!==undefined?b.kingK:FIX.kingK, t:b.t, bar:b.bar });
    trail.push(JSON.parse(JSON.stringify(st)));
  });
  return { st:st, trail:trail };
}

// ================= 1. THE 2026-08-17 SEQUENCE IS RECOVERED FROM THE FIXTURE =========
const FIX=JSON.parse(fs.readFileSync('./daily-data/fixture_2026-08-17_synthetic.json','utf8'));
ok(FIX.synthetic===true, '1·0 the fixture declares itself SYNTHETIC (no real 08-17 export exists in this repo)', FIX.date);
var R=replay(FIX.bars);
var S=R.st, T=R.trail;

ok(T[0].dir==='dn' && T[0].phase==='RLY', '1a bar 1: downtrend -> the engine is in the RLY leg', T[0].dir+'/'+T[0].phase);
ok(T[0].magnet && T[0].magnet.k===773 && T[0].magnet.isKing===true,
   '1b ...rallying DOWN to the magnet, which is the King below', T[0].magnet&&T[0].magnet.k);
ok(T[0].predictedPB===true, '1c ...and it PREDICTS a pullback node above before one exists', T[0].predictedPB);
ok(T[1].pbDetected===null, '1d a 12%-of-King node is not meaningful (PB_MIN_PCT=20) — nothing is detected', String(T[1].pbDetected));

ok(T[2].pbDetected && T[2].pbDetected.k===776, '1e bar 3: 776 APPEARS above price -> pullback node DETECTED', T[2].pbDetected&&T[2].pbDetected.k);
ok(T[2].phase==='PB' && T[2].roll.count===1 && T[2].pbDetected.rolledFrom===null,
   '1f ...phase PB, step 1, nothing to have rolled from yet', T[2].roll.count);
ok(T[2].event==='pbDetected', '1g ...and the bar is flagged so the ⚑ banner fires exactly once', T[2].event);

ok(T[3].phase==='RLY' && T[3].predictedPB===true,
   '1h bar 4: price leaves the node -> a NEW leg toward the magnet, predicting the next PB lower', T[3].phase);
ok(T[3].pbZone && T[3].pbZone.hi===776, '1i ...and the predicted zone is bounded by the last pullback node', T[3].pbZone&&T[3].pbZone.hi);

ok(T[4].pbDetected && T[4].pbDetected.k===775.5 && T[4].pbDetected.rolledFrom===776,
   '1j bar 5: 775.5 forms LOWER than 776 — the roll', (T[4].pbDetected||{}).k+' from '+(T[4].pbDetected||{}).rolledFrom);
ok(T[4].roll.count===2 && T[4].roll.signal===true && T[4].roll.confirmed===false,
   '1k ...2 consecutive = SIGNAL, not yet confirmation (Academy count rule)', T[4].roll.count);
ok(T[4].rolledOff.indexOf(776)>=0, '1l ...776 is tagged ROLLED OFF (it has lost target status)', JSON.stringify(T[4].rolledOff));

ok(S.pbDetected && S.pbDetected.k===775 && S.pbDetected.rolledFrom===775.5,
   '1m bar 7: 775 forms lower again', (S.pbDetected||{}).k);
ok(JSON.stringify(S.roll.steps)===JSON.stringify(FIX.expect.rollSteps),
   '1n THE WHOLE SEQUENCE IS RECOVERED: 776 -> 775.5 -> 775', JSON.stringify(S.roll.steps));
ok(S.roll.count===3 && S.roll.confirmed===true, '1o 3 consecutive = CONFIRMED downtrend', S.roll.count);
ok(S.rolledOff.indexOf(775.5)>=0 && S.rolledOff.indexOf(776)>=0,
   '1p both older ceilings are rolled off, the CURRENT pullback node is the resistance', JSON.stringify(S.rolledOff));
ok(S.air && S.air.lo===775 && S.air.hi===775.5,
   '1q the vacated zone between the old and new node is tagged AIR (fast travel, per doctrine)', JSON.stringify(S.air));
ok(S.magnet.k===773, '1r the magnet is still the King below — the target of the deflection', S.magnet.k);

// ---- the VOICE, in the user's own vocabulary ----
var sent=legSentence(S);
ok(/Downtrend \(SMA\)\./.test(sent), '1s the READ opens with the SMA-owned trend', sent);
ok(/Pullback node formed at 775/.test(sent) && /rolled lower from 775\.5/.test(sent),
   '1t ...names the node and what it rolled from');
ok(/3rd step — confirmed downtrend/.test(sent), '1u ...and the roll step, in words');
ok(/Resistance to sell from/.test(sent) && /Potential drop to magnet 773/.test(sent),
   '1v ...the LEVEL description the user uses, plus the magnet as the potential target');
ok(!/(sell now|go short|enter|stop at|size )/i.test(sent), '1w ...and never an instruction', sent);

// the RLY/prediction variant
var pred=legSentence(T[3]);
ok(/Rallying down to magnet 773\./.test(pred) && /Expect a pullback node to form above, below 776 — sell level\./.test(pred),
   '1x the prediction sentence is the spec\'s own wording', pred);

// the decision line + the row tags
ok(legDecisionLine(S,{k:775})==='sell-side deflection · tgt magnet 773 · inval above PB 775',
   '1y the decision line at the pullback node', legDecisionLine(S,{k:775}));
ok(legZoneTag(S,{k:775}).lab==='PB · 3rd lower', '1z1 the PB row carries its roll step', legZoneTag(S,{k:775}).lab);
ok(legZoneTag(S,{k:773}).lab==='MAG · target', '1z2 the magnet row is the target', legZoneTag(S,{k:773}).lab);
ok(legZoneTag(S,{k:776}).lab==='rolled off' && legZoneTag(S,{k:776}).dim===true,
   '1z3 a rolled-off ceiling is marked and dimmed', legZoneTag(S,{k:776}).lab);

// ================= 2. THE UPTREND IS THE EXACT MIRROR ==============================
var UP=[
  { bar:1, t:1, px:770.4, close:770.4, dirIn:'up', kingK:774, levels:[{k:774,pct:100,isKing:true,state:'Steady'}] },
  { bar:2, t:2, px:771.0, close:771.0, dirIn:'up', kingK:774, levels:[{k:774,pct:100,isKing:true,state:'Steady'},{k:770,pct:45,state:'Building'}] },
  { bar:3, t:3, px:771.8, close:771.8, dirIn:'up', kingK:774, levels:[{k:774,pct:100,isKing:true,state:'Steady'},{k:770,pct:44,state:'Steady'}] },
  { bar:4, t:4, px:772.2, close:772.2, dirIn:'up', kingK:774, levels:[{k:774,pct:100,isKing:true,state:'Steady'},{k:770,pct:40,state:'Fading'},{k:771,pct:50,state:'Building'}] },
  { bar:5, t:5, px:772.9, close:772.9, dirIn:'up', kingK:774, levels:[{k:774,pct:100,isKing:true,state:'Steady'},{k:771,pct:52,state:'Steady'},{k:772,pct:48,state:'Building'}] }
];
var U=replay(UP).st;
ok(U.dir==='up' && U.magnet.k===774 && U.magnet.isKing===true, '2a uptrend: the magnet is the King ABOVE', U.magnet.k);
ok(JSON.stringify(U.roll.steps)===JSON.stringify([770,771,772]), '2b pullback SUPPORTS roll HIGHER', JSON.stringify(U.roll.steps));
ok(U.roll.count===3 && U.roll.confirmed===true, '2c 3 higher supports = confirmed uptrend', U.roll.count);
ok(U.roll.side==='flr', '2d the roll is on the FLOOR side in an uptrend', U.roll.side);
var us=legSentence(U);
ok(/Uptrend \(SMA\)\./.test(us) && /rolled higher from 771/.test(us) && /Support to buy from/.test(us) &&
   /Potential rise to magnet 774/.test(us), '2e the mirrored voice: buy level, support to buy from', us);
ok(legDecisionLine(U,{k:772})==='buy-side deflection · tgt magnet 774 · inval below PB 772',
   '2f ...and the mirrored decision line', legDecisionLine(U,{k:772}));

// ================= 3. A PULLBACK NODE THAT ROLLS AGAINST THE TREND RESETS ==========
// Still a downtrend, price still under the 775 node, but the new node forms ABOVE the
// last one: exposure is building higher. The roll breaks, the count restarts, and the
// engine says "weakening" instead of pretending the sequence continued.
var AG=replay([{ bar:8, t:8000, px:774.30, close:774.30, dirIn:'dn', kingK:773,
   levels:[{k:773,pct:100,isKing:true,state:'Steady'},{k:775,pct:50,state:'Steady'},{k:776.5,pct:60,state:'Building'}] }], S).st;
ok(AG.pbDetected && AG.pbDetected.k===776.5, '3a the higher node IS taken as the new pullback node', AG.pbDetected.k);
ok(AG.roll.count===1 && AG.roll.confirmed===false, '3b ...and the roll count RESTARTS at 1', AG.roll.count);
ok(AG.roll.weakening===true, '3c ...flagged as weakening (a lower-high that is no longer lower)', AG.roll.weakening);
ok(/trend weakening/.test(legSentence(AG)), '3d ...and the READ says so out loud', legSentence(AG));

// ================= 4. INVALIDATION: A CLOSE THROUGH THE PULLBACK NODE ==============
var INV=replay([{ bar:9, t:9000, px:775.60, close:775.60, dirIn:'dn', kingK:773,
   levels:[{k:773,pct:100,isKing:true,state:'Steady'},{k:775,pct:48,state:'Fading'}] }], S).st;
ok(INV.invalidations.pbBreak===true, '4a a close ABOVE the pullback node invalidates the leg', INV.invalidations.pbBreak);
ok(INV.phase==='none', '4b ...the phase drops to none rather than carrying a dead frame', INV.phase);
ok(INV.note==='lower-high broken', '4c ...and it is described exactly as the doctrine describes it', INV.note);
ok(INV.roll.count===0 && INV.lastBroken && INV.lastBroken.k===775,
   '4d the roll sequence is broken, and WHICH level broke is remembered', JSON.stringify(INV.lastBroken));
ok(/Lower-high broken\./.test(legSentence(INV)), '4e the READ carries the break', legSentence(INV));
// a trend FLIP is the other invalidation
var FL=replay([{ bar:10, t:10000, px:775.9, close:775.9, dirIn:'up', kingK:773, levels:[{k:773,pct:100,isKing:true}] }], S).st;
ok(FL.invalidations.trendBreak===true && FL.roll.count===0,
   '4f the SMA flipping resets the engine entirely (the trend owns the direction)', FL.invalidations.trendBreak);

// ================= 5. THE MAGNET IS CAPPED AT THE KING =============================
var CAP=replay([{ bar:1, t:1, px:776, close:776, dirIn:'dn', kingK:773,
   levels:[{k:773,pct:100,isKing:true,state:'Steady'},{k:770,pct:95,state:'Building'},{k:768,pct:88,state:'Building'}] }]).st;
ok(CAP.magnet.k===773, '5a a heavy node BEYOND the King does not become the magnet — the King caps it', CAP.magnet.k);
var CAP2=replay([{ bar:1, t:1, px:776, close:776, dirIn:'dn', kingK:780,
   levels:[{k:780,pct:100,isKing:true,state:'Steady'},{k:774,pct:60,state:'Building'},{k:772,pct:35,state:'Steady'}] }]).st;
ok(CAP2.magnet.k===774, '5b with the King ABOVE in a downtrend, the strongest node below is the magnet', CAP2.magnet.k);

// ================= 6. IDLE + THE NODE-CLUSTER MEMORY ==============================
var IDLE=replay([{ bar:1, t:1, px:775, close:775, dirIn:'none', kingK:773, levels:[{k:773,pct:100,isKing:true}] }]).st;
ok(IDLE.phase==='none' && IDLE.magnet===null && IDLE.predictedPB===false,
   '6a with no confirmed trend the engine IDLES — it never invents a leg', IDLE.phase);
ok(legSentence(IDLE)==='', '6b ...and says nothing at all');
var NH=nodeHistRow({ px:775.6, levels:[{k:776,pct:45},{k:777,pct:60},{k:773,pct:100,isKing:true},{k:775,pct:8}] });
ok(NH.ceil.k===776 && NH.flr.k===773, '6c NODEHIST keeps the NEAREST meaningful ceiling above / floor below', NH.ceil.k+'/'+NH.flr.k);
ok(NH.px===775.6, '6d ...anchored to the bar\'s price');

// ================= 7. THE PERSISTED SHAPE ==========================================
ok(/gpts_nodehist_v1/.test(src), '7a NODEHIST uses a NEW localStorage key, renaming nothing');
ok(/function nodeHistSample/.test(src) && /last\.d===TODAY && last\.bar===bar/.test(ex('nodeHistSample')),
   '7b the sample is idempotent per (day, closed bar) like fcHistSample');
ok(/val\.leg=legEngine\(sym\)/.test(ex('spineOf')), '7c the engine hangs off the per-bar spine cache (one evaluation per bar)');
ok(/legBarKey\(sym\)/.test(ex('legEngine')) && !/spineBarKey/.test(ex('legEngine')),
   '7e ...keyed on the CLOSED BAR, not the live price — one node can never count as several roll steps');
ok(/already=true/.test(ex('legStep')), '7f a strike already in the roll cannot fire again (no oscillation between two nodes)');
ok(/PB_MIN_PCT=20/.test(src), '7d PB_MIN_PCT is 20% of the King, per the spec');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
