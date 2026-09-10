// test_v1593.js — (v15.93) THE WICK OPEN IS THE RTH OPEN (the 08:30:00 print).
//
// Operator, 2026-09-09, comparing his FuturesPulse tool (wick% 9%) with the panel (15%): "check it.. it is being
// displayed" → "the tool gets data from yahoo finance" → "RTH". Both tools pull the SAME Yahoo ES=F 1-minute feed; the
// only field that ever disagreed was wick%, and it is entirely the session OPEN. His tool opened the wick on the 08:28
// print (7661.75 → 9%), the panel on the 08:27 print (7659.50 → 15%, the v15.87 "tools" mirror); both are PRE-open and
// his tool's opening minute was not even stable day to day (08:27 on 09-08). He chose the true RTH open — Yahoo's
// 08:30:00 print (7660.75 → 12% on 09-09). So HL_TOOL_A / LOAD_A move 08:27 → 08:30; the first RTH bar is 08:30–08:32,
// its open IS the session open, pre-open minutes leave the extremes, every clock is unchanged. The corpus is re-folded
// (study-hodlod), the boot literal re-baked (bake-hodlod), the E row and A row stay one definition.
const fs = require('fs'); const cp = require('child_process');
const src = fs.readFileSync('./v10.js', 'utf8');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); } else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} } return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }
const py = (args) => { try { return cp.execFileSync('python3', args, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }); } catch (e) { return 'ERR ' + (e.stdout || '') + (e.stderr || ''); } };

ok(/@version\s+15\.93/.test(src) && /var GPTS_VERSION='15\.93';/.test(src), '0a v15.93 in both spots');

// ---- 1 · THE GRID ANCHOR IS 08:30 ----------------------------------------------------------------------------------
global.mul=(a,b)=>a/(1/b);
eval((src.match(/var HL_TOOL_A = [^\n]*;\n/)||[''])[0]); eval(ex('hlToolBars'));
{
  ok(HL_TOOL_A===8*3600+30*60, '1a HL_TOOL_A is 08:30 — the RTH open (was 08:27, the pre-open)', HL_TOOL_A/3600);
  const S=(h,m)=>h*3600+m*60;
  // a pre-open minute (08:29) folds into no RTH bar; the 08:30 minute opens the first bar
  const B=hlToolBars([{so:S(8,29),o:99,h:99,l:99,c:99},{so:S(8,30),o:7660.75,h:7661,l:7659,c:7660},{so:S(8,31),o:7660,h:7662,l:7658,c:7661},{so:S(8,32),o:7661,h:7663,l:7660,c:7662}]);
  ok(B.length===1 && B[0].so===S(8,33) && B[0].o===7660.75, '1b 08:29 is dropped; the first bar is 08:30–08:32 ending 08:33, opening on the 08:30 print', B[0]);
}

