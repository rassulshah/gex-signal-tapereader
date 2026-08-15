// (v10.43) BO-tag fix (user-reported) + King Path significant-move labels.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}

// ---- kingPathSigMoves (pure) ----
eval(ex('kingPathSigMoves'));
var M=60000, t0=0;
var pts=[
 {t:0,      k:780},
 {t:3*M,    k:778},   // step 2 -> significant
 {t:6*M,    k:777},   // step 1, dwell 3m -> NOT significant
 {t:9*M,    k:776},   // step 1 but holds 20m -> significant (dwell)
 {t:29*M,   k:775},   // last vertex -> excluded (gutter labels it)
];
var sig=kingPathSigMoves(pts, 40*M);
ok(sig.some(s=>s.k===778 && s.step===2),        'step >=2 strikes labeled (778)');
ok(!sig.some(s=>s.k===777),                     'small quick wiggle NOT labeled (777)');
ok(sig.some(s=>s.k===776 && s.dwellM>=15),      'long dwell labeled (776, 20m hold)');
ok(!sig.some(s=>s.i===pts.length-1),            'last vertex excluded — gutter 👑 owns it');
var many=[{t:0,k:700}]; for(let i=1;i<=9;i++) many.push({t:i*3*M,k:700+i*2}); many.push({t:40*M,k:730});
ok(kingPathSigMoves(many, 60*M).length<=5,      'label count capped at 5');

// ---- setupTagForNode: terminal setups no longer haunt the strike ----
global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d'};
global.fmtNum=function(x){return ''+x;};
global.BO_HL_LOOKBACK=14;
global.sym='SPY';
global.STATE={SPY:{setups:{
  old_go:  {strike:776, dir:'long', stage:'GO', outcome:'T2',  voided:false, ts:100, updated:200},
  dead:    {strike:776, dir:'long', stage:'GO', outcome:'FAILED', voided:false, ts:150, updated:250},
  live_ft: {strike:776, dir:'long', stage:'FT', outcome:null,  voided:false, ts:900, updated:950}
}}};
eval(ex('setupTagForNode'));
var tag=setupTagForNode(776);
ok(/BO·FT/.test(tag),                       'live BO·FT setup renders BO·FT');
var chainTxt=(tag.match(/>([A-Z·]+)<\/span>$/)||[])[1]||'';
ok(chainTxt==='BO·FT',                          'chip text is exactly BO·FT — finished GO setups no longer paint the full chain');
global.STATE={SPY:{setups:{ only_done:{strike:776,dir:'long',stage:'GO',outcome:'T2',voided:false,ts:1,updated:2} }}};
ok(setupTagForNode(776)==='',                    'all setups terminal -> no chip (outcome echo covers it)');
global.STATE={SPY:{setups:{
  older_go:{strike:776,dir:'long',stage:'GO',outcome:null,voided:false,ts:100,updated:100},
  newer_bo:{strike:776,dir:'long',stage:'BO',outcome:null,voided:false,ts:500,updated:500}
}}};
ok(/>BOw</.test(setupTagForNode(776)),           'two LIVE setups -> the MOST RECENT wins (BOw pre-FT, v10.44 vocabulary), not the max stage');

// ---- guards ----
ok(/s\.outcome==='T2'\|\|s\.outcome==='FAILED'\|\|s\.outcome==='EXPIRED'/.test(src), 'GUARD: terminal outcomes skipped');
ok(/var tSel=\(s\.updated\|\|s\.ts\|\|0\)/.test(src), 'GUARD: recency selection');
ok(/kingPathSigMoves\(pts, xMax\)/.test(src),    'GUARD: sig-move labels wired into the sparkline');
ok(/gx-lastLabX<22/.test(src),                   'GUARD: x-collision skip for crowded labels');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
