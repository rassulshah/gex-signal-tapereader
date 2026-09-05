// ============================================================================================
// test_v1569.js — (v15.69) THE OBJECTIVE OUTCOMES + THE LEARN TAB CARRIES THE RECORD. Operator, 2026-09-04: "the idea is
//   to have a trading decision support system that is data driven … everything that is displayed on the dashboard" · "the
//   learn tab should also be updated" · "the entire data, analysis and testing process results in learning and it is from
//   the learning that can know something" · "reflect on the entire approach … and then build".
//   Executed, not grepped: the outcomes on a synthetic day and on the recorded day, the columns, the rules' verdicts, the
//   seed's merge, the panel's table and rule rows, the plan.
// ============================================================================================
const fs=require('fs'); const cp=require('child_process'); const os=require('os'); const path=require('path');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,500):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const esc=s=>String(s==null?'':s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
const PAL={ ink:'#e6edf3', sub:'#8b98a5', gold:'#e3c341', line:'#2a3140', card:'#161b22', blue:'#5ea4ff', longAccent:'#2ec27e', shortAccent:'#f0616d' };
const py=(args,opts)=>{ try{ return { out:cp.execSync('python3 '+args,Object.assign({encoding:'utf8',stdio:['ignore','pipe','pipe']},opts||{})), code:0 }; }catch(e){ return { out:String(e.stdout||''), err:String(e.stderr||''), code:e.status }; } };

ok(/@version\s+15\.(69|[7-9]\d)/.test(src) && /var GPTS_VERSION='15\.(69|[7-9]\d)';/.test(src),'0a v15.69+ in both spots');

// ---- 1 · the outcomes: pre-registered, on a synthetic day, and on the recorded day --------------------------------------
{
  const st=py('tools/nightly/patterns.py --selftest');
  ok(st.code===0 && /patterns\.py selftest ok/.test(st.out),'1a patterns.py --selftest: the LOD tap turns (1) and resumes (1); a mid-day floor 0/1; a retest None; the HOD ceiling 1/0; a ceiling under the HOD 0/0; no time → None; a thin record → None',st.out.slice(-300)+(st.err||''));
  const pt=fs.readFileSync('tools/nightly/patterns.py','utf8');
  ok(/THE OBJECTIVE OUTCOMES \(v15\.69\) — written here BEFORE the first number was read \(2026-09-04\)/.test(pt) && /TURN_TOL = 0\.5/.test(pt) && /MIN_BARS = 20/.test(pt) && /only the FIRST tap per \(strike, dir\) of a day/.test(pt),'1b the definitions are written in the file before the numbers: TURN_TOL 0.50 SPY, 20 bars minimum, first tap per node per day');
  const log=JSON.parse(fs.readFileSync('learning/log/2026-09-03.json','utf8')); const R={}; log.patterns.rows.forEach(r=>{ R[r.key]=r; });
  ok(log.patterns.outcomes && log.patterns.outcomes.turnTol===0.5 && log.patterns.outcomes.scored===19 && R['all'].turn && R['all'].turn.n===19 && R['all'].turn.hit===1 && R['all'].resume.n===19 && R['all'].resume.hit===13,'1c THE FIRST READ, 2026-09-03 (one day, 19 episodes, old names): the tap was the session’s turn 1 of 19; the trend resumed after 13 of 19 — a node tap is rarely THE turn, the base every class must beat',log.patterns.outcomes);
  ok(R['dir:up'].turn.n===12 && R['dir:dn'].turn.n===7 && R['all'].turn.n===R['dir:up'].turn.n+R['dir:dn'].turn.n,'1d the episodes split by direction add up: 12 floors + 7 ceilings = 19',[R['dir:up'].turn,R['dir:dn'].turn]);
}

// ---- 2 · results.py: the lines carry turn / resume; the rules get the record's verdict -------------------------------------
const LOG={ date:'2026-09-09', patterns:{ rows:[
  {key:'all',label:'every tap',n:51,held:24,broke:27,pending:2,rate:47,lo:34,turn:{n:40,hit:3,rate:8,lo:2},resume:{n:40,hit:26,rate:65,lo:50}},
  {key:'king:any',label:'K',n:20,held:9,broke:11,pending:0,rate:45,lo:26,turn:{n:18,hit:2,rate:11,lo:2},resume:{n:18,hit:13,rate:72,lo:49}},
  {key:'spx:grow',label:'g',n:20,held:15,broke:5,pending:0,rate:75,lo:53,turn:{n:18,hit:2,rate:11,lo:2},resume:{n:18,hit:12,rate:67,lo:44}},
  {key:'spx:fade',label:'f',n:16,held:6,broke:10,pending:0,rate:38,lo:18,turn:{n:15,hit:1,rate:7,lo:0},resume:{n:15,hit:9,rate:60,lo:36}},
  {key:'spx:new',label:'n',n:17,held:10,broke:7,pending:0,rate:59,lo:36,turn:{n:16,hit:5,rate:31,lo:14},resume:{n:16,hit:11,rate:69,lo:44}},
  {key:'spx:pos',label:'p',n:30,held:18,broke:12,pending:0,rate:60,lo:42,turn:{n:25,hit:2,rate:8,lo:1},resume:{n:25,hit:17,rate:68,lo:48}},
  {key:'spx:neg',label:'q',n:20,held:13,broke:7,pending:0,rate:65,lo:43,turn:{n:15,hit:1,rate:7,lo:0},resume:{n:15,hit:9,rate:60,lo:36}},
  {key:'spx:pika',label:'k',n:9,held:5,broke:4,pending:0,rate:56,lo:27,turn:{n:8,hit:2,rate:25,lo:7},resume:{n:8,hit:5,rate:63,lo:31}} ] }, hypotheses:[] };
{
  const st=py('tools/nightly/results.py --selftest');
  ok(st.code===0 && /results\.py selftest ok · 10 studies answered · 3 rules judged/.test(st.out),'2a results.py --selftest: the lines carry turn / resume; L1 agrees, L5 contradicts, L2 agrees, L6 thin, L9 measured, L3 / L7 not measured',st.out.slice(-200)+(st.err||''));
  const tmp=path.join(os.tmpdir(),'gpts-log69-'+process.pid+'.json'); fs.writeFileSync(tmp, JSON.stringify(LOG));
  const r=py('tools/nightly/results.py --json "'+tmp+'"'); let J=null; try{ J=JSON.parse(r.out); }catch(e){} try{ fs.unlinkSync(tmp); }catch(e){}
  ok(J && J.results['S0.1'] && J.results['S0.1'].line==='any King held 9 / 20 = 45% (low 26%) · turn 2 / 18 = 11% (low 2%) · resume 13 / 18 = 72% (low 49%) · nightly 2026-09-09','2b an Analysis row’s line: held · turn · resume, each with n and the Wilson low',J&&J.results['S0.1']);
  const RL=(J&&J.rules)||{};
  ok(RL.L1 && RL.L1.verdict==='agrees' && /^held: growing 15 \/ 20 = 75% \(low 53%\) vs fading 6 \/ 16 = 38% \(low 18%\) — agrees \(growing into the tap holds more than fading\) · nightly 2026-09-09$/.test(RL.L1.evidence),'2c L1 (growing into the tap): the record agrees — 75% vs 38%, both at n ≥ 15',RL.L1);
  ok(RL.L5 && RL.L5.verdict==='contradicts' && /^held: a −γ node 13 \/ 20 = 65% \(low 43%\) vs a \+γ node 18 \/ 30 = 60% \(low 42%\) — contradicts/.test(RL.L5.evidence),'2d L5 (a −γ node holds less): the record CONTRADICTS on this fixture — a taught rule can be wrong, and the tab must say so',RL.L5);
  ok(RL.L2 && RL.L2.verdict==='agrees' && /^turn: a NEW node 5 \/ 16 = 31% \(low 14%\) vs every tap 3 \/ 40 = 8% \(low 3%\) — agrees/.test(RL.L2.evidence),'2e L2 (a fresh node at the extreme): judged on TURN, not held — 31% vs 8% of every tap',RL.L2);
  ok(RL.L6 && RL.L6.verdict==='thin' && /^turn: pika stack \+ barney stack 2 \/ 8 \(thin\) vs every tap 3 \/ 40/.test(RL.L6.evidence) && RL.L9 && RL.L9.verdict==='measured' && RL.L3 && RL.L3.verdict==='not measured' && /King path/.test(RL.L3.evidence) && RL.L7.verdict==='not measured',['2f L6 thin under 15; L9 a base row → measured; L3 / L8 / L4 / L7 not measured, with the reason'].join(''),[RL.L6&&RL.L6.verdict,RL.L9&&RL.L9.verdict,RL.L3&&RL.L3.evidence]);
  // apply_rules against a temp Learn doc + the seed's merge
  const dir=fs.mkdtempSync(path.join(os.tmpdir(),'gpts-learn-')); fs.mkdirSync(path.join(dir,'learning','deflections'),{recursive:true}); fs.mkdirSync(path.join(dir,'learning','log'),{recursive:true});
  fs.writeFileSync(path.join(dir,'learning','deflections','examples.json'), JSON.stringify({schema:1, rules:[{id:'L1',rule:'r',status:'PROPOSED'},{id:'L5',rule:'r',status:'PROPOSED'},{id:'L99',rule:'r',status:'PROPOSED'}], examples:[]}));
  fs.writeFileSync(path.join(dir,'learning','studies.json'), JSON.stringify({schema:1,subjects:[]})); fs.writeFileSync(path.join(dir,'learning','log','2026-09-09.json'), JSON.stringify(LOG));
  const w=py('- <<\'EOF\'\nimport sys, json; sys.path.insert(0, "tools/nightly"); import results\nresults.write('+JSON.stringify(dir)+')\nEOF');
  const L2=JSON.parse(fs.readFileSync(path.join(dir,'learning','deflections','examples.json'),'utf8')); const RJ=JSON.parse(fs.readFileSync(path.join(dir,'learning','results.json'),'utf8'));
  ok(w.code===0 && RJ.rules && RJ.rules.L1 && L2.rules[0].verdict==='agrees' && L2.rules[0].status==='PROPOSED' && L2.rules[0].asOf==='2026-09-09' && L2.rules[1].verdict==='contradicts' && L2.rules[1].status==='PROPOSED' && !('verdict' in L2.rules[2]),'2g write() patches the Learn doc’s rules (evidence · verdict · asOf), NEVER their status (an agreeing PROPOSED rule stays PROPOSED — the review promotes); an unknown rule is untouched; results.json carries `rules`',w.err||[L2.rules.map(x=>[x.id,x.verdict,x.status])]);
  const ls=fs.readFileSync('tools/learn-seed.py','utf8');
  ok(/_results\.apply_rules\(DOC, _rj\.get\("rules"\) or \{\}\)/.test(ls) && /the record \(nightly %s\): \*\*%s\*\* — %s/.test(ls),'2h tools/learn-seed.py merges the record’s verdicts when it regenerates, and LEARNING.md prints them under each rule');
  const LD=JSON.parse(fs.readFileSync('learning/deflections/examples.json','utf8')); const seed=JSON.parse(/var LEARN_SEED=(\{.*?\});\n/.exec(src)[1]);
  ok(LD.rules.every(x=>x.verdict) && LD.rules.find(x=>x.id==='L3').verdict==='not measured' && JSON.stringify(seed)===JSON.stringify(LD),'2i the committed Learn doc: every rule carries a verdict (all thin / not measured on 09-03); LEARN_SEED = examples.json',LD.rules.map(x=>[x.id,x.verdict]));
}

// ---- 3 · the panel: the table's columns, the rule rows -----------------------------------------------------------------------
{
  const g={ RATE_MIN_N:15, PAL, g3esc:esc, tabEmpty:t=>'<div class="empty">'+t+'</div>' };
  const f=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+ex('objCell')+'\n'+ex('patternRowsHtml')+'\nreturn patternRowsHtml;')(g);
  const T={ rows:LOG.patterns.rows, events:53, stamped:0, days:1, outcomes:{turnTol:0.5, scored:40} };
  const h=f(T,'the day files (2026-09-09)');
  ok(/· 40 scored for turn \/ resume<\/div>/.test(h) && /<td style="text-align:right" title="[^"]*the turn">turn<\/td><td style="text-align:right" title="[^"]*\(stay in\)">resume<\/td><\/tr>/.test(h),'3a the nightly’s table gains turn and resume columns, with the definitions in the headers’ hovers',h.slice(0,600));
  ok(/every tap<\/td>[\s\S]*?<td style="text-align:right;color:#f0616d;font-weight:700;white-space:nowrap">8% <i[^>]*>≥2% · 3\/40<\/i><\/td><td style="text-align:right;color:#2ec27e;font-weight:700;white-space:nowrap">65% <i[^>]*>≥50% · 26\/40<\/i><\/td><\/tr>/.test(h),'3b every tap: turn 8% (red) · resume 65% (green), each with its low and hit/n',h.match(/every tap<\/td>[\s\S]*?<\/tr>/)[0]);
  ok(/<td style="text-align:right;color:#8b98a5;font-weight:700;white-space:nowrap">2\/8 <i[^>]*>thin<\/i><\/td>/.test(h),'3c under 15 the cell says hit/n thin',null);
  const h2=f({ rows:[{key:'all',label:'every tap',n:5,held:3,broke:2,pending:0,rate:60,lo:23}], events:5, stamped:5 },'this browser’s ledger');
  ok(!/turn<\/td>/.test(h2) && !/resume<\/td>/.test(h2) && !/scored for turn/.test(h2),'3d the live table (no whole day) shows held only — no columns, no caption',h2);
  ok(/The nightly\\u2019s table adds the two OBJECTIVE outcomes \(PURPOSE\.md\): turn = the tap\\u2019s extreme within 0\.50 SPY/.test(ex('patternScoresHtml')) && /The live table has no whole day and shows held only/.test(ex('patternScoresHtml')),'3e the footnote defines turn and resume and says the live table has neither');
  // the Learn rule rows
  const lb=ex('learnBlock');
  ok(/if\(r\.verdict\)\{ var vc=\{ agrees:PAL\.longAccent, contradicts:PAL\.shortAccent, thin:'#7cc7ff', measured:'#7cc7ff' \}\[r\.verdict\]\|\|PAL\.sub;/.test(lb) && /\\u27f3 the record'\+\(r\.asOf\?\(' \('\+g3esc\(r\.asOf\)\+'\)'\):''\)\+': <b style="color:'\+vc\+'">'\+g3esc\(r\.verdict\)\+'<\/b> <span class="ci">'\+g3esc\(r\.evidence\|\|''\)\+'<\/span><\/div>'/.test(lb),'3f each Learn rule renders the record’s verdict — agrees green, contradicts red, thin / measured blue — with the evidence line');
  ok(/a taught rule the record contradicts is the first thing to re-teach/.test(lb),'3g the section note says what a contradiction means');
  const P=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); const plan=JSON.parse(fs.readFileSync('learning/plan.json','utf8'));
  const r69=P.roadmap.find(r=>r.v==='15.69'); const later=P.roadmap.find(r=>/deflection candidate score/.test(r.title));
  ok(r69 && /THE OBJECTIVE OUTCOMES/.test(r69.title) && ['next','shipped'].includes(r69.status) && P.roadmap.some(r=>r.v==='15.68' && r.status==='shipped') && later && parseFloat(later.v)>15.69 && JSON.stringify(P)===JSON.stringify(plan) && P.system.components.some(c=>c.id==='nightly' && /turn \/ resume/.test(c.does) && /the Learn tab's rules/.test(c.does)),'3h the plan: v15.68 shipped, v15.69 = the objective outcomes, the candidate score after it; the nightly component names turn / resume and the Learn rules',[r69&&r69.status, later&&later.v]);
}

// ---- 4 · the records ---------------------------------------------------------------------------------------------------------
{
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8');
  ok(/## v15\.69/.test(cl) && cl.indexOf('## v15.69')<cl.indexOf('## v15.68'),'4a the CHANGELOG has the v15.69 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG');
  ok(/### v15\.69/.test(ls.slice(logAt)),'4b the lesson log carries the v15.69 entry');
  const fd=fs.readFileSync('skylit-docs/FINDINGS.md','utf8');
  ok(/## F-19/.test(fd) && /1 of 19/.test(fd) && /13 of 19/.test(fd),'4c FINDINGS F-19: the first read of the objective outcomes, one day, with its n');
  const pu=fs.readFileSync('design/PURPOSE.md','utf8');
  ok(/data-driven/.test(pu) && /the learn tab should also be updated/.test(pu) && /results in learning/.test(pu),'4d PURPOSE.md carries his three statements of the evening');
}

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
