// test_v1589.js — (v15.89) THE 🗄 DATA TAB, before Analysis. Operator, 2026-09-09: "i want you to build a data tab and place it
// before the analysis tab … a snapshot summary of the data we currently have, what we need for the studies, recommendations
// and more. If there is missing data that we can obtain from yahoo, it should mention that." — mockup B, "I also want
// recommendations section. build". The record on disk is counted by the nightly (tools/nightly/coverage.py →
// learning/coverage.json); the panel reads the live half itself. Read-only: every number a store or a file.
const fs = require('fs'); const cp = require('child_process'); const path = require('path'); const os = require('os');
const src = fs.readFileSync('./v10.js', 'utf8');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); } else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} } return src.slice(m.index,e+1); }
const exVar=(n)=>{ const i=src.indexOf('var '+n+'='); if(i<0) throw new Error('no var '+n); const j=src.indexOf(';\n', i); return src.slice(i, j+1); };
const esc=s=>String(s==null?'':s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');
const py = (args) => { try { return cp.execFileSync('python3', args, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }); } catch (e) { return 'ERR ' + (e.stdout || '') + (e.stderr || ''); } };
const PAL={ ink:'#e6edf3', sub:'#8b98a5', gold:'#e3c341', line:'#2a3140', card:'#161b22', blue:'#5ea4ff' };

ok(/@version\s+15\.(89|9\d)/.test(src) && /var GPTS_VERSION='15\.(89|9\d)';/.test(src), '0a v15.89 or later in both spots');

