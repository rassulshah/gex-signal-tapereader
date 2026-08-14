// Test v10.25 Step 5: attraction-only stage + resolved-outcome echo.
// Model: accumulation ONLY attracts; NO deflect/break prediction. Outcome is a
// REPORT the BO state machine resolves over time: broke up/dn, held, false break.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }

// palette + globals the extracted fns reference
var PAL={longAccent:'#2ec27e', shortAccent:'#f0616d', gold:'#f5c518', amber:'#e0a030', blue:'#4aa3ff', sub:'#8a94a6', ink:'#e8ecf2'};
var STATE={SPY:{price:772, setups:{}}};

function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// pull AT_NODE_STRK constant too
eval(src.match(/var AT_NODE_STRK\s*=\s*[0-9.]+;/)[0]);
eval(['nodeAttraction','nodeOutcome','outcomeMarker'].map(ex).join('\n'));

// ---- nodeAttraction ----
// Building magnet ABOVE price, price emphasis 'above' (approaching) => attracting
var a1=nodeAttraction('SPY', 780, 'above', 'Building', 772, 'above');
ok(a1.stage==='attracting', 'Acm magnet approaching (not yet at) -> attracting ('+a1.stage+')');

// price sitting right on the node (within AT_NODE_STRK) => at-node
var a2=nodeAttraction('SPY', 772.3, 'above', 'Building', 772, 'above');
ok(a2.stage==='at-node' && a2.atNode===true, 'price consolidating on Acm node -> at-node ('+a2.stage+')');

// Building but price moving AWAY (emphasis 'below', node above) => no attraction stage
var a3=nodeAttraction('SPY', 780, 'above', 'Building', 772, 'below');
ok(a3.stage===null, 'Acm node but price moving away -> no stage ('+a3.stage+')');

// Fading node => never attracts (only accumulation attracts)
var a4=nodeAttraction('SPY', 780, 'above', 'Fading', 772, 'above');
ok(a4.stage===null, 'Fading node -> no attraction ('+a4.stage+')');

// ---- nodeOutcome (report from state machine) ----
function setSetup(o){ STATE.SPY.setups={ k0:Object.assign({sym:'SPY',strike:780,dir:'long',stage:'BO',voided:false,goFired:false,tokens:['BO'],updated:1},o) }; }

// still forming (BO only, not voided) => unresolved (null)
setSetup({});
ok(nodeOutcome('SPY',780)===null, 'BO-only unresolved -> null');

// clean break up (reached FT, long, not voided)
setSetup({stage:'FT', tokens:['BO','FT']});
ok(nodeOutcome('SPY',780)==='up', 'FT long not voided -> broke up');

// clean break down (short, GO, not voided)
setSetup({dir:'short', stage:'GO', goFired:true, tokens:['BO','FT','TST','CONF','GO']});
ok(nodeOutcome('SPY',780)==='dn', 'GO short -> broke down');

// held / clean reject: voided WITHOUT ever breaking (still BO when voided)
setSetup({stage:'BO', voided:true, tokens:['BO','VOID']});
ok(nodeOutcome('SPY',780)==='held', 'voided at BO (never broke) -> held');

// false break: broke (FT token) THEN voided (reversed back) = trap
setSetup({stage:'FT', voided:true, tokens:['BO','FT','VOID']});
ok(nodeOutcome('SPY',780)==='false', 'FT then voided -> false break');

// no matching setup => null (no fabricated call)
STATE.SPY.setups={};
ok(nodeOutcome('SPY',780)===null, 'no setup at strike -> null');

// wrong strike => null
setSetup({strike:769});
ok(nodeOutcome('SPY',780)===null, 'setup at different strike -> null');

// ---- outcomeMarker vocabulary ----
ok(outcomeMarker('up').txt==='broke \u2191', 'marker up = broke up-arrow');
ok(outcomeMarker('dn').txt==='broke \u2193', 'marker dn = broke down-arrow');
ok(outcomeMarker('held').txt==='held', 'marker held');
ok(outcomeMarker('false').txt==='FBO', 'marker false break -> FBO (v10.34 abbrev)');
ok(outcomeMarker(null)===null, 'no outcome -> no marker');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
