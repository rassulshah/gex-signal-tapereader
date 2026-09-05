// ============================================================================================
// test_v1570.js — (v15.70) 💡 REC. Operator, 2026-09-04: "we need one other tab called recommendations. based on the entire
//   process and what you have learned you need to make recommendations and get my approval to implement." · "lets abbreviate
//   as Rec" · "from that point on you take over from data, analysis, testing, learning all the way to the Rec tab".
//   Executed, not grepped: the machine's conditions on a fixture, the merge (three writers, by id), his decisions riding the
//   day file, the withdrawals, the panel's rows and sections, the ✓ / ✗, the export, the seed.
// ============================================================================================
const fs=require('fs'); const cp=require('child_process'); const os=require('os'); const path=require('path');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,500):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const esc=s=>String(s==null?'':s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
const PAL={ ink:'#e6edf3', sub:'#8b98a5', gold:'#e3c341', line:'#2a3140', card:'#161b22', blue:'#5ea4ff', longAccent:'#2ec27e', shortAccent:'#f0616d' };
const py=(args,opts)=>{ try{ return { out:cp.execSync('python3 '+args,Object.assign({encoding:'utf8',stdio:['ignore','pipe','pipe']},opts||{})), code:0 }; }catch(e){ return { out:String(e.stdout||''), err:String(e.stderr||''), code:e.status }; } };

ok(/@version\s+15\.(7\d|[89]\d)/.test(src) && /var GPTS_VERSION='15\.(7\d|[89]\d)';/.test(src),'0a v15.70 or later in both spots');

// ---- 1 · the nightly's side: conditions, merge, decisions, withdrawals ----------------------------------------------------------
{
  const st=py('tools/nightly/recommend.py --selftest');
  ok(st.code===0 && /recommend\.py selftest ok · 4 machine rows on the fixture/.test(st.out),'1a recommend.py --selftest: a class clear of the base → RULE; a contradicted rule → TEACH; a cleared hypothesis → RULE; grow (low < base), old:*, dir:*, a thin class → nothing; decisions, the latest per id; withdrawals and returns; the seed regenerates without touching his status',st.out.slice(-300)+(st.err||''));
  const rc=fs.readFileSync('tools/nightly/recommend.py','utf8');
  ok(/THE MACHINE'S CONDITIONS — written here BEFORE any row was generated \(2026-09-04\)/.test(rc) && /if lo <= br:\n\s+continue/.test(rc) && /if n < RATE_MIN_N or bn < RATE_MIN_N:/.test(rc),'1b the conditions are written in the file before any row; a RULE needs the Wilson low ABOVE the base at n ≥ 15 on both');
  const run=fs.readFileSync('tools/nightly/run.py','utf8');
  ok(/_rc\.update\(ROOT, log, None, days\)/.test(run),'1c run.py calls recommend.update after results — the Rec file follows every 💾');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8'));
  const ids=R.rows.map(r=>r.id);
  ok(R.schema===1 && ['R-1','R-2','R-3','R-4','R-5','R-6'].every(i=>ids.includes(i)) && R.rows.filter(r=>r.by==='review').length===6 && R.rows.filter(r=>r.by==='review').every(r=>r.status==='proposed') && R.counts.proposed===6 && !R.rows.some(r=>r.by==='nightly'),'1d the committed file: the review’s six rows, all proposed, no machine row yet (no class is clear of the base on 09-03); (v15.71) R-7, his own ask, rides as implemented',R.counts);
  ok(R.rows.every(r=>r.text && r.changes && r.evidence && r.kind && r.asOf) && !R.rows.some(r=>/illustrative/i.test(r.text)),'1e every row carries text, changes, evidence, kind, asOf — and the illustrative mockup row is NOT in the real file');
  const seed=fs.readFileSync('tools/rec-seed.py','utf8');
  ok(/R\.merge\(doc, ROWS, None, \{\}, doc\.get\('asOf'\) or '2026-09-04', \[\]\)/.test(seed) && /his decisions survive a regeneration/.test(seed),'1f the review’s seed merges by id and never touches his status');
  // an end-to-end decision: a day file with `reco` → the file's status, through update()
  const dir=fs.mkdtempSync(path.join(os.tmpdir(),'gpts-rec-')); fs.mkdirSync(path.join(dir,'learning','log'),{recursive:true}); fs.mkdirSync(path.join(dir,'data'));
  fs.copyFileSync('learning/recommendations.json', path.join(dir,'learning','recommendations.json'));
  fs.writeFileSync(path.join(dir,'learning','log','2026-09-08.json'), JSON.stringify({date:'2026-09-08',patterns:{rows:[]},hypotheses:[]}));
  fs.writeFileSync(path.join(dir,'learning','results.json'), JSON.stringify({schema:1,rules:{}}));
  fs.writeFileSync(path.join(dir,'data','2026-09-08.json'), JSON.stringify({reco:{'R-1':{d:'approved',t:1788500000000,note:null},'R-3':{d:'declined',t:1788500000001,note:'later — after the reads'}}}));
  const u=py('- <<\'EOF\'\nimport sys; sys.path.insert(0, "tools/nightly"); import recommend\nrecommend.update('+JSON.stringify(dir)+')\nEOF');
  const R2=JSON.parse(fs.readFileSync(path.join(dir,'learning','recommendations.json'),'utf8')); const by={}; R2.rows.forEach(r=>{ by[r.id]=r; });
  ok(u.code===0 && by['R-1'].status==='approved' && by['R-1'].decidedOn==='2026-09-08' && by['R-3'].status==='declined' && by['R-3'].note==='later — after the reads' && by['R-2'].status==='proposed' && R2.counts.approved===1 && R2.counts.declined===1 && R2.asOf==='2026-09-08',"1g a ✓ and a ✗ in a day file's `reco` become the rows' status through the nightly — approved (with the day), declined (with his note), the rest untouched",u.err||R2.counts);
}

// ---- 2 · the panel: the module, executed ------------------------------------------------------------------------------------------
{
  const store={};
  const g={ PAL, g3esc:esc, RATE_MIN_N:15, localStorage:{ getItem:k=>(k in store?store[k]:null), setItem:(k,v)=>{ store[k]=v; } }, panOpen:()=>'<div class="g3pan">', panNote:t=>'<div class="note">'+t+'</div>', panFoot:()=>'<div class="foot"></div>', tabEmpty:t=>'<div class="empty">'+t+'</div>',
    secOpen:()=>true, REC_VIEW:true, Date:class extends Date{ static now(){ return 1788500000000; } } };   // every section open in the harness (④ is collapsed by default on the tab)
  const mk=(gg,tail)=>new Function('__g', Object.keys(gg).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('REC_KEY')+exVar('REC_SEED')+exVar('REC_KIND_COL')+'\n'+ex('panSection')+'\n'+['recLoad','recoLoad','recoSave','recDecide','recoExport','recMerged','recKindChip','recRowHtml','recBlock'].map(ex).join('\n')+'\n'+tail);
  const FILE={schema:1,asOf:'2026-09-08',rows:[
    {id:'R-1',kind:'FEATURE',by:'review',asOf:'2026-09-04',text:'Record every READ',changes:'the reads get a chain',evidence:'no read has ever been scored',status:'proposed'},
    {id:'RN-king.floor-held',kind:'RULE',by:'nightly',asOf:'2026-09-08',text:'King as a floor: held 12 of 16 = 75% (Wilson low 51%) against 47%',changes:'the face shows it',evidence:'n=16, low 51% > base 47%',status:'proposed',n:16,rate:75,lo:51,base:47},
    {id:'R-2',kind:'TEST',by:'review',asOf:'2026-09-04',text:'Draft the register entry',changes:'c',evidence:'e',status:'approved',decidedOn:'2026-09-05',decidedAt:1788400000000,version:'15.72'},
    {id:'R-9',kind:'DATA',by:'review',asOf:'2026-09-04',text:'old',changes:'c',evidence:'e',status:'implemented',version:'15.69'},
    {id:'RN-hyp-H3',kind:'RULE',by:'nightly',asOf:'2026-09-06',text:'H3 cleared',changes:'c',evidence:'e',status:'withdrawn',why:'H3 was refused by the register on 2026-09-08'} ]};
  store['gpts_rec_v1']=JSON.stringify(FILE);
  const f=mk(g,'return { block:recBlock, decide:recDecide, exp:recoExport, merged:recMerged, load:recLoad };')(g);
  const h1=f.block();
  ok(/<b>2 awaiting your approval<\/b> · 1 approved · 1 implemented · 1 declined \/ withdrawn · as of 2026-09-08/.test(h1),'2a the header counts the four states from the file',h1.slice(0,400));
  ok(/data-gsec="rc1"[\s\S]*R-1<\/span><span style="[^"]*color:#7cc7ff[^"]*">FEATURE<\/span><span class="qq">Record every READ<\/span><span style="margin-left:auto;white-space:nowrap"><span onclick="window\.__gptsDebug&&window\.__gptsDebug\.recDecide&&window\.__gptsDebug\.recDecide\('R-1','approved'\)"[^>]*>✓ approve<\/span> <span onclick="[^"]*recDecide\('R-1','declined'\)"[^>]*>✗ decline<\/span><\/span><\/div>/.test(h1),'2b a proposed row: id · kind chip · text · ✓ approve / ✗ decline wired to recDecide',h1.match(/R-1<\/span>[\s\S]{0,700}/)[0]);
  ok(/King as a floor: held 12 of 16 = 75% \(Wilson low 51%\) against 47%/.test(h1) && /by the nightly · 2026-09-08/.test(h1) && /RULE<\/span>/.test(h1),'2c a machine row prints its number, its n and its author');
  ok(/data-gsec="rc2"[\s\S]*R-2<\/span>[\s\S]*?APPROVED<\/b> 2026-09-05[\s\S]*?target v15\.72/.test(h1) && !/recDecide\('R-2'/.test(h1),'2d an approved row sits in ② with its decision day and target version — no buttons');
  ok(/data-gsec="rc3"[\s\S]*R-9<\/span>[\s\S]*?IMPLEMENTED v15\.69<\/b>/.test(h1) && /data-gsec="rc4"[\s\S]*RN-hyp-H3[\s\S]*?WITHDRAWN<\/b> — H3 was refused by the register on 2026-09-08/.test(h1),'2e ③ implemented with its version; ④ withdrawn with the reason');
  // his ✓: saved at once, shown at once, rides the export
  const d=f.decide('R-1','approved',null);
  ok(d && d.d==='approved' && d.t===1788500000000 && JSON.parse(store['gpts_reco_v1'])['R-1'].d==='approved','2f recDecide saves the decision to gpts_reco_v1 at once',d);
  const h2=f.block();
  ok(/<b>1 awaiting your approval<\/b> · 2 approved/.test(h2) && /data-gsec="rc2"[\s\S]*R-1<\/span>[\s\S]*?your ✓<\/b> · rides the next 💾/.test(h2) && !/recDecide\('R-1'/.test(h2),'2g …the row moves to ② at once, marked "your ✓ · rides the next 💾", buttons gone',h2.match(/R-1<\/span>[\s\S]{0,500}/)[0]);
  const e=f.exp('2026-09-08');
  ok(JSON.stringify(e)===JSON.stringify({'R-1':{d:'approved',t:1788500000000,note:null}}) && JSON.parse(store['gpts_reco_v1'])['R-1'].exported==='2026-09-08' && /exported 2026-09-08/.test(f.block()),'2h recoExport returns his decisions for the day file and marks them exported; the row then says "exported <day>"',e);
  const d2=f.decide('RN-king.floor-held','declined','not on the face until the register clears it');
  ok(d2.note==='not on the face until the register clears it' && /data-gsec="rc4"[\s\S]*RN-king\.floor-held[\s\S]*?your ✗<\/b>[\s\S]*?not on the face until the register clears it/.test(f.block()),'2i a ✗ with a note: the row moves to ④ with "your ✗" and the note');
  ok(f.decide('R-1','maybe')===null && f.decide('','approved')===null,'2j an unknown decision or an empty id is refused');
  // the file catching up: a decision the file already carries no longer shows as local
  const FILE2=JSON.parse(JSON.stringify(FILE)); FILE2.rows[0].status='approved'; FILE2.rows[0].decidedAt=1788500000001; FILE2.rows[0].decidedOn='2026-09-08'; store['gpts_rec_v1']=JSON.stringify(FILE2);
  const m=f.merged().find(r=>r.id==='R-1');
  ok(m.status==='approved' && m.local===null && /APPROVED<\/b> 2026-09-08/.test(f.block()),'2k once the file carries his decision (the nightly ran), the row shows the file’s status, no longer "local"',m);
  // the seed renders honestly when the file was never fetched
  delete store['gpts_rec_v1'];
  const h3=f.block();
  ok(/file not fetched — rendering the seed/.test(h3) && f.load()===undefined || /rendering the seed/.test(h3),'2l with no fetched file the tab says it is rendering the seed');
}

// ---- 3 · the wiring: the tab, the views, the export, the fetches ---------------------------------------------------------------------
{
  const bar=(src.match(/function analysisTabBar\(\)\{[\s\S]*?\n\}/)||[''])[0];
  ok(/tab\('\\uD83D\\uDCA1 Rec', REC_VIEW, 'window\.__gptsDebug&&window\.__gptsDebug\.showRec&&window\.__gptsDebug\.showRec\(true\)'\)/.test(bar) && /!LEARN_VIEW&&!REC_VIEW\), onDash/.test(bar),'3a the tab bar has 💡 Rec after Learn; Dashboard is "on" only when no view is');
  const shows=src.match(/window\.__gptsDebug\.show\w+=function[^\n]*/g)||[];
  const others=shows.filter(l=>!/showRec=|showDashboard=/.test(l));
  ok(/window\.__gptsDebug\.showRec=function\(b\)\{ REC_VIEW=\(b!==false\); if\(REC_VIEW\)\{ ANALYSIS_VIEW=false; TESTING_VIEW=false; ARCH_VIEW=false; ROADMAP_VIEW=false; ITEMS_VIEW=false; LEARN_VIEW=false; \}/.test(src) && others.length===6 && others.every(l=>/REC_VIEW=false/.test(l)) && /showDashboard=function\(\)\{[^\n]*REC_VIEW=false/.test(src),'3b showRec closes every other view and every other show* closes Rec (no two tabs "on")',others.map(l=>l.slice(0,40)));
  ok(/if\(ARCH_VIEW \|\| ROADMAP_VIEW \|\| ITEMS_VIEW \|\| LEARN_VIEW \|\| REC_VIEW\)\{/.test(src) && /\(REC_VIEW\?recBlock\(\):roadmapBlock\(\)\)/.test(src),'3c render dispatches REC_VIEW to recBlock');
  ok(/reco:\(function\(\)\{ try\{ return recoExport\(dk\); \}catch\(eRc\)\{ return \{\}; \} \}\)\(\),/.test(src),'3d the day export carries `reco`');
  ok(/try\{ recFetch\(\); \}catch\(eRf\)\{\}/.test(src) && /try\{ recFetch\(\); \}catch\(eRc2\)\{\}/.test(ex('pipeCheck')),'3e the file is fetched at boot and on the 10-minute check');
  ok(/\/learning\/recommendations\.json/.test(ex('recFetch')),'3f recFetch reads learning/recommendations.json');
  const seedJs=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); const file=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8'));
  ok(JSON.stringify(seedJs)===JSON.stringify(file),'3g REC_SEED equals learning/recommendations.json (splice-seed)');
  const P=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]);
  const nx=P.roadmap.filter(r=>r.status==='next');
  ok(nx.length===1 && P.roadmap.some(r=>r.v==='15.70' && r.status==='shipped' && /💡 REC — the eighth and last tab/.test(r.title)) && P.roadmap.some(r=>r.v==='15.69' && r.status==='shipped') && P.tabs.some(t=>t.tab==='Rec') && /Rec tab/.test(P.stages[4].what) && /APPROVED on Rec/.test(P.stages[6].what),'3h the plan: v15.69 and v15.70 shipped (v15.71 re-pinned it as shipped); Rec in the tabs; stages ⑤ and ⑦ name it',nx.map(x=>x.v));
}

// ---- 4 · the records --------------------------------------------------------------------------------------------------------------------
{
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8');
  ok(/## v15\.70/.test(cl) && cl.indexOf('## v15.70')<cl.indexOf('## v15.69'),'4a the CHANGELOG has the v15.70 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG');
  ok(/### v15\.70/.test(ls.slice(logAt)),'4b the lesson log carries the v15.70 entry');
  ok(fs.existsSync('mockups/mockup-recommendations.png'),'4c the approved mockup is in mockups/');
}

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
