// v10.56 PART A — THE HANDOFF. User: "the key is identifying the dissipating old ceiling and the
// new one forming as the pullback node. you must be able to identify this shift."
// The roll is a STRENGTH transfer before it is a strike change: old ceiling bleeds (m15 Dec / off
// its peak) while a lower node above price builds. Detected BEFORE the new node qualifies as the
// PB; resolves into pbDetected; mirror for floors in an uptrend.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.PB_MIN_PCT=20; global.PB_LEG_PTS=0.5; global.PB_REACH=5; global.LEG_ROLL_SIGNAL=2; global.LEG_ROLL_CONFIRM=3;
global.HANDOFF_DEC=-8; global.HANDOFF_DROP=25; global.HANDOFF_ACM=8; global.MAP_ACM=8; global.MAP_DEC=-8; global.MAP_DROP=25; global.DEFLECT_ZONE=0.5;
global.dispIsFut=()=>false; global.futMark=()=>''; global.dispR=()=>1; global.fmtFut=String; function mul(a,b){return a/(1/b);} global.mul=mul;
eval(['fmtNum','fmtLvl','mapNodeState','legNodeDissipating','legNodeBuilding','legBlank','legClone','legStep','legVoiceRef','legVoice'].map(ex).join('\n'));
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};

// ---- pure predicates ----
ok('0a dissipating: m15 Dec <= HANDOFF_DEC', legNodeDissipating({k:771,pct:60,acm15:-9})===true);
ok('0b dissipating: >= HANDOFF_DROP off session peak', legNodeDissipating({k:771,pct:40,pctPeak:60,acm15:0})===true);
ok('0c NOT dissipating: small dec, near peak', legNodeDissipating({k:771,pct:58,pctPeak:60,acm15:-3})===false);
ok('0d building: m15 Acm >= HANDOFF_ACM', legNodeBuilding({k:769,pct:15,acm15:10})===true);
ok('0e building: a Building-state node >= PB_MIN_PCT counts; a merely-large flat node does NOT (v11.0)', legNodeBuilding({k:769,pct:25,acm15:0,state:'Building'})===true && legNodeBuilding({k:769,pct:25,acm15:0})===false);
ok('0f NOT building: thin and flat', legNodeBuilding({k:769,pct:10,acm15:2})===false);

// ---- DOWNTREND: price 770, King 768 below. Old ceiling 771 (lastPB) bleeding, 769 building above price.
const K=768;
function bar(n,px,levels){ return { dirIn:'dn', px:px, close:px, levels:levels, kingK:K, t:n*60, bar:n }; }
const KING={k:768,pct:100,isKing:true};
// bar1: 771 appears as the first PB (Building) -> lastPB=771
let st=legStep(null, bar(1,770.4,[KING,{k:771,pct:60,state:'Building',acm15:12,pctPeak:60}]));
st=legStep(st, bar(2,770.6,[KING,{k:771,pct:60,state:'Building',acm15:12,pctPeak:60}]));
ok('1a lastPB = 771 (first ceiling)', st.lastPB && st.lastPB.k===771, st.lastPB);
ok('1b no handoff while 771 is strong', !st.handoff, st.handoff);
// bar3: 771 bleeds (acm15 -10) and 769 (between price and 771) builds acm15 +9 but is still thin (pct 12) -> not yet a PB
st=legStep(st, bar(3,768.7,[KING,{k:771,pct:52,state:'Fading',acm15:-10,pctPeak:60},{k:769,pct:12,acm15:9,pctPeak:12}]));
ok('2a HANDOFF ACTIVE: 771 dissipating, 769 building', st.handoff && st.handoff.active===true, st.handoff);
ok('2b from = 771, to = 769', st.handoff && st.handoff.from.k===771 && st.handoff.to.k===769, st.handoff);
ok('2c detected BEFORE 769 qualifies as a PB (lastPB still 771, no pbDetected event this bar)', st.lastPB.k===771 && st.event!=='pbDetected' && !(st.pbDetected&&st.pbDetected.k===769), {pbd:st.pbDetected,lastPB:st.lastPB,ev:st.event});
ok('2d since = bar 3', st.handoff.since===3, st.handoff.since);
// READ voice sentence 2 while handoff is active
ok('2e READ says the handoff (user sentence #2)', legVoice(st).s==='Downtrend. Rallying down to 768. 771 ceiling dissipating and rolling down to form pullback node at 769.', legVoice(st).s);
// bar4: still active, since carries
st=legStep(st, bar(4,768.6,[KING,{k:771,pct:45,state:'Fading',acm15:-12,pctPeak:60},{k:769,pct:16,acm15:10,pctPeak:16}]));
ok('3a handoff persists; since still bar 3 (lead time accumulates)', st.handoff && st.handoff.active && st.handoff.since===3, st.handoff);
// bar5: 769 qualifies (pct 24 >= PB_MIN_PCT, Building) -> becomes the PB; handoff RESOLVES
st=legStep(st, bar(5,768.7,[KING,{k:771,pct:40,state:'Fading',acm15:-12,pctPeak:60},{k:769,pct:24,state:'Building',acm15:11,pctPeak:24}]));
ok('4a 769 becomes the PB (pbDetected 769, event pbDetected)', st.pbDetected && st.pbDetected.k===769 && st.event==='pbDetected', {pbd:st.pbDetected,ev:st.event});
ok('4b handoff RESOLVED (active false, resolved true)', st.handoff && st.handoff.active===false && st.handoff.resolved===true, st.handoff);
ok('4c leadBars = 2 (flagged at bar 3, landed at bar 5)', st.handoff && st.handoff.leadBars===2, st.handoff && st.handoff.leadBars);
ok('4d it is a ROLL step (771 -> 769, step 2 = signal)', st.roll.count===2 && st.roll.signal===true, st.roll);
ok('4e note names the transfer', /handoff resolved: 771 → 769/.test(st.note||''), st.note);
ok('4f READ: price 768.7 is IN CONTACT with the new PB 769 -> user sentence #4', legVoice(st).s==='Pulling back to resistance pullback node 769. Deflection expected to target 768 below.', legVoice(st).s);
ok('4g a resolved handoff never claims active again next bar (per-bar observation)', (function(){ var n=legStep(st, bar(6,768.7,[KING,{k:771,pct:40,acm15:-3,pctPeak:60},{k:769,pct:24,acm15:2,pctPeak:24}])); return !n.handoff; })());

