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
// (v10.56) the handoff thresholds + the contact band the engine reads for atPB / magnetReached
global.HANDOFF_DEC=-8; global.HANDOFF_DROP=25; global.HANDOFF_ACM=8; global.DEFLECT_ZONE=0.50;
global.FUTMODE={ chart:'SPY', fam:null, underlying:'SPY', r:1, live:true, approx:false, ok:true };
eval(['mul','fmtNum','fmtFut','dispIsFut','dispR','futMark','fmtLvl',
      'legStepWord','legNodeDissipating','legNodeBuilding','legBlank','legClone','legStep',
      'legVoiceRef','legVoice','legSentence','legDecisionLine','legZoneTag',
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

// ---- the VOICE (v10.56: the user's own fifteen sentences, verbatim; the full set is
// pinned character-for-character in test_read_voice_leg.js) ----
var sent=legSentence(S);
ok(sent==='Resistance pullback node formed at 775. Deflection expected to target 773.',
   '1s the READ is the user\'s own sentence: the node that formed, and what the deflection targets', sent);
ok(legVoice(S).id==='dn3', '1t ...selected by state, not by the renderer', legVoice(S).id);
ok(!/3rd step/.test(sent) && legZoneTag(S,{k:775}).lab==='PB · 3rd lower',
   '1u the roll step is on the ROW TAG, never inside the locked sentence', legZoneTag(S,{k:775}).lab);
ok(!/magnet|sell level|resistance to sell from/i.test(sent),
   '1v ...and the sentence quotes plain levels — no jargon the user did not write', sent);
ok(!/(sell now|go short|enter|stop at|size )/i.test(sent), '1w ...and never an instruction', sent);

// the RLY/prediction variant: the pullback node has formed and price has left it
var pred=legSentence(T[3]);
ok(pred==='Resistance pullback node formed at 776. Deflection expected to target 773.',
   '1x price away from the node it just formed = sentence 3, naming the magnet as the target', pred);
ok(T[3].phase==='RLY' && T[3].pbDetected.k===776,
   '1x2 ...on a bar the engine calls RLY, with the node still standing', T[3].phase);

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
ok(us==='Support pullback node formed at 772. Deflection expected to target 774.',
   '2e the mirrored voice: SUPPORT pullback node, target above', us);
ok(legVoice(U).id==='up3', '2e2 ...the same state, mirrored', legVoice(U).id);
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
// (v10.56) a node forming AGAINST the roll is sentence 6a/6b — which one depends on the
// OLD node: still holding = stacking, dissipated = the ceiling rolling the wrong way.
ok(AG.stack && AG.stack.from===775 && AG.stack.k===776.5,
   '3d the READ knows WHICH node stacked on which', JSON.stringify(AG.stack));
ok(legSentence(AG)==='Pullback node 775 holding. New resistance forming above at 776.5 — resistance stacking.',
   '3e ...and says it in the user\'s words (old node holding = stacking)', legSentence(AG));
ok(legVoice(AG).id==='dn6a', '3f ...sentence 6a', legVoice(AG).id);
// the same bar with the old node BLEEDING is the other sentence
var AGd=replay([{ bar:8, t:8000, px:774.30, close:774.30, dirIn:'dn', kingK:773,
   levels:[{k:773,pct:100,isKing:true,state:'Steady'},{k:775,pct:50,state:'Steady',acm15:-30},{k:776.5,pct:60,state:'Building'}] }], S).st;
ok(legSentence(AGd)==='Pullback node 775 dissipated. New pullback node formed higher at 776.5 — ceiling rolling up.',
   '3g old node dissipating = sentence 6b, the ceiling rolling UP', legSentence(AGd));

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
