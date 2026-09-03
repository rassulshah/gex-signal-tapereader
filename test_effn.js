// (v10.54, audit 8) EFFECTIVE N — THE ONLY N THAT COUNTS.
//
// A feature is recorded once per 3-minute BAR and every record is scored over the SAME
// FEAT_FWD-bar forward window. Ten consecutive bar-records of one feature are ten
// OVERLAPPING views of one 30-minute outcome, not ten independent observations. Counting
// them as ten inflated every n on the panel by ~10x, let the RULE_UNLOCK_N gate open on
// a single afternoon, and let the promotion bar be cleared by two hours of tape.
//
// This file pins: the arithmetic, the house rendering ("n=200 bars → eff 20"), that
// RULE_UNLOCK_N and PROMO_MIN_N are compared against effN and not against raw records,
// and that no percentage is ever rendered without its n.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

global.FEAT_FWD=10;
global.RULE_UNLOCK_N=20;
global.PROMO_MIN_N=20;
eval(['effN','nTxt','pctN','_fpct','gradeMonotone'].map(ex).join('\n'));

// ================= 1. THE ARITHMETIC =========================================
ok(effN(200)===20, '1a 200 bar-records over a 10-bar window = 20 effective observations', effN(200));
ok(effN(10)===1,   '1b 10 bars = 1 effective observation — one 30-minute outcome, seen ten times', effN(10));
ok(effN(15)===2,   '1c it ROUNDS (15/10 = 1.5 -> 2), it does not truncate to zero', effN(15));
ok(effN(4)===0,    '1d fewer than half a window is 0 effective observations, not "nearly one"', effN(4));
ok(effN(0)===0 && effN(null)===0 && effN(undefined)===0 && effN(-5)===0,
   '1e missing / negative degrade to 0, never to NaN');
ok(effN(1200)===120, '1f a full multi-day sample scales linearly', effN(1200));
// the divisor is FEAT_FWD, not a magic 10
FEAT_FWD=5;
ok(effN(200)===40, '1g the divisor IS FEAT_FWD — change the window and effN follows', effN(200));
FEAT_FWD=10;

// ================= 2. THE HOUSE RENDERING ====================================
ok(nTxt(200)==='n=200 bars → eff 20', '2a every displayed n reads "n=200 bars → eff 20"', nTxt(200));
ok(nTxt(0)==='n=0 bars → eff 0', '2b ...including zero, which is stated rather than hidden', nTxt(0));
ok(/bars/.test(nTxt(37)) && /eff/.test(nTxt(37)), '2c the units are always named, so the two numbers cannot be confused', nTxt(37));
// NO % WITHOUT ITS n
ok(pctN(64, 200)==='64% (n=200 bars → eff 20)', '2d a rate is rendered WITH its n, always', pctN(64,200));
ok(/recording/.test(pctN(null, 30)) && /n=30/.test(pctN(null,30)),
   '2e a rate that does not exist says "recording" and STILL shows the n', pctN(null,30));
ok(pctN(null,0).indexOf('%')<0, '2f ...and never emits a bare % sign for a missing rate', pctN(null,0));

// ================= 3. THE GATES COMPARE AGAINST effN =========================
var FSH=ex('featureScorecardsHtml');
ok(/var en=effN\(b\.n\);/.test(FSH) && /var unlocked=\(en>=RULE_UNLOCK_N\)/.test(FSH),
   '3a the feature scorecards unlock on EFFECTIVE n');
ok(!/unlocked=\(b\.n>=RULE_UNLOCK_N\)/.test(FSH), '3b ...the raw-record comparison is gone');
var DFH=ex('dirFactorsHtml');
ok(/effN\(r\.n\)>=RULE_UNLOCK_N/.test(DFH), '3c the direction-factor table unlocks on EFFECTIVE n');
ok(/eff n/.test(DFH), '3d ...and labels the column so it cannot be misread', 'eff n');
var PCB=ex('proposalClearsBar');
ok(/loc\.effN>=PROMO_MIN_N/.test(PCB), '3e the promotion bar compares against EFFECTIVE n');
ok(/nTxt\(loc\.n\)/.test(PCB), '3f ...and the refusal states both numbers, so the shortfall is inspectable');
var RT=ex('ruleTier');
ok(/lr\.effN >= RULE_UNLOCK_N/.test(RT) && /if\(lr\.gated\) return '⚖'/.test(RT), '3g the ⚖ -> 📊 tier flip needs EFFECTIVE n too — and (v15.54) a gated feature never flips');
var RLR=ex('ruleLocalRate');
ok(/out\.effN=effN\(out\.n\)/.test(RLR), '3h the local rate carries its own effN');

// ================= 4. IT PROPAGATES INTO THE UI =============================
ok(/nTxt\(/.test(ex('deflTableHtml')), '4a the deflection tables use the house n');
ok(/effN\(/.test(ex('tabTile')), '4b the headline tiles are gated on effN');
ok(/recording — need/.test(ex('tabTile')), '4c ...and say what they are waiting for instead of showing a thin %');
ok(/nTxt\(/.test(ex('proposalsQueueHtml')), '4d the proposals queue states n the same way');
ok(/effN\(/.test(ex('unlockRowsHtml')), '4e "what unlocks when" counts effective observations');
// (v15.54) 4f pinned the question queue, archived — the register (preregHtml) reports n / minN per hypothesis instead
ok(/n\+'\/'\+H\.minN/.test(ex('preregHtml')) || /n\+' \/ '\+H\.minN/.test(ex('preregHtml')), '4f the register reports n against the minimum n per hypothesis (v15.58: as a bar, n/minN)');
// the monotone check refuses to speak on a sample of one 30-minute outcome
var thin=gradeMonotone({ A:{n:5,hit:1}, B:{n:5,hit:4}, C:{n:5,hit:5} });
ok(thin.ok===null, '4g the monotone check stays SILENT on eff n<2 in every grade — calling the fusion wrong off one outcome is the same fiat this release closes', ''+thin.ok);
var real=gradeMonotone({ A:{n:200,hit:140}, B:{n:200,hit:120}, C:{n:200,hit:100} });
ok(real.ok===true, '4h ...and speaks once the sample is real', ''+real.ok);
var broken=gradeMonotone({ A:{n:200,hit:100}, B:{n:200,hit:120}, C:{n:200,hit:140} });
ok(broken.ok===false, '4i ...including when the answer is "the fusion is wrong"', ''+broken.ok);

// ================= 5. THE EXPORT SAYS SO ====================================
ok(/effN:\{ fwd:FEAT_FWD/.test(src), '5a the day export carries the fwd window, so a consumer can re-derive effN');
ok(/effective observations = n \/ fwd/.test(src), '5b ...with the rule stated in the file itself');
ok(/EFFECTIVE observations/.test(ex('tabGuide')), '5c the tab guide explains effN where the reader meets it');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