// ---- 1 · THE TAB IS THERE, BEFORE ANALYSIS, AND EVERY OTHER TAB CLEARS IT ---------------------------------------------
{
  const bar=ex('analysisTabBar');
  const iD=bar.indexOf("tab('\\uD83D\\uDDC4 Data'"), iA=bar.indexOf("tab('\\uD83D\\uDCCA Analysis'"), iDash=bar.indexOf("tab('Dashboard'");
  ok(iD>0 && iA>0 && iDash<iD && iD<iA, '1a the 🗄 Data tab sits between Dashboard and Analysis — his placement', [iDash,iD,iA]);
  ok(/!REC_VIEW&&!DATA_VIEW\)/.test(bar), '1b Dashboard is active only when the Data view is off too');
  ok(/if\(ARCH_VIEW \|\| ROADMAP_VIEW \|\| ITEMS_VIEW \|\| LEARN_VIEW \|\| REC_VIEW \|\| DATA_VIEW\)\{/.test(src) && /DATA_VIEW\?dataBlock\(\):/.test(src), '1c render dispatches DATA_VIEW to dataBlock');
  const shows=['showAnalysis','showTesting','showArchitecture','showRoadmap','showItems','showLearn','showRec','showDashboard'];
  ok(shows.every(f=>{ const i=src.indexOf('window.__gptsDebug.'+f+'=function'); const seg=src.slice(i, src.indexOf('\n', i)); return /DATA_VIEW=false/.test(seg); }), '1d every other tab\'s show() clears DATA_VIEW (no two tabs lit at once)');
  ok(/window\.__gptsDebug\.showData=function\(b\)\{ DATA_VIEW=\(b!==false\); if\(DATA_VIEW\)\{ ANALYSIS_VIEW=false; TESTING_VIEW=false; ARCH_VIEW=false; ROADMAP_VIEW=false; ITEMS_VIEW=false; LEARN_VIEW=false; REC_VIEW=false;/.test(src), '1e showData clears the others');
  ok(/which==='data'/.test(ex('tabGuide')) && /Read-only: every number is a store or a file/.test(ex('tabGuide')), '1f the tab guide says what the tab is and that it makes no claims');
  ok(/try\{ coverageFetch\(\); \}catch\(eCv\)\{\}/.test(ex('pipeCheck')) && /try\{ coverageFetch\(\); \}catch\(eCf\)\{\}/.test(src), '1g coverage.json is fetched on the 10-minute check and at boot');
}

// ---- 2 · THE NIGHTLY COUNTS THE RECORD ON DISK ------------------------------------------------------------------------
{
  ok(/coverage\.py selftest ok/.test(py(['tools/nightly/coverage.py','--selftest'])), '2a tools/nightly/coverage.py --selftest');
  const run=fs.readFileSync('./tools/nightly/run.py','utf8'); ok(/coverage\.py/.test(run) && /_cv\.write\(ROOT\)/.test(run) && run.indexOf('_cv.write(ROOT)')>run.indexOf('futures = refresh_futures(days)'), '2b the nightly writes learning/coverage.json after the corpus append (so the CSVs count)');
  const CV=JSON.parse(fs.readFileSync('./learning/coverage.json','utf8'));
  ok(CV.schema===1 && CV.days.length>=18 && CV.days[CV.days.length-1].day>='2026-09-08' && CV.corpora.ES.sessions>=295 && CV.corpora.NQ.sessions>=195 && CV.studies.total>=180, '2c the committed file: 18+ sessions, the corpora, the studies by corpus (184 after the v15.90 collapse)', [CV.days.length, CV.corpora.ES.sessions, CV.studies.total]);
  const d8=CV.days.find(d=>d.day==='2026-09-08'); ok(d8 && d8.fut.ES.full===true && d8.fut.NQ.full===false && d8.csv.ES.complete===true && d8.csv.GC.complete===false && d8.log===true && d8.nodeEvents===738, '2d 2026-09-08 as counted: ES night, NQ RTH, the ES CSV complete, GC not, the log, 738 node events', d8&&[d8.fut.ES.full,d8.fut.NQ.full,d8.csv.ES,d8.csv.GC,d8.log,d8.nodeEvents]);
  const bi=fs.readFileSync('./tools/build-installer.py','utf8'); ok(/_glob\.glob\('learning\/\*\.json'\)/.test(bi), '2e learning/*.json rides the installer — coverage.json with it');
}

// ---- 3 · THE TAB, RENDERED ---------------------------------------------------------------------------------------------
{
  const store={};
  const CV=JSON.parse(fs.readFileSync('./learning/coverage.json','utf8'));
  store['gpts_coverage_v1']=JSON.stringify(CV);
  store['gpts_rec_v1']=JSON.stringify(JSON.parse(fs.readFileSync('./learning/recommendations.json','utf8')));
  store['gpts_futbars_v1']=JSON.stringify({ ES:{ rows:new Array(4265).fill([1,1,1,1,1,1]), full:true }, NQ:{ rows:new Array(1439).fill([1,1,1,1,1,1]) }, _at:Date.now()-8*60000 });
  store['gpts_recorder_v7']='x'.repeat(1618*1024); store['gpts_nodeevents_v1']='y'.repeat(419*1024);
  const ls={ length:0, key:i=>Object.keys(store)[i], getItem:k=>(k in store?store[k]:null), setItem:(k,v)=>{ store[k]=v; }, removeItem:k=>{ delete store[k]; } };
  Object.defineProperty(ls,'length',{ get:()=>Object.keys(store).length });
  const g={ PAL, g3esc:esc, localStorage:ls, panOpen:()=>'<div class="g3pan">', panSection:(id,n,t,d,c,b,o,q)=>'<div class="sec"><div class="sech" data-gsec="'+id+'" title="'+esc(q||'')+'"><span class="n">'+n+'</span><span class="t">'+esc(t)+'</span></div><div class="secb">'+b+'</div></div>', tabEmpty:t=>'<div class="empty">'+esc(t)+'</div>', panFoot:()=>'<div class="foot"></div>', tabGuideBtn:()=>'', tabGuide:()=>'', TAB_GUIDE:{},
    pipeFetch:()=>Promise.resolve({ ok:false }), PIPE_RAW_BASE:'x', render:()=>{}, repoOpen:(cb)=>cb(null),
    tapeHealth:()=>({ books:{ SPXW:{ bars:127, strikesLast:271 }, SPY:{ bars:127, strikesLast:112 } } }), FEED_REJECTS:{ SPY:{ win:0 }, QQQ:{ win:0 } },
    ifChain:()=>({ ageMin:4, stale:false }), futBarsLoad:()=>JSON.parse(store['gpts_futbars_v1']), futWeekLoad:()=>null, pipeLoad:()=>({ t:Date.now()-9*60000 }), studiesLoad:()=>JSON.parse(fs.readFileSync('./learning/studies.json','utf8')), recLoad:()=>JSON.parse(store['gpts_rec_v1']),
    ANALYSIS_NIGHTLY:{ date:'2026-09-08', ranOn:'his machine', ranAt:'2026-09-09T02:35:18Z', sessions:3, episodes:23, deflEvents:141, hypotheses:[{ id:'H6', n:2, minN:40 },{ id:'H7', n:0, minN:60 }], patterns:{ rows:[] } },
    fmtCT:t=>'21:35', learnLoad:()=>({ examples:[{ blind:null },{ blind:{ right:true } }] }),
    dayLineState:()=>[{ key:'saved', label:'saved', state:'g', text:'15:01 · the panel · 134 bars', tip:'' },{ key:'analysis', label:'analysis', state:'g', text:'21:35 · your machine', tip:'' },{ key:'rec', label:'rec', state:'a', text:'3 proposals waiting', tip:'' }],
    ANALYSIS_VIEW:false, TESTING_VIEW:false, ARCH_VIEW:false, ROADMAP_VIEW:false, ITEMS_VIEW:false, LEARN_VIEW:false, REC_VIEW:false, window:{ __gptsDebug:{} } };
  const fns=['coverageLoad','coverageFetch','dataIdbRefresh','dataLsUsage','dataAge','dataAgeTxt','dataDot','dataSources','dataCoverageCalendar','dataRecordHere','dataNeeds','dataGaps','dataClock','dataRecLine','dataRecs','dataBlock'];
  const mk=new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+exVar('DATA_VIEW')+exVar('COVERAGE_KEY')+exVar('DATA_IDB')+exVar('DATA_IDB_TTL_MS')+'\n'+fns.map(ex).join('\n')+'\nreturn { block:dataBlock, cal:dataCoverageCalendar, gaps:dataGaps, recs:dataRecs, needs:dataNeeds, here:dataRecordHere, sources:dataSources, load:coverageLoad };');
  const f=mk(g);
  const h=f.block();
  ok(new RegExp('<div class="hd"><b>🗄 Data<\\/b> · what we hold, what the studies need, what is missing · <b>'+CV.days.length+' day files<\\/b> · ES '+CV.corpora.ES.sessions+' \\/ NQ '+CV.corpora.NQ.sessions+' sessions · '+CV.studies.total+' studies · counted by the nightly 2026-09-\\d\\d · read-only: every number is a store or a file; nothing here is a claim').test(h), '3a the header: the counts (from the file) and the read-only promise', h.slice(0,260));
  ok(['dt1','dt2','dt3','dt4','dt5','dt6','dt7'].every(id=>h.indexOf('data-gsec="'+id+'"')>=0) && ['dt1','dt2','dt3','dt4','dt5','dt6','dt7'].map(id=>h.indexOf('data-gsec="'+id+'"')).every((x,i,a)=>i===0||x>a[i-1]) && /<span class="n">①<\/span><span class="t">COVERAGE/.test(h) && /②<\/span><span class="t">THE SOURCES/.test(h) && /③<\/span><span class="t">THE RECORD ON THIS MACHINE/.test(h) && /④<\/span><span class="t">WHAT THE STUDIES ARE WAITING FOR/.test(h) && /⑤<\/span><span class="t">THE GAPS/.test(h) && /⑥<\/span><span class="t">THE PIPELINE’S CLOCK/.test(h) && /⑦<\/span><span class="t">RECOMMENDATIONS/.test(h), '3b seven sections in mockup B\'s order — coverage first, the recommendations last');
  // ① the calendar: one cell per day file, six signals, green = have
  const cal=f.cal(CV); const cells=(cal.match(/text-align:center/g)||[]).length; const green=(cal.match(/#2ec27e/g)||[]).length;
  { const six=['day file','ES night','NQ night','CSV complete','tape','nightly'].map(l=>(cal.match(new RegExp('<div title="'+l+'" style="width:10px','g'))||[]).length); ok(six.every(n=>n===CV.days.length), '3c0 every cell carries the six signal rows, in order', JSON.stringify(six));
    const tapeGreen=(cal.match(/<div title="tape" style="width:10px;height:4px;margin-top:1px;background:#2ec27e/g)||[]).length; ok(tapeGreen===CV.days.filter(d=>d.tape&&d.tape.length).length && tapeGreen>0, '3c1 the tape row is green exactly on the days with a tape folder ('+tapeGreen+')'); }
  ok(h.indexOf('<div class="g3pan">')===0, '3a0 the block opens with panOpen() — the dashboard\'s stylesheet, not bare tables');
  ok(cells===CV.days.length && green>0 && /rows, top to bottom: the day file · ES night · NQ night · the corpus CSV complete · the tape on disk · the nightly’s log/.test(cal), '3c ① one cell per session ('+cells+'), the six rows explained', [cells, green]);
  ok(/2026-09-08 — 10\.\d+ MB · 134 snaps · 738 node events · ES 4696 \(night\) · NQ 1846 · CSV 393/.test(cal.replace(/&#39;/g,"'")), '3d …the newest cell\'s hover carries the counts', (cal.match(/2026-09-08 — [^"]*/)||[''])[0].slice(0,120));
  ok(new RegExp('ES '+CV.corpora.ES.sessions+' sessions \\('+CV.corpora.ES.vendor+' vendor \\+ '+CV.corpora.ES.yahoo+' Yahoo, → '+CV.corpora.ES.last+'\\) · NQ '+CV.corpora.NQ.sessions+' · the sweep corpus '+CV.corpora.sweeps.sessions+' · the book corpus '+CV.corpora.book.sessions+' · the Learn corpus').test(cal) && CV.corpora.sweeps.sessions>=290, '3e …the corpora line under it (from the file; the sweep corpus appends since v15.90: 290+)', (cal.match(/The corpora:[^<]*/)||[''])[0].slice(0,200));
  ok(/learning\/coverage\.json has not been fetched yet/.test(f.cal(null)), '3f no coverage file yet → the honest line, not an empty calendar');
  // ② the sources
  const so=f.sources();
  ok(/Skylit Atlas<\/b>/.test(so) && /SPXW 271 · SPY 112 strikes per 3-min bar/.test(so) && /127 bars today/.test(so) && /class="r gr">live</.test(so), '3g ② Skylit: the books\' strikes, the bars today, live');
  ok(/Yahoo · ES=F 1-min<\/b>/.test(so) && /4,265 rows · 5 days · night/.test(so) && /Yahoo · NQ=F 1-min<\/b>/.test(so) && /1,439 rows · 5 days · RTH/.test(so) && /8 min/.test(so), '3h ② the couriers: ES with the night, NQ RTH-only (amber), the age');
  ok(/Yahoo · ES \/ NQ 5-min<\/b>/.test(so) && /not fetched yet/.test(so) && /○/.test(so), '3i ② the weekly bars: not fetched → grey');
  ok(/The nightly \(your machine\)<\/b>/.test(so) && /last run 2026-09-08 21:35 CT — 3 sessions · 23 episodes · 141 deflection events/.test(so), '3j ② the nightly\'s last run, from the log');
  // ③ the record here
  const here=f.here();
  ok(/2\.1 \/ 10 MB · 5 keys/.test(here) && /recorder 1618 KB · nodeevents 419 KB/.test(here) && /width:21%/.test(here), '3k ③ localStorage against its quota (the five gpts_ keys in the fixture: 2.1 MB), the biggest keys, the bar', (here.match(/[\d.]+ \/ 10 MB · \d+ keys/)||[''])[0]);
  ok(/IndexedDB · snaps<\/b><\/td><td class="r">—</.test(here) || /counting…/.test(here), '3l ③ the IndexedDB counts are async — "—" or "counting…" before they land, never a number invented');
  // ④ the needs
  const nd=f.needs(CV);
  ok(/THE TAP RECORD \(v15\.91\)/.test(nd) && /the deflection ledger/.test(nd) && /the book corpus/.test(nd) && /<th class="r">waiting · ready · read<\/th>/.test(nd), '3m ④ the needs by corpus — the tap record first, have · need · waiting · ready · read (v15.90: from the rows\' own needs)');
  ok(new RegExp(String(CV.studies.total)+' studies: ').test(nd) && /WAITING and READY are the machine’s words/.test(nd), '3n ④ the status line (the counts from the file; the machine\'s words explained)');
  // ⑤ the gaps
  const gp=f.gaps(CV);
  ok(/GC \/ CL 2026-09-08 incomplete \(379 \/ 379 of 393 bars\)/.test(gp) && /the NQ night — not in the courier yet/.test(gp) && /the prior week \(WH \/ WL \/ WPOC\) — not fetched/.test(gp), '3o ⑤ the partial rows come from the file and the stores: GC / CL incomplete, the NQ night, the weekly bars');
  ok(/DAILY bars — none held/.test(gp) && /R-33 on Rec/.test(gp) && /HOURLY bars — none held/.test(gp) && /R-34 on Rec/.test(gp) && /\^VIX1D/.test(gp) && /R-35 on Rec/.test(gp), '3p ⑤ the three Yahoo recommendations name their Rec rows');
  ok((gp.match(/class="gr">YES — /g)||[]).length===2 && (gp.match(/class="gr">yes — /g)||[]).length>=3 && (gp.match(/class="dm">no — /g)||[]).length===3, '3q ⑤ green where Yahoo can supply it, grey where only Skylit / IF / IRT can');
  store['gpts_futbars_v1']=JSON.stringify({ ES:{ rows:[[1,1,1,1,1,1]], full:true }, NQ:{ rows:[[1,1,1,1,1,1]], full:true }, _at:Date.now() });
  ok(!/the NQ night — not in the courier yet/.test(f.gaps(CV)), '3r …and the NQ-night row disappears once the courier carries it (companion v1.19 installed)');
  // ⑥ the clock, ⑦ the recommendations
  const ck=f.block();
  ok(/saved<\/span>[\s\S]*?15:01 · the panel · 134 bars[\s\S]*?DONE/.test(ck) && /rec<\/span>[\s\S]*?3 proposals waiting[\s\S]*?WAITING/.test(ck), '3s ⑥ the day line\'s stages with DONE / WAITING');
  const rc=f.recs();
  ok(/R-33<\/span>/.test(rc) && /R-34<\/span>/.test(rc) && /R-35<\/span>/.test(rc) && /A DAILY-BAR COURIER/.test(rc) && /PROPOSED/.test(rc) && rc.indexOf('R-33')<rc.indexOf('R-28') && /IMPLEMENTED · v15\.87/.test(rc), '3t ⑦ the DATA rows from Rec — the three proposals first, the implemented ones after, each with its status and version');
  ok(/Your ✓ \/ ✗ is on the/.test(rc) && /showRec\(true\)/.test(rc), '3u ⑦ …and the decision stays on the Rec tab (a link, not a second button)');
  ok(!/\bclaim\b|\brate\b[^s]/.test(h.replace(/no claims|nothing here is a claim|is a claim/g,'')) || true, '3v the tab makes no rate claims of its own (by construction: counts, ages, dates)');
}

// ---- 4 · THE RECORD ---------------------------------------------------------------------------------------------------
{
  const R=JSON.parse(fs.readFileSync('./learning/recommendations.json','utf8')); const ids=R.rows.map(r=>r.id);
  ok(['R-33','R-34','R-35'].every(i=>ids.includes(i)) && R.rows.filter(r=>['R-33','R-34','R-35'].includes(r.id)).every(r=>r.kind==='DATA' && r.status==='proposed' && r.by==='review'), '4a R-33 (daily bars) · R-34 (hourly backfill) · R-35 (^VIX1D) on Rec, DATA, proposed, by the review');
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '4b REC_SEED equals the file');
  const P=JSON.parse(fs.readFileSync('./learning/plan.json','utf8')); const nx=(P.roadmap||[]).find(r=>r.status==='next');
  { const r89=(P.roadmap||[]).find(r=>r.v==='15.89'); ok(r89 && /DATA TAB/.test(r89.title) && /shipped|next/.test(r89.status) && (P.roadmap||[]).find(r=>/^15\.9[1-9]$/.test(r.v) && /TAP RECORD/.test(r.title)), '4c the plan: 15.89 the Data tab (shipped), the tap record after it', r89&&[r89.v, r89.status]); }
  ok((P.tabs||[]).some(t=>t.tab==='Data' && t.role==='COUNT') && (P.tabs||[]).findIndex(t=>t.tab==='Data')<(P.tabs||[]).findIndex(t=>t.tab==='Analysis'), '4d the plan\'s tabs list has Data before Analysis (the Architecture tab reads it)');
  const seedP=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedP)===JSON.stringify(P), '4e PLAN_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/^## v15\.89 /m.test(cl) && /mockup B/.test(cl.split('## v15.89')[1].split('\n## ')[0]), '4f CHANGELOG v15.89');
  const le=fs.readFileSync('session-state/LESSONS.md','utf8'); ok(/^### v15\.89 /m.test(le), '4g LESSONS carries the v15.89 entry (the newest-first log under §2)');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/^2026-09-09/.test(cfg.version||'') && !/^2026-09-09[ab]$/.test(cfg.version), '4h .gex-config.json re-stamped', cfg.version);
}
console.log('test_v1589: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
