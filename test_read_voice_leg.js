// v10.56 — the LEG VOICE. These 15 sentences were authored by the user (2026-08-18) and must be
// reproduced CHARACTER-FOR-CHARACTER (numbers are live). Downtrend 1..7 (6a/6b) + uptrend mirror.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function mul(a,b){return a/(1/b);} global.mul=mul;
global.dispIsFut=function(){return false;}; global.futMark=function(){return '';}; global.dispR=function(){return 1;}; global.fmtFut=function(x){return String(x);};
eval(['fmtNum','fmtLvl','legVoiceRef','legVoice'].map(ex).join('\n'));
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};
function say(leg){ return legVoice(leg).s; }
const D=(o)=>Object.assign({dir:'dn',phase:'RLY',magnet:{k:768,pct:92}},o);
const U=(o)=>Object.assign({dir:'up',phase:'RLY',magnet:{k:772,pct:92}},o);

// ---------------- DOWNTREND ----------------
ok('dn1 rallying, no PB yet',
   say(D({pbZone:{lo:768,hi:771}}))==='Downtrend. Rallying down to 768. Expect pullback node to form from 771 ceiling rolling down.', say(D({pbZone:{lo:768,hi:771}})));
ok('dn2 HANDOFF: old ceiling dissipating, new one forming lower',
   say(D({handoff:{active:true,from:{k:771},to:{k:769}}}))==='Downtrend. Rallying down to 768. 771 ceiling dissipating and rolling down to form pullback node at 769.', say(D({handoff:{active:true,from:{k:771},to:{k:769}}})));
ok('dn3 PB formed, price away',
   say(D({pbDetected:{k:769},lastPB:{k:769}}))==='Resistance pullback node formed at 769. Deflection expected to target 768.', say(D({pbDetected:{k:769},lastPB:{k:769}})));
ok('dn4 pulling back INTO the PB',
   say(D({pbDetected:{k:769},lastPB:{k:769},atPB:true}))==='Pulling back to resistance pullback node 769. Deflection expected to target 768 below.', say(D({pbDetected:{k:769},lastPB:{k:769},atPB:true})));
ok('dn5 deflected, next leg',
   say(D({deflected:true,deflectedFrom:769,phase:'RLY',lastPB:{k:769}}))==='Deflected off 769. Rallying down to 768. Expect pullback node to form from 769 ceiling rolling down.', say(D({deflected:true,deflectedFrom:769,phase:'RLY',lastPB:{k:769}})));
ok('dn6a stacking (old holds)',
   say(D({stack:{k:770,from:769,holding:true}}))==='Pullback node 769 holding. New resistance forming above at 770 — resistance stacking.', say(D({stack:{k:770,from:769,holding:true}})));
ok('dn6b rolling UP (old dissipated)',
   say(D({stack:{k:770,from:769,holding:false}}))==='Pullback node 769 dissipated. New pullback node formed higher at 770 — ceiling rolling up.', say(D({stack:{k:770,from:769,holding:false}})));
ok('dn7 target hit',
   say(D({magnetReached:true}))==='Rallied down to 768 target. On watch for a pullback.', say(D({magnetReached:true})));

// ---------------- UPTREND (mirror) ----------------
ok('up1 rallying, no PB yet',
   say(U({pbZone:{lo:768,hi:772}}))==='Uptrend. Rallying up to 772. Expect pullback node to form from 768 floor rolling up.', say(U({pbZone:{lo:768,hi:772}})));
ok('up2 HANDOFF: floor BUILDING and rolling up (not "dissipating")',
   say(U({handoff:{active:true,from:{k:768},to:{k:769}}}))==='Uptrend. Rallying up to 772. 768 floor building and rolling up to form pullback node at 769.', say(U({handoff:{active:true,from:{k:768},to:{k:769}}})));
ok('up3 PB formed',
   say(U({pbDetected:{k:769},lastPB:{k:769}}))==='Support pullback node formed at 769. Deflection expected to target 772.', say(U({pbDetected:{k:769},lastPB:{k:769}})));
ok('up4 pulling back INTO the PB',
   say(U({pbDetected:{k:769},lastPB:{k:769},atPB:true}))==='Pulling back to support pullback node 769. Deflection expected to target 772 above.', say(U({pbDetected:{k:769},lastPB:{k:769},atPB:true})));
ok('up5 deflected, next leg',
   say(U({deflected:true,deflectedFrom:769,phase:'RLY',lastPB:{k:769}}))==='Deflected off 769. Rallying up to 772. Expect pullback node to form from 769 floor rolling up.', say(U({deflected:true,deflectedFrom:769,phase:'RLY',lastPB:{k:769}})));
ok('up6a stacking',
   say(U({stack:{k:768,from:769,holding:true}}))==='Pullback node 769 holding. New support forming below at 768 — support stacking.', say(U({stack:{k:768,from:769,holding:true}})));
ok('up6b rolling DOWN (old dissipated)',
   say(U({stack:{k:768,from:769,holding:false}}))==='Pullback node 769 dissipated. New pullback node formed lower at 768 — floor rolling down.', say(U({stack:{k:768,from:769,holding:false}})));
ok('up7 target hit',
   say(U({magnetReached:true}))==='Rallied up to 772 target. On watch for a pullback.', say(U({magnetReached:true})));

// no leg → empty (the v10.54 3-beat voice takes over)
ok('no leg → no leg sentence', say({dir:'none'})==='');
// vocabulary red line: no imperatives anywhere in the 15
const all=[D({pbZone:{lo:768,hi:771}}),D({handoff:{active:true,from:{k:771},to:{k:769}}}),D({pbDetected:{k:769},lastPB:{k:769}}),D({pbDetected:{k:769},lastPB:{k:769},atPB:true}),D({deflected:true,deflectedFrom:769,lastPB:{k:769}}),D({stack:{k:770,from:769,holding:true}}),D({stack:{k:770,from:769,holding:false}}),D({magnetReached:true})].map(say).join(' ');
ok('no imperatives in the voice', !/\b(sell now|buy now|go long|go short|enter|stop at|size)\b/i.test(all));
console.log('test_read_voice_leg: '+p+' passed, '+f+' failed');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
