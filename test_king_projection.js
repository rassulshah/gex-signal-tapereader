// (v10.42) PROJECTION + FEEDBACK LOOP — pure tests + wiring guards.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);
  let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}
  return src.slice(m.index,e+1);}
// (v14.84) kingProjection now reads SUCC_META for the succession basis string — the one constant
// that replaced the withdrawn 76% typed into five places. Pull the declaration in with the
// functions, so the harness exercises the SAME numbers the panel ships rather than a stub.
eval((/var SUCC_META=\{[\s\S]*?\n\};/.exec(src)||[''])[0]);
eval(['fmtKd','fmtChg','kingProjection','projRecs'].map(ex).join('\n'));

// ---- the two display bugs from the live screenshot ----
ok(fmtKd(1125690)==='$1.13B',  'K$ unit fixed: 1,125,690K -> $1.13B (was "$1125.69B")');
ok(fmtKd(996886)==='$997M',    '$K -> $M below a billion');
ok(fmtKd(850)==='$850K',       'small values stay $K');
ok(fmtChg(-7)==='▼7%',         'double-sign fixed: -7 -> ▼7% (was "▼-7%")');
ok(fmtChg(12)==='▲12%',        'positive change ▲12%');

// ---- projection core (pure) ----
var A={ok:true, king:775, px:776.4, dist:-1.4, adist:1.4, grav:true, pol:true,
       succ:{k:780,a:65}, succHot:true, appr:{approaching:true, etaBars:9, rate:-0.15},
       eva:{lo:775,hi:780}, inVA:true, phase:'MID', toClose:69, over:false};
var ctx={gateK:776, hod:779.4, lod:771.2, avgRange:0.42, phase:'MID', toClose:69, evaLo:775, evaHi:780};
var P=kingProjection(A, ctx);
ok(P.projKing===780,           'succession hot -> PROJECTED King 780');
ok(P.projSrc==='succession',   'projection source recorded');
ok(P.tgt===780,                'target = projected King (T2 cap at the crown)');
ok(P.t1===null || P.t1===776 ? P.t1!==null && P.t1===776 || (776>A.px)===false : false, 'T1 gate logic evaluated');
// gate 776 is between px 776.4 and tgt 780? px<tgt, gate must be >px: 776<776.4 -> NOT between -> t1 null
ok(P.t1===null,                'gate below price on an upward path -> no T1 (must sit between)');
ok(P.rails.some(r=>r.label==='T2 · King'&&r.k===780), 'rails include T2 King');
ok(P.rails.some(r=>r.label==='HOD'),  'rails include HOD');
ok(P.rails.some(r=>r.cls==='eva'),    'rails include eVA edges');
ok(P.etaBars===Math.ceil(Math.abs(780-776.4)/0.15), 'ETA = dist/rate bars toward the TARGET');
ok(P.cone.half(4)>P.cone.half(1),     'cone widens with bars (sqrt scaling)');
ok(P.pin===false,              'no pin band outside POWER phase');
var P2=kingProjection({...A, succHot:false, succ:null, adist:1.0}, {...ctx, phase:'POWER'});
ok(P2.projKing===null && P2.tgt===775, 'no hot succession -> crown projected to hold');
ok(P2.pin===true,              'POWER + proximity -> pin band on');
ok(kingProjection({ok:false},ctx)===null, 'not-ok analyzer -> null (never invent)');

// ---- recommendations engine (the feedback loop) ----
var recs1=projRecs({covN:30,cov:15, etaErrs:new Array(20).fill(9), etaMed:9, succN:12,succ:5, pinN:6,pin:2, reachN:25,reach:20, days:9});
ok(recs1.some(r=>r.sev==='high'&&/Cone too narrow/.test(r.t)),   'coverage 50% -> widen-cone recommendation');
ok(recs1.some(r=>/ETA runs late/.test(r.t)),                     'ETA median +9m -> rate-window recommendation');
// ⚠⚠ (v14.84) THIS FIXTURE WAS BUILT AROUND THE WITHDRAWN 76%. 5/12 = 42% used to be "under the
// backtest" and raised a high-severity warning. Against the MEASURED 23% (SUCC_META.hz30), 42% is
// comfortably ABOVE expectation and warning about it would be crying wolf. The threshold moved, so
// the fixture has to move with it — otherwise the test pins the old, wrong bar.
ok(!recs1.some(r=>r.sev==='high'&&/Succession hit/.test(r.t)),
   'succession at 42% is ABOVE the measured 23% — no HIGH warning, and that is the fix not the bug',
   recs1.filter(r=>/Succession/.test(r.t)).map(r=>r.sev+':'+r.t.slice(0,40)));
