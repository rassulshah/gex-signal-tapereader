#!/usr/bin/env node
// test_v1576.js — (v15.76) G2–G5 IN THE IRT EXPORT (R-13). Operator, 2026-09-07: "i want to update the irt export so it
// exports the top 5 levels for the spx also. G1 - G5. all should be white. in addition to this, it already exports the
// kings, cw0, pw0 and the flip." Asked which five (Skylit's top-5 includes the King, and the King already has its gold
// line), he answered "Mirror Skylit: G2–G5". The BEHAVIOUR is pinned in test_irt_export.js §4t (129 assertions, 14/14
// mutants) and test_em_band.js; THIS file pins the build's shape and its record, the way every build since v15.67 does.
'use strict';
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } } return src.slice(m.index,e+1); }

// ---------- 0. the version, in both spots ----------
ok(/@version\s+15\.(7[6-9]|[89]\d)/.test(src) && /var GPTS_VERSION='15\.(7[6-9]|[89]\d)';/.test(src), '0a v15.76 or later in both spots');

// ---------- 1. the shape of the change ----------
const B=ex('irtBuildCsv');
ok(B.length>0, '1a irtBuildCsv is findable');
ok(/var spxRow=function\(k\)\{/.test(B) && /var kSpyX=spxRow\(kK\);/.test(B), '1b ONE conversion closure — the King row goes through spxRow');
ok(/gs\.push\(\{ k:spxRow\(n\.k\)/.test(B), '1c ...and so do the G rows');
ok(/Math\.abs\(b\.pct\)-Math\.abs\(a\.pct\)/.test(B), '1d ranked by |%King| — size, not sign');
ok(/if\(exK!=null && Math\.abs\(k-exK\)<0\.001\) return;/.test(B), '1e the EXPORTED King\'s strike is dropped from the ranking (no duplicate line)');
ok(/ranked\.slice\(0,4\)/.test(B) && /lbl:'G'\+\(i\+2\)/.test(B), '1f four rows, labelled G2..G5 — the King\'s slot is the gold line, no G1');
ok(/col:IRT_COLORS\.gate, w:1, style:0/.test(B), '1g white, width 1, solid — his call');
ok(/try\{ irtGLatch\(gs\); \}catch\(eGL\)\{\}/.test(B), '1h the hold is an instrument: its call sits in its own try');
ok(/var HG=null; try\{ HG=irtGHeld\(\); \}catch\(eHG\)\{\}/.test(B), '1i ...and so does the read of the hold');
ok(/IRT_LAST\.gWhy=gWhy;/.test(B), '1j IRT_LAST.gWhy reports live / held / nothing');
ok(ex('irtGLatch').length>0 && ex('irtGHeld').length>0, '1k irtGLatch / irtGHeld exist');
ok(/if\(!o \|\| o\.day!==ctTodayStr\(\) \|\| !o\[K\]/.test(ex('irtGHeld')), '1l the hold is day-scoped, like the Kings (v15.80: keyed G for ES, GQ for NQ)');
ok(/\/\^G\[2-5\]\$\/\.test/.test(ex('irtGHeld')), '1m ...and only ever returns G2..G5 rows');
ok(/THE FILE WAS THREE LINES/.test(B) && /G2\.\.G5 — the rest of Skylit's top-5/.test(B), '1n the header comment says the v14.20 contract is history and names what returned');

// ---------- 2. the record ----------
{ const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=P.roadmap.filter(r=>r.status==='next');
  ok(P.roadmap.some(r=>r.v==='15.76' && /G2–G5 IN THE IRT EXPORT/.test(r.title) && (r.status==='shipped' || nx.some(x=>x.v==='15.76'))) && P.roadmap.some(r=>r.v==='15.75' && r.status==='shipped'),
     '2a the plan: v15.75 shipped, v15.76 this build or shipped', nx.map(x=>x.v));
  ok(P.roadmap.some(r=>/candidate score/.test(r.title) && /^15\.(7[7-9]|[89]\d)$/.test(r.v)), '2b the candidate score sits after this build (v15.77 or later)');
  const seedJs=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(P), '2c PLAN_SEED equals the file');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r13=R.rows.find(r=>r.id==='R-13');
  ok(r13 && r13.status==='implemented' && r13.version==='15.76' && r13.by==='operator' && /G2–G5/.test(r13.text) && /Mirror Skylit/.test(r13.why||''),
     '2d R-13 on Rec, by operator, implemented in v15.76, his answer in why', r13&&[r13.status,r13.version]);
  const r5=R.rows.find(r=>r.id==='R-5'); ok(r5 && /v15\.(7[7-9]|[89]\d)/.test(r5.text), '2e R-5 points the score at v15.77 or later');
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '2f REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.76/.test(cl) && cl.indexOf('## v15.76')<cl.indexOf('## v15.75'), '2g the CHANGELOG has the v15.76 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.76/.test(ls.slice(logAt>=0?logAt:0)), '2h the lesson log carries the v15.76 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(7[6-9]|[89]\d)/.test(rn.slice(0,600)) && /G2–G5/.test(rn), '2i the resume note is at v15.76 or later and names the G rows');
  const dc=fs.readFileSync('session-state/DECISIONS.md','utf8'); ok(/mirrors Skylit's top-5: G2–G5/.test(dc), '2j DECISIONS records the choice and the question');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/G2–G5/.test(cfg.theWhatAndTheHow.irtExport||'') && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1576.js')>=0, '2k .gex-config.json names the export and this test');
}

console.log('test_v1576: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
