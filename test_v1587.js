// test_v1587.js — (v15.87) THE TOOL GRID · THE CORPUS APPENDS ITSELF
//
// Two of his decisions, 2026-09-09. (1) "tools": the ⓪a A row reads the session the way his tool does — 3-minute bars
// stamped by END, the session = the bars ending 08:30…15:00, the open = the bar ending 08:30 (the 08:27 minute's open),
// the extremes' clocks are bar ends, W.End is the first bar AFTER the extreme's to close through the open. His row for
// 2026-09-08 (HOD 8:33 · Took 3m · BOP 3m · Wick 6m · W.End 8:36 · Wick% 6 · LOD 3:00pm · MD $2,138) is the fixture.
// (2) "yes": the couriered Yahoo minute bars append to the per-market corpus every night (append-futures → study-hodlod
// → BASERATES, ES and NQ), the panel's boot literal is re-baked from the file at build time (bake-hodlod), and the
// nightly re-runs itself when an installer has pasted older outputs over its log (tick.py, by mtime and by content).
const fs = require('fs'); const cp = require('child_process');
const src = fs.readFileSync('./v10.js', 'utf8');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); } else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} } return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }
const py = (args) => { try { return cp.execFileSync('python3', args, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }); } catch (e) { return 'ERR ' + (e.stdout || '') + (e.stderr || ''); } };

ok(/@version\s+15\.(8[7-9]|9\d)/.test(src) && /var GPTS_VERSION='15\.(8[7-9]|9\d)';/.test(src), '0a v15.87 or later in both spots');

// ---- 1 · THE FOLDER: minutes → 3-minute bars stamped by END --------------------------------------------------------
global.mul=(a,b)=>a/(1/b); global.two=x=>{x=''+x;return x.length<2?'0'+x:x;};
eval((src.match(/var HL_TOOL_A = [^\n]*;\n/)||[''])[0]);   // the three constants, one line in the panel
eval(ex('hlToolBars'));
{
  ok(HL_TOOL_A===8*3600+27*60 && HL_TOOL_B===15*3600 && HL_TOOL_BAR===180, '1a the grid: from 08:27, before 15:00, 180 s bars', [HL_TOOL_A,HL_TOOL_B,HL_TOOL_BAR]);
  const S=(h,m)=>h*3600+m*60;
  const mins=[]; for(let i=0;i<9;i++) mins.push({ so:S(8,27)+i*60, o:100+i, h:110+i, l:90-i, c:101+i, t:1000+i });
  const B=hlToolBars(mins);
  ok(B.length===3 && B[0].so===S(8,30) && B[1].so===S(8,33) && B[2].so===S(8,36), '1b nine minutes from 08:27 fold into three bars ending 08:30 · 08:33 · 08:36', B.map(b=>b.so));
  ok(B[0].o===100 && B[0].h===112 && B[0].l===88 && B[0].c===103 && B[0].n===3 && B[0].start===S(8,27) && B[0].t===1002,
     '1c a bar opens on its first minute, takes the highest high and lowest low, closes on its last minute (t = the last minute\'s)', B[0]);
  ok(hlToolBars([{so:S(8,26),o:1,h:1,l:1,c:1},{so:S(15,0),o:1,h:1,l:1,c:1},{so:S(15,1),o:1,h:1,l:1,c:1}]).length===0, '1d a minute before 08:27 or at/after 15:00 belongs to no session bar');
  ok(hlToolBars([]).length===0 && hlToolBars(null).length===0 && hlToolBars([{o:1}]).length===0, '1e no bars, null, a bar without a clock → an empty grid, no throw');
  const one=hlToolBars([{so:S(14,59),o:5,h:6,l:4,c:5}]); ok(one.length===1 && one[0].so===S(15,0), '1f the 14:59 minute is the bar ending 15:00 — the last bar of the session', one);
  ok(/typeof hlToolBars==='function'/.test(ex('hodLod')) && /typeof hlToolBars==='function'/.test(ex('gdActual')), '1g hodLod and gdActual fold through the grid behind a typeof guard (DEGRADE, DO NOT DEPEND)');
}