// ---- NEGATIVE: bleeding old ceiling but NOTHING building lower -> no handoff (no invented shift)
let s2=legStep(null, bar(1,770.4,[KING,{k:771,pct:60,state:'Building',acm15:12,pctPeak:60}]));
s2=legStep(s2, bar(2,768.7,[KING,{k:771,pct:40,state:'Fading',acm15:-12,pctPeak:60},{k:769,pct:8,acm15:2,pctPeak:8}]));
ok('5a old ceiling bleeding + nothing building below it -> NO handoff', !s2.handoff, s2.handoff);
// building node must be BETWEEN price and `from` (a node ABOVE 771 is stacking, not a handoff)
let s3=legStep(null, bar(1,770.4,[KING,{k:771,pct:60,state:'Building',acm15:12,pctPeak:60}]));
s3=legStep(s3, bar(2,768.7,[KING,{k:771,pct:40,state:'Fading',acm15:-12,pctPeak:60},{k:773,pct:30,acm15:12,pctPeak:30}]));
ok('5b a building node ABOVE the old ceiling is not a handoff (that is stacking / rolling up)', !s3.handoff, s3.handoff);

// ---- UPTREND MIRROR: price 769.6, King 772 above. Old floor 768 bleeding, 769 building below price.
function ubar(n,px,levels){ return { dirIn:'up', px:px, close:px, levels:levels, kingK:772, t:n*60, bar:n }; }
const UK={k:772,pct:100,isKing:true};
let u=legStep(null, ubar(1,768.6,[UK,{k:768,pct:60,state:'Building',acm15:12,pctPeak:60}]));
u=legStep(u, ubar(2,768.4,[UK,{k:768,pct:60,state:'Building',acm15:12,pctPeak:60}]));
ok('6a up: lastPB = 768 floor', u.lastPB && u.lastPB.k===768, u.lastPB);
u=legStep(u, ubar(3,769.3,[UK,{k:768,pct:50,state:'Fading',acm15:-10,pctPeak:60},{k:769,pct:12,acm15:9,pctPeak:12}]));
ok('6b up: HANDOFF from 768 floor to 769 (higher floor building)', u.handoff && u.handoff.active && u.handoff.from.k===768 && u.handoff.to.k===769, u.handoff);
ok('6c up READ uses "building" (user sentence #2 mirror)', legVoice(u).s==='Uptrend. Rallying up to 772. 768 floor building and rolling up to form pullback node at 769.', legVoice(u).s);
u=legStep(u, ubar(4,769.4,[UK,{k:768,pct:40,state:'Fading',acm15:-12,pctPeak:60},{k:769,pct:24,state:'Building',acm15:11,pctPeak:24}]));
ok('6d up: resolves into PB 769, roll step 2', u.pbDetected && u.pbDetected.k===769 && u.handoff.resolved && u.roll.count===2, {pbd:u.pbDetected,ho:u.handoff});

// ---- enrollment: recorded + rules-seeded + LLM brief + spec
ok('7a leg.handoff is a registered FEATURE', /registerFeature\(\{\s*key:'leg\.handoff'/.test(src));
ok('7b rules.json seeds leg.handoff', /"id":\s*"leg\.handoff"/.test(fs.readFileSync('./learning/rules.json','utf8')));
ok('7c LLM brief evaluates leg.handoff (lead time)', /leg\.handoff[\s\S]{0,400}lead/i.test(fs.readFileSync('./docs/LLM-NIGHTLY-BRIEF.md','utf8')));
ok('7d master-spec §24 documents the handoff', /24\.1[^\n]*HANDOFF/.test(fs.readFileSync('./master-spec.md','utf8')));
console.log('test_handoff: '+p+' passed, '+f+' failed');
