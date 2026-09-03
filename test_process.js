// ============================================================================================
// test_process.js — THE HOW IS PINNED LIKE THE WHAT. design/PROCESS.md names the loop, its files, its
//   rules and its hardening backlog; roadmap/ROADMAP.md is the incremental plan; both are registered in the
//   gex skill's load order and in the resume note, so a future context cannot miss them. (test_purpose.js
//   does the same for design/PURPOSE.md.)
// ============================================================================================
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,200):''));} };
const P='design/PROCESS.md', R='roadmap/ROADMAP.md', I='design/DASHBOARD-INVENTORY.md', S='skills/gex/SKILL.md', N='session-state/latest-resume-note.md';
ok(fs.existsSync(P) && fs.existsSync(R) && fs.existsSync(I),'h0 PROCESS.md, ROADMAP.md and DASHBOARD-INVENTORY.md exist');
const p=fs.readFileSync(P,'utf8'), r=fs.readFileSync(R,'utf8'), i=fs.readFileSync(I,'utf8'), s=fs.readFileSync(S,'utf8'), n=fs.readFileSync(N,'utf8');
ok(/the WHAT, in one line/i.test(p) && /HOD and LOD/.test(p) && /pullback/i.test(p) && /gamma node deflects price/i.test(p),'h1 PROCESS restates the WHAT (HOD/LOD, pullback, the deflection mechanism)');
['RECORD','EXPORT','PUSH','NIGHTLY','REVIEW','REGISTRY','BUILD','INSTALL','GATE','DASHBOARD','SCORE'].forEach(st=>ok(new RegExp('\\b'+st+'\\b').test(p),'h2 the loop names stage '+st));
['learning/studies.json','learning/register.json','learning/requests.json','learning/log/<day>.json','learning/suite.json','data/es-1min/SWEEPS.json','data/es-1min/SWEEPS-BOOK.json','data/<day>.json'].forEach(f=>ok(p.indexOf(f)>=0,'h3 PROCESS names '+f+' and who writes it'));
ok(/never rendered without its n/i.test(p) && /must be able to fail/i.test(p) && /read ONCE at minN/i.test(p) && /mutation-tested/i.test(p) && /rides the installer/i.test(p),'h4 the rules the machinery enforces are written down, with their tests');
ok(/tighten and harden/i.test(p) && /Score the READ/i.test(p) && /Process tab/i.test(p) && /definitions/i.test(p) && /face manifest/i.test(p),'h5 the hardening backlog is in PROCESS.md');
ok(/Dashboard = act/.test(p) && /Analysis = ask/.test(p) && /Testing = trust/.test(p),'h6 the three tabs are the loop’s three faces');
ok(/design\/PROCESS\.md/.test(s) && /roadmap\/ROADMAP\.md/.test(s) && /DASHBOARD-INVENTORY/.test(s),'h7 the gex skill’s load order names PROCESS, ROADMAP and the inventory');
ok(s.indexOf('design/PROCESS.md')<s.indexOf('design/DEPENDENCIES.md'),'h8 …PROCESS comes right after PURPOSE, before everything else');
ok(/THE WHAT AND THE HOW/.test(n) && /design\/PROCESS\.md/.test(n) && /roadmap\/ROADMAP\.md/.test(n),'h9 the resume note carries a standing WHAT-AND-THE-HOW block that points at both');
ok(/v15\.59/.test(r) && /Done when/.test(r) && /Where we are/.test(r),'h10 the roadmap is versioned, incremental, with a definition of done per item');
ok(/THE READ FROM THE STATS/.test(i) && /face manifest/i.test(i) && /MARK/.test(i) && /STATE/.test(i) && /unmeasured/i.test(i),'h11 the inventory maps every face element and names the ones that imply an unmeasured claim');
console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