// ---- 2 · HIS ROW FOR 2026-09-08, FROM THE COURIERED BARS ----------------------------------------------------------
global.ES_USD_PER_PT=50; let NOWSEC=15*3600; global.ctNowSecOfDay=()=>NOWSEC; global.inReplay=()=>false; global.showingStaleBook=()=>false;
global.dispIsFut=()=>false; global.dispR=()=>1; global.rmemo=(k,f)=>f(); global.rmemoNext=()=>{};
global.localStorage={ getItem:()=>null, setItem:()=>{}, removeItem:()=>{} };
global.HLBASE_KEY='gpts_hodlod_base_v1'; global.HLBASE_MIN_SESSIONS=120; global.HLBASE_MIN_BUCKET=50; global.HODLOD_BASE=val('HODLOD_BASE');
global.futBarsLoad=()=>null; let CANDLES=[]; global.closedCandles=()=>CANDLES;
eval(ex('hlBaseNormalise')); eval(ex('hodlodBase')); eval(ex('measureBars')); eval(ex('measureBarsRaw')); eval(ex('hlClock')); eval(ex('hlDur')); eval(ex('hlTier')); eval(ex('hodLod'));
{
  const rows=fs.readFileSync('./data/futures/ES/2026-09-08.csv','utf8').trim().split('\n').slice(1).map(l=>l.split(','));
  CANDLES=rows.map(r=>{ const hm=r[1].slice(11,16).split(':'); return { so:(+hm[0])*3600+(+hm[1])*60, o:+r[3], h:+r[4], l:+r[5], c:+r[6], t:Date.parse(r[1].replace(' ','T')+'-05:00') }; });
  ok(CANDLES.length>=379 && CANDLES[0].so===8*3600+27*60, '2a the day\'s corpus file starts at 08:27 (the open of the bar his tool labels 8:30) — '+CANDLES.length+' minutes', CANDLES[0]);
  const D=hodLod('SPY');
  ok(D.ok && D.grid==='tool' && D.open===7715, '2b the session open is the bar ending 08:30\'s open — 7715.00, his tool\'s (the 08:30 minute opened 7711.50)', [D.ok, D.grid, D.open]);
  ok(D.first==='HOD' && D.hod===7717.75 && D.hodT===8*3600+33*60 && Math.round(D.took)===3, '2c HOD 7717.75 in the bar ending 8:33 — Took 3m', [D.first, D.hod, D.hodT, D.took]);
  ok(D.wend===8*3600+36*60 && Math.round(D.bop)===3 && Math.round(D.wick)===6, '2d W.End 8:36 — the first bar AFTER the extreme\'s to close through the open — BOP 3m · Wick 6m', [D.wend, D.bop, D.wick]);
  ok(D.wickPct===6, '2e Wick% 6 — |7715 − 7717.75| / the range 45.5', D.wickPct);
  ok(D.lod===7672.25 && D.lodT===15*3600 && D.second==='LOD', '2f LOD 7672.25 in the bar ending 3:00pm', [D.lod, D.lodT]);
  ok(Math.abs(D.rngPts-45.5)<1e-9 && Math.round(D.mud)===384, '2g HL Rng 45.5 pts · MUD 6h24m (W.End 8:36 → 3:00pm)', [D.rngPts, D.mud]);
  ok(Math.abs((D.open-D.lod)*50-2137.5)<1e-9, '2h MD = |LOD − open| × $50 = $2,137.50 — his "$2,138"', (D.open-D.lod)*50);
  // the grid is STATED on the face — the A row's hover, rendered and read, not grepped
  global.g3esc=s=>String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;'); global.g3tip=t=>t?(' title="'+g3esc(t)+'"'):'';
  global.GD_META={ n:282, fires:225, acc:76, base:51, ciLo:71, ciHi:82 }; global.gdRead=()=>({ ok:false, why:'as it closed' }); global.gdActual=()=>({ green:false, pts:-36.75, open:7715, now:7678.25 });
  global.replayOn=()=>false; global.hlDayShown=()=>'2026-09-08'; global.swallow=()=>{};
  eval(['hlClock12','hlARowHtml','hlMudLabel'].map(ex).join('\n'));
  const aH=hlARowHtml('SPY', D, false);
  ok(/Grid: tool — 3-minute bars stamped by their END \(the bar labelled 8:30 is 08:27–08:30\); the OPEN is that first bar’s open, every clock is a bar end, W\.End is the first bar after the extreme’s to close through the open\./.test(aH.replace(/&#39;|&quot;/g,"'")),
     '2k the A row\'s hover states the grid and the open\'s definition (rendered)', aH.slice(0,260));
  ok(!/Grid: tool/.test(hlARowHtml('SPY', Object.assign({}, D, { grid:undefined }), false)), '2l …and says nothing about a grid when hodLod did not fold (an older reader)');
  // the empty-grid messages
  CANDLES=[{so:8*3600+20*60,o:1,h:1,l:1,c:1}]; const E1=hodLod('SPY'); ok(!E1.ok && E1.why==='no RTH bars yet', '2i bars before 08:27 only → "no RTH bars yet"', E1.why);
  CANDLES=[]; const E2=hodLod('SPY'); ok(!E2.ok && E2.why==='no candles', '2j no bars at all → "no candles"', E2.why);
}

// ---- 3 · THE CORPUS APPENDS ITSELF ---------------------------------------------------------------------------------
{
  const BR=JSON.parse(fs.readFileSync('./data/es-1min/BASERATES.json','utf8'));
  ok(BR.corpus.sessions>=294 && BR.corpus.last>='2026-09-04' && /tool grid/.test(BR.corpus.definition||''), '3a ES BASERATES: ≥ 294 sessions through ≥ 2026-09-04, the definition names the tool grid', [BR.corpus.sessions, BR.corpus.last]);
  const srcs=BR.corpus.sources||{}; const vendor=Object.entries(srcs).filter(([k])=>!/^\d{4}/.test(k)).reduce((a,[,v])=>a+v,0); const yahoo=Object.entries(srcs).filter(([k])=>/^\d{4}/.test(k)).reduce((a,[,v])=>a+v,0);
  ok(vendor===284 && yahoo>=10 && vendor+yahoo===BR.corpus.sessions, '3b provenance per session: 284 from the vendor file, the rest from the Yahoo dailies, summing to the corpus', [vendor, yahoo]);
  ok(fs.existsSync('./data/futures/NQ/BASERATES.json') && JSON.parse(fs.readFileSync('./data/futures/NQ/BASERATES.json','utf8')).corpus.sessions>=10, '3c the NQ corpus has its own BASERATES (Yahoo only, ≥ 10 sessions)');
  ok(fs.existsSync('./data/futures/ES/2026-09-04.csv') && fs.existsSync('./data/futures/NQ/2026-09-04.csv'), '3d the per-day corpus files exist for ES and NQ');
  const ap=fs.readFileSync('./tools/append-futures.py','utf8'); ok(/RTH_A, RTH_B = 8\*3600\+27\*60, 15\*3600/.test(ap), '3e append-futures harvests from 08:27 (the tool grid\'s first minute)');
  const sh=fs.readFileSync('./tools/study-hodlod.py','utf8'); ok(/LOAD_A, LOAD_B = 8\*3600\+27\*60, 15\*3600/.test(sh) && /^BAR = 180/m.test(sh) && /def tool_bars\(/.test(sh) && /market='ES'/.test(sh), '3f study-hodlod folds on the same grid, per market');
  const run=fs.readFileSync('./tools/nightly/run.py','utf8'); ok(/def refresh_futures\(days, keep_last=(4|None)\)/.test(run) && /futures = refresh_futures\(days\)/.test(run) && /futures=futures/.test(run), '3g the nightly appends the day files (the last four at v15.87; every one from v15.88) and rebuilds ES + NQ BASERATES, and the log carries it');
  const og=fs.readFileSync('./tools/origin-guard.py','utf8'); ok(/'data\/es-1min\/BASERATES\.json'/.test(og) && /'data\/futures\/'/.test(og), '3h the origin guard treats BASERATES and data/futures/ as the nightly\'s writes');
  ok(/bake-hodlod: the literal EQUALS the file/.test(py(['tools/bake-hodlod.py','--check'])), '3i tools/bake-hodlod.py --check: the panel\'s HODLOD_BASE literal equals the file');
  ok(/RE-BAKED BY tools\/bake-hodlod\.py/.test(src) && val('HODLOD_BASE').n===BR.corpus.sessions, '3j the literal says it is baked, and its n is the file\'s');
  const csvRows=fs.readdirSync('./data/futures/ES').filter(f=>/^\d{4}-\d\d-\d\d\.csv$/.test(f)).flatMap(f=>fs.readFileSync('./data/futures/ES/'+f,'utf8').trim().split('\n').slice(1));
  ok(csvRows.length>=11*390 && csvRows.every(l=>/ \d\d:\d\d:00,/.test(l)), '3k every corpus row is on the minute — Yahoo\'s live-quote row (14:49:41, volume 0) is dropped by the harvest', csvRows.filter(l=>!/ \d\d:\d\d:00,/.test(l)).slice(0,2));
}

// ---- 4 · THE NIGHTLY RE-RUNS ITSELF AFTER AN INSTALL ---------------------------------------------------------------
{
  const tk=fs.readFileSync('./tools/nightly/tick.py','utf8');
  ok(/OUTPUTS = \('learning\/results\.json', 'learning\/studies\.json', 'learning\/recommendations\.json', 'learning\/deflections\/examples\.json',\s*'data\/es-1min\/BASERATES\.json'\)/.test(tk), '4a tick.py names the five outputs');
  ok(/an installer wrote over the nightly\\'s outputs/.test(tk) && /_as_of\(/.test(tk) && /aso < day/.test(tk), '4b …and re-runs when one is older than the log (mtime) or results.json is dated before the log\'s day (content)');
  ok(/tick\.py selftest ok/.test(py(['tools/nightly/tick.py','--selftest'])), '4c tick.py --selftest (both clobber cases)');
  ok(/origin-guard selftest ok/.test(py(['tools/origin-guard.py','--selftest'])), '4d origin-guard --selftest');
}

// ---- 5 · THE RECORDS ----------------------------------------------------------------------------------------------
{
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/^## v15\.87 /m.test(cl) && /tools/.test(cl.split('## v15.87')[1].split('\n## ')[0]) && /15:21/.test(cl.split('## v15.87')[1].split('\n## ')[0]), '5a CHANGELOG v15.87 — his "tools", and the clobber told');
  const de=fs.readFileSync('session-state/DECISIONS.md','utf8'); ok(/"tools"/.test(de) && /bar ending 08:30/i.test(de), '5b DECISIONS carries his word and the open\'s definition');
  const le=fs.readFileSync('session-state/LESSONS.md','utf8'); ok(/^### v15\.87 /m.test(le), '5c LESSONS carries the v15.87 entry (the newest-first log under §2)');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/^2026-09-09/.test(cfg.version||''), '5d .gex-config.json stamped 2026-09-09', cfg.version);
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=(P.roadmap||[]).find(r=>r.status==='next'); const r87=(P.roadmap||[]).find(r=>r.v==='15.87'); ok(r87 && /tool grid/i.test(r87.title||'') && /shipped|next/.test(r87.status), '5e the plan carries 15.87, the tool grid (next at its build, shipped after)', r87&&[r87.v,r87.status]);
}
console.log('test_v1587: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
