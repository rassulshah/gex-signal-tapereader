// (v10.40) KING ANALYZER — pure-function tests + layout guards.
// Backtest-driven design: succession 76% (n=148) HEADLINE; approach 63/47;
// gravity gate <=3 strikes; eVA outside=continuation; drift DEMOTED (50%, n=68).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}
// (v11.68) sessPhaseCT now reads the SHARED session clock instead of holding a private copy of it, so
// the extracted function needs those constants in scope. EXTRACT them from the source rather than
// restating them here — a hardcoded 30600 in this file is exactly the second copy the change removed.
const SESSC=(src.match(/var SESS_OPEN_SEC=\d+, SESS_CLOSE_SEC=\d+;/)||[])[0];
if(!SESSC) { console.log('FAIL session clock constants not found in source'); process.exit(1); }
eval(SESSC);
eval(['evaBandFromPct','successionFromPct','kingTapsCross','sessPhaseCT','kingApproach'].map(ex).join('\n'));

// ---- eVA ----
var m={'780.00':100,'779.00':7,'778.00':10,'777.00':5,'776.00':9,'775.00':45,'774.00':-1,'772.00':3};
var e1=evaBandFromPct(m);
ok(!!e1, 'eVA computes on an 8-strike map');
ok(e1.lo===775 && e1.hi===780, 'eVA band = 775-780 (top-mass strikes)');
ok(e1.cov>=0.7, 'eVA covers >=70% of mass');
ok(evaBandFromPct({'780.00':100,'775.00':45})===null, 'too few strikes -> null (never fabricated)');
ok(evaBandFromPct(null)===null, 'null map -> null');

// ---- succession ----
var s1=successionFromPct(m,780);
ok(s1.k===775 && s1.a===45, 'succession finds 775@45 (strongest non-King)');
ok(successionFromPct({'780.00':100},780)===null, 'no contender -> null');

// ---- taps / crossings / dwell ----
var bars=[
 {h:778,l:776,c:777},{h:779.4,l:777,c:779.3},{h:780.4,l:779,c:779.6}, // tap1 (touch zone)
 {h:779,l:777,c:777.5},{h:778,l:776,c:776.5},                          // away
 {h:780.6,l:779.6,c:780.4},{h:781,l:780,c:780.8},                      // tap2 + cross above
 {h:780.2,l:778.8,c:779.0}                                             // cross back below
];
var t1=kingTapsCross(bars,780,0.5);
ok(t1.taps===2, 'two tap EPISODES (consecutive touching bars = one tap) -> '+t1.taps);
ok(t1.cross===2, 'two side crossings -> '+t1.cross);
ok(t1.dwell>=4, 'dwell counts touching bars -> '+t1.dwell);

// ---- phase (CT seconds) ----
ok(sessPhaseCT(30600).ph==='OPEN',  '8:30 CT -> OPEN');
ok(sessPhaseCT(34200+60).ph==='MID','9:31 CT -> MID');
ok(sessPhaseCT(42000).ph==='LUNCH', '11:40 CT -> LUNCH');
ok(sessPhaseCT(52200).ph==='POWER', '2:30 CT -> POWER (last 30m)');
ok(sessPhaseCT(55000).ph==='CLOSED','after close -> CLOSED');
ok(sessPhaseCT(52200).toClose===30, '30 minutes to close at 2:30 CT');

// ---- approach / ETA ----
var a1=kingApproach(2.0, 3.5, 3);
ok(a1.approaching===true, 'closing distance -> approaching');
ok(a1.etaBars===4, 'ETA = ceil(2.0 / 0.5 per bar) = 4 bars');
ok(kingApproach(3.5, 2.0, 3).approaching===false, 'receding -> not approaching');
ok(kingApproach(null, 2, 3)===null, 'missing input -> null');

// ---- LAYOUT / DEMOTION GUARDS ----
ok(src.indexOf('KING PATH \\u00b7 today')===-1, 'GUARD: old header row is GONE');
ok(/kingReadHtml\(kingAnalyzer\(sym\), kv\)/.test(src), 'GUARD: narrative wired into kingBlock (top)');
ok(/gpts-kp-drift/.test(src), 'GUARD: drift chip lives INSIDE the chart overlay');
ok(/DESCRIPTIVE ONLY: 3-bar drift tested 50%/.test(src), 'GUARD: drift demotion documented at the chip');
ok(/padR=46/.test(src), 'GUARD: right gutter reserved (padR=46)');
ok(/x1="'\+\(W-padR\)\+'" y1="0"/.test(src), 'GUARD: gutter divider drawn');
// ⚠⚠ (v14.84) THE 76% WAS WITHDRAWN — it did not reproduce on 9 recorded sessions at any horizon
// (23% at 30m against the DRAWN crown). This guard used to check the claim carried its n; it now
// checks the number comes from ONE constant, because the withdrawn figure had been hand-typed into
// five separate strings and correcting it meant hunting all five.
ok(/var SUCC_META=\{/.test(src), 'GUARD: the succession numbers live in one constant');
ok(!/76% crowned/.test(src) && !/76% chance the crown/.test(src),
   'GUARD: the withdrawn 76% claim appears in no live string');
ok((src.match(/SUCC_META\./g)||[]).length>=5,
   'GUARD: every site reads that constant rather than a typed number',
   (src.match(/SUCC_META\./g)||[]).length);
ok(/withdrawn:76/.test(src), 'GUARD: ...and the withdrawn figure is KEPT and named, not erased');
ok(src.indexOf('outside value')>=0 && src.indexOf('(n=25)')>=0, 'GUARD: eVA outside-value read present with its n');
ok(/measured 4d\/324 bars/.test(src), 'GUARD: base-rate provenance line present');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
