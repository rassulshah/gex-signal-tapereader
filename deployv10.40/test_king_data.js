// (v10.39) BATCH-1 DATA LAYER — polarity keystone + King dollars.
// Backtest findings that motivated this (2026-08-14, 324 bars / 4 days):
//   contender>=60% -> King rolled to that strike within 20 bars 76% (med 4 bars)
//   approaching-King -> 63% continue toward vs 47% receding
//   gravity gate: toward-King edge exists <=3 strikes, flips beyond
//   drift(3-bar) -> next-30m direction 50% = DESCRIPTIVE ONLY
// K$ momentum + polarity were UNTESTABLE because they were never recorded.
// This batch records them. Forward-only.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}

// ---- recNode carries pos + abs ----
global.livePctAt=function(){ return 42; };
eval(ex('recNode'));
var r1=recNode({k:780, pos:true, abs:1.25e9, role:'King', side:'above', pct:100, state:{label:'Steady'}});
ok(r1.pos===true,            'recNode records pos=true (+gamma)');
ok(r1.abs===1.25e9,          'recNode records signed/abs magnitude');
var r2=recNode({k:775, pos:false, abs:8.49e8, side:'below', pct:45});
ok(r2.pos===false,           'recNode records pos=false (-gamma)');
var r3=recNode({k:770, side:'below', pct:10});
ok(r3.pos===null && r3.abs===null, 'missing polarity -> null, never fabricated');
ok(r1.k===780 && r1.role==='King' && r1.tp===42, 'legacy fields intact');

// ---- parseKingDollarsK ----
eval(ex('parseKingDollarsK'));
ok(parseKingDollarsK('$1,252,620K')===1252620,      'parses $1,252,620K');
ok(parseKingDollarsK('$1,397,016K +4%')===1397016,  'parses with growth chip');
ok(parseKingDollarsK('$996,886K')===996886,         'parses the live bleed value');
ok(parseKingDollarsK('45%')===null,                 'plain pct -> null');
ok(parseKingDollarsK('')===null,                    'empty -> null');

// ---- SYNC GUARDS: capture is wired in ----
ok(/kingKd=kd0/.test(src) && /kingKd=kdB/.test(src), 'GUARD: King $ captured in BOTH tape paths');
ok(/rA\.kingKd=kingKd/.test(src) && /rB\.kingKd=kingKd/.test(src), 'GUARD: kingKd attached to both returns');
ok(/kd:\(function\(\)\{ try\{ var tK=tapeMap\(sym\)/.test(src), 'GUARD: snapshot records kd per bar');
ok(/pos:\(typeof r\.pos==='boolean'\?r\.pos:null\)/.test(src),  'GUARD: recNode pos capture present');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