// ---- 2 · HIS 09-09 A ROW: OPEN 7660.75, WICK% 12, HOD 10:03, LOD 10:27 --------------------------------------------
global.ES_USD_PER_PT=50; let NOWSEC=15*3600; global.ctNowSecOfDay=()=>NOWSEC; global.inReplay=()=>false; global.showingStaleBook=()=>false;
global.dispIsFut=()=>false; global.dispR=()=>1; global.rmemo=(k,f)=>f(); global.localStorage={ getItem:()=>null, setItem:()=>{}, removeItem:()=>{} };
global.HLBASE_KEY='x'; global.HLBASE_MIN_SESSIONS=120; global.HLBASE_MIN_BUCKET=50; global.HODLOD_BASE=val('HODLOD_BASE');
global.futBarsLoad=()=>null; let CANDLES=[]; global.closedCandles=()=>CANDLES;
eval(ex('hlBaseNormalise')); eval(ex('hodlodBase')); eval(ex('measureBars')); eval(ex('measureBarsRaw')); eval(ex('hlClock')); eval(ex('hlDur')); eval(ex('hlTier')); eval(ex('hodLod'));
{
  const S=(h,m)=>h*3600+m*60;
  const mins=[];
  // pre-open, with a spike that must NOT become the day's HOD (proves pre-open is excluded)
  mins.push({so:S(8,27),o:7659.5,h:7662,l:7659.5,c:7661.75,t:1});
  mins.push({so:S(8,28),o:7661.75,h:7699,l:7661,c:7662,t:2});   // a 7699 pre-open spike
  mins.push({so:S(8,29),o:7662,h:7662,l:7659,c:7660.5,t:3});
  // RTH: open 7660.75 at 08:30; fill benign; HOD 7665 at 10:00; LOD 7628.75 at 10:25
  for(let s=S(8,30); s<=S(14,59); s+=60){
    let o=7650,h=7651,l=7649,c=7650;
    if(s===S(8,30)){ o=7660.75; h=7661; l=7659; c=7660; }
    if(s===S(10,0)){ o=7658; h=7665; l=7657; c=7661; }      // the HOD
    if(s===S(10,25)){ o=7632; h=7633; l=7628.75; c=7631; }  // the LOD
    mins.push({so:s,o:o,h:h,l:l,c:c,t:s});
  }
  CANDLES=mins;
  const D=hodLod('SPY');
  ok(D.ok && D.grid==='tool' && D.open===7660.75, '2a the session open is the RTH open — the 08:30 print 7660.75, NOT the 08:27 (7659.50) or the 08:28 (7661.75)', [D.grid, D.open]);
  ok(D.first==='HOD' && D.hod===7665 && D.hodT===S(10,3), '2b the HOD 7665 is at 10:00 → the bar ending 10:03; the 7699 pre-open spike is not the HOD (pre-open is out)', [D.first, D.hod, D.hodT/3600]);
  ok(D.lod===7628.75 && D.lodT===S(10,27), '2c the LOD 7628.75 → the bar ending 10:27', [D.lod, D.lodT/3600]);
  ok(Math.abs(D.rngPts-36.25)<1e-9, '2d the range is 36.25 pts (7665 − 7628.75)', D.rngPts);
  ok(D.wickPct===12, '2e Wick% 12 — |7660.75 − 7665| / 36.25 (the RTH open; 08:27 read 15, his tool’s 08:28 read 9)', D.wickPct);
  // the hover states the RTH open
  global.g3esc=s=>String(s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;'); global.g3tip=t=>t?(' title="'+g3esc(t)+'"'):'';
  global.gdActual=()=>({ green:false, pts:-1, open:7660.75, now:7649 }); global.replayOn=()=>false; global.hlDayShown=()=>'2026-09-09'; global.swallow=()=>{};
  global.GD_META={ n:282, fires:225, acc:76, base:51, ciLo:71, ciHi:82 }; global.gdRead=()=>({ ok:false, why:'' });
  eval(['hlClock12','hlARowHtml','hlMudLabel'].map(ex).join('\n'));
  const aH=hlARowHtml('SPY', D, false).replace(/&#39;|&quot;/g,"'");
  ok(/the OPEN is the RTH open \(the 08:30:00 print, the bar 08:30–08:32\)/.test(aH), '2f the A row’s hover states the RTH open (rendered)', aH.slice(0,240));
}

// ---- 3 · THE STUDY AND THE BOOT LITERAL FOLLOW ---------------------------------------------------------------------
{
  const sh=fs.readFileSync('./tools/study-hodlod.py','utf8');
  ok(/LOAD_A, LOAD_B = 8\*3600\+30\*60, 15\*3600/.test(sh), '3a study-hodlod LOAD_A is 08:30 — the RTH open, one definition end to end with the panel');
  ok(/^MIN_BARS = 383/m.test(sh), '3b MIN_BARS 383 (386 − the 3 pre-open minutes) so the same 295 sessions stay complete', /^MIN_BARS = (\d+)/m.exec(sh)[1]);
  ok(/RTH open/.test(sh) && /the 08:30:00 print/.test(sh), '3c the corpus definition names the RTH open');
  const BR=JSON.parse(fs.readFileSync('./data/es-1min/BASERATES.json','utf8'));
  ok(BR.corpus.sessions===295 && BR.corpus.min_bars===383 && /RTH open/.test(BR.corpus.definition), '3d BASERATES: 295 sessions, min_bars 383, the definition says RTH open', [BR.corpus.sessions, BR.corpus.min_bars]);
  const chk=py(['tools/bake-hodlod.py','--check']);
  ok(!/ERR/.test(chk) && !/disagree|differ/i.test(chk), '3e the panel’s HODLOD_BASE literal equals the re-baked file (bake-hodlod --check)', chk.trim().slice(0,120));
}

console.log('\n' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