ok(!/n=148 backtest/.test(src), '...and no live string still calls the withdrawn figure "the backtest"');
var recsLo=projRecs({covN:30,cov:28, etaErrs:new Array(20).fill(1), etaMed:1, succN:12,succ:2, pinN:6,pin:5, reachN:25,reach:22, days:9});
ok(recsLo.some(r=>r.sev==='high'&&/Succession hit 17%/.test(r.t)),
   '...and a genuinely low 17% DOES raise it', recsLo.filter(r=>/Succession/.test(r.t)).map(r=>r.t));
ok(recsLo.some(r=>/node STATE split/.test(r.t)),
   '...pointing at the state split, not at the 60% size threshold — size is the weakest of the three');
var recs2=projRecs({covN:3,cov:2, etaErrs:[], etaMed:null, succN:2,succ:2, pinN:1,pin:1, reachN:4,reach:3, days:1});
ok(recs2.every(r=>r.sev==='info'),  'insufficient n -> only honest "recording" lines, no fabricated tuning');

// ---- wiring guards ----
ok(/proj:projSnapshotRecord\(sym\)/.test(src),      'GUARD: projection recorded per bar in the snapshot');
// (v10.54 GROUP 5) the Analysis tab was reorganised into seven question-led sections;
// the projection scorecard now lives inside the collapsible DETAIL section.
ok(/legacy\+=projScorecardHtml\(\)/.test(src),      'GUARD: 🎯 scorecard renders in the Analysis tab (DETAIL section)');
ok(/DETAIL · every enrolled feature/.test(src),      'GUARD: ...and that section exists and is named');
ok(/projReview:\(function/.test(src),               'GUARD: scorecard+recs ride in the day export (EOD LLM review)');
ok(/__gptsDebug\.projReport=/.test(src),            'GUARD: projReport() debug hook exposed');
ok(/PROJECTION SCORECARD/.test(src),                'GUARD: scorecard section titled');
ok(/RECOMMENDATIONS \(auto\)/.test(src),            'GUARD: auto recommendations section present');
ok(/projChartHtml\(__A2, kingProjectionLive/.test(src), 'GUARD: projected chart wired under the King Path');
// tooltips everywhere: count title= occurrences in the console + projection code
var seg=src.slice(src.indexOf('function kingReadHtml'), src.indexOf('function kingBlock'));
ok((seg.match(/tile\('/g)||[]).length>=8 && (seg.match(/title=/g)||[]).length>=3, 'GUARD: console carries hover explanations (8 tiles + section titles)');
var seg2=src.slice(src.indexOf('function projChartHtml'), src.indexOf('function projSnapshotRecord'));
ok((seg2.match(/tt\(/g)||[]).length>=7, 'GUARD: projection SVG elements carry native hover tooltips (7+ tt() call sites)');
ok(!/▼'-/.test(src) && /fmtChg/.test(src),          'GUARD: no double-signed change strings');


// ---- (v10.43) rendering fixes ----
eval(ex('projTaperHalf'));
var hf=function(b){ return 0.3*Math.sqrt(Math.max(1,b)); };
ok(projTaperHalf(4,9,20,hf)===hf(4),          'taper: pre-ETA cone follows sqrt growth');
ok(projTaperHalf(9,9,20,hf)===hf(9),          'taper: at ETA = full width');
ok(projTaperHalf(20,9,20,hf)<hf(9),           'taper: POST-ETA the cone NARROWS (black-wedge fix)');
ok(projTaperHalf(20,9,20,hf)>=0.5,            'taper: floor 0.5 = the pin range');
ok(projTaperHalf(15,null,20,hf)===hf(10),     'no ETA: growth capped at the 10-bar envelope');
ok(/inRails/.test(src) && /edgeHi/.test(src), 'GUARD: focused domain with edge-tag rails');
ok(/▲ /.test(src),                            'GUARD: above-window rails render as ▲ edge tag');
ok(/placed\.push\(ly\)/.test(src),            'GUARD: rail-label anti-collision nudging');
ok(/var MINGAP=15/.test(src),                 'GUARD: history-gutter min gap raised to 15px');
// ⚠ (v15.21) SCOPED TO THE TILES. This searched the WHOLE FILE for the ellipsis declaration, so it
// failed the moment an unrelated component — the ladder's new column headers, where truncating a
// label IS the right behaviour — used the same three properties. A guard about how KING PROJECTION
// TILES behave has to read the tiles' own CSS; a file-wide grep makes one component's rule a law
// for every component, and the failure it produces points at the wrong place entirely.
{
  const tileCss=(src.match(/\.g3kptile\{[^}]*\}/g)||[]).join(' ') +
                (src.match(/\.g3kpt[a-z]*\{[^}]*\}/g)||[]).join(' ');
  ok(tileCss.indexOf('text-overflow:ellipsis')===-1,
     'GUARD: tiles wrap instead of ellipsizing', tileCss.slice(0,120));
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
