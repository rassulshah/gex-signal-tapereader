#!/usr/bin/env node
// test_v1579.js — (v15.79) THE E ROW BELOW THE LADDER, BIGGER; THE CANDLE'S SWEEPS ARE THE SHOWN DAY'S (R-16). Operator,
// 2026-09-08: "move the hod expected stats above the replay below the node ladder and the daily candle and make the
// font slightly bigger because it is very small and cant read it" → "make the font bigger and add some spacing so it
// takes up the row, show mockup" → "build". Mid-build: "on the daily candle you should indicate the levels that the
// hod and lod swept" — they were the SWEPT line's events, which read TODAY's bars while the closed state's candle
// stands on Friday's; now both read the SHOWN day's. Found on the way: futSessionBars sorted its day keys as strings.
// (sloppy mode on purpose: a direct eval must declare the panel's functions into this scope)
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } } return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }

// ---------- 0. the version ----------
ok(/@version\s+15\.(79|[89]\d)/.test(src) && /var GPTS_VERSION='15\.(79|[89]\d)';/.test(src), '0a v15.79 or later in both spots');

// ---------- 1. the E row: built in secDay, emitted by panelV3 below the ladder block, above the strip ----------
{
  const sd=ex('secDay'), pv=ex('panelV3');
  ok(/SECDAY_EROW=''; try\{ SECDAY_EROW=hlERowHtml\(sym, D, base, NOREAD\); \}catch\(eER\)\{ swallow\('hlERowHtml', eER\); \}/.test(sd), '1a secDay BUILDS the row into SECDAY_EROW (its base, its D) and no longer emits it');
  ok(!/h\+=hlERowHtml\(/.test(sd), '1b …nothing in secDay appends it to the top');
  ok(/var SECDAY_EROW='';/.test(src), '1c SECDAY_EROW is declared bare (exVar-readable)');
  const sl=ex('secLoc'), sf=ex('secFrame'); const iEmit=sl.indexOf("try{ if(typeof SECDAY_EROW==='string' && SECDAY_EROW) h+=SECDAY_EROW; }catch(eER2){}"), iSet=sl.indexOf("<em>SET</em>");
  ok(/var h=secFrame\(sym\)\+/.test(sl) && sf.indexOf("h+='<div class=\"g3ladcdl\" data-sym=")>0 && iEmit>0 && iSet>iEmit,
     '1d secLoc opens with secFrame (the King cards, the ladder, the candle slot) and emits the row BEFORE its SET line — the replay strip follows in render()');
  ok(!/SECDAY_EROW/.test(pv), '1d2 …and panelV3 does not emit it a second time');
  const iRp=src.indexOf("try{ html+=replayBarHtml('strip'); }catch(eRPS){"), iPv=src.indexOf("try{ html+=panelV3(__asym); }");
  ok(iPv>0 && iRp>iPv, '1e …and render() puts the strip after panelV3, so the row sits above it');
  // the sizes and the spread — the mockup he chose (mockups/mockup-e-row-big.png)
  ok(/#gpts-body \.g3erow\{display:flex;flex-wrap:wrap;align-items:center;justify-content:space-between;gap:4px 0;margin:4px 0 6px;padding:5px 10px;[^}]*font-size:10\.5px/.test(src), '1f the row: 10.5 px, spread across the row (space-between), roomier padding');
  ok(/#gpts-body \.g3erow \.et\{font-size:9px;/.test(src) && /#gpts-body \.g3erow \.ec i\{font-style:normal;font-size:8\.6px;/.test(src) && /#gpts-body \.g3erow \.chip\{display:inline-block;font-size:9px;[^}]*line-height:17px/.test(src),
     '1g the E tag 9 px, the labels 8.6 px, the chips 9 px on a 17 px line');
  ok(!/\.g3erow \.ec\{[^}]*margin-right:6px/.test(src) && /\.g3erow \.ec\{display:inline-flex;align-items:baseline;white-space:nowrap;margin-right:0\}/.test(src), '1h the cells carry no fixed gap — the spread is the row\'s');
  ok(fs.existsSync('mockups/mockup-e-row-big.png') && fs.existsSync('mockups/mockup-e-row-moved.png') && fs.existsSync('mockups/mockup-e-row-moved.js'), '1i the two mockups he chose from are in the repo');
}

// ---------- 2. the courier's day keys: numeric, and anchored on a shown day ----------
global.futBarsLoad=()=>null;
eval(['futDayKeyOf','futDayKeyNum','futSessionBars'].map(ex).join('\n'));
{
  ok(futDayKeyOf('2026-09-04')==='2026-9-4' && futDayKeyOf('2026-09-10')==='2026-9-10' && futDayKeyOf('garbage')===null, '2a the courier key of a day is the unpadded form futSessionBars always used');
  ok(futDayKeyNum('2026-9-10')>futDayKeyNum('2026-9-8') && futDayKeyNum('2026-9-8')>futDayKeyNum('2026-8-31'), '2b …and the keys compare as numbers');
  // a courier window straddling the 8th → the 10th: string order puts the 10th before the 8th
  const CT=5*3600; const epochOf=(y,m,d,hh,mm)=>Date.UTC(y,m-1,d,hh,mm)/1000+CT;   // CT wall clock → epoch
  const rows=[];
  [[2026,9,4],[2026,9,8],[2026,9,9],[2026,9,10]].forEach(([y,m,d],di)=>{
    for(let k=0;k<5;k++) rows.push([epochOf(y,m,d,3,k), 7700+di, 7710+di, 7690+di, 7700+di, 100]);        // overnight (03:0k)
    for(let k=0;k<5;k++) rows.push([epochOf(y,m,d,9,k), 7700+di, 7720+di, 7680+di, 7705+di, 100]);        // RTH (09:0k)
  });
  global.futBarsLoad=()=>({ ES:{ rows } });
  const T0=futSessionBars(0), T1=futSessionBars(1);
  ok(T0 && T0.rth.length===5 && T0.rth[0][1]===7703, '2c on the 10th, "today" is the 10th\'s bars (a string sort would have said the 9th)', T0&&T0.rth[0]);
  ok(T1 && T1.rth[0][1]===7702, '2d …and the prior session is the 9th');
  const F=futSessionBars(0,'2026-09-04'), F1=futSessionBars(1,'2026-09-04');
  ok(F && F.rth[0][1]===7700 && F.on.length===5, '2e anchored on Friday the 4th: Friday\'s RTH and Friday\'s overnight');
  ok(F1===null, '2f …and Friday\'s prior session is outside the window → null, never a wrong day');
  ok(futSessionBars(0,'2026-09-07')===null, '2g a day the courier does not hold (Labor Day) → null');
  ok(futSessionBars(0,'2026-09-09') && futSessionBars(0,'2026-09-09').rth[0][1]===7702 && futSessionBars(1,'2026-09-09').rth[0][1]===7701, '2h anchored on the 9th, its prior session is the 8th');
}

// ---------- 3. the shown day's sweeps ----------
{
  let CALLS=[]; global.sweepEventsToday=(sym,day)=>{ CALLS.push([sym,day]); return [{level:'POC',side:'LOD',day:day||'today'}]; };
  global.ctTodayStr=()=>'2026-09-08'; let REPLAY_ON=false, SHOWN='2026-09-08'; global.replayOn=()=>REPLAY_ON; global.hlDayShown=()=>SHOWN;
  eval(ex('sweepEventsShown'));
  ok(sweepEventsShown('SPY')[0].day==='today' && CALLS[0][1]===null, '3a live: today\'s events, no day passed (sweepEventsToday unchanged for every other reader)');
  REPLAY_ON=true; SHOWN='2026-09-04'; CALLS=[];
  ok(sweepEventsShown('SPY')[0].day==='2026-09-04' && CALLS[0][1]==='2026-09-04', '3b parked on Friday (the closed state) or replaying it: Friday\'s events');
  SHOWN='2026-09-08'; CALLS=[];
  ok(CALLS.length===0 && sweepEventsShown('SPY')[0].day==='today', '3c a replay of TODAY reads today\'s (no day passed)');
  REPLAY_ON=false;
  const SL=ex('sweepLevelsToday'), SE=ex('sweepEventsToday');
  ok(/function sweepLevelsToday\(sym, dayStr\)/.test(SL) && /overnightHL\(dayStr\)/.test(SL) && /futSessionBars\(1, dayStr\)/.test(SL) && /priorProfile\(dayStr\)/.test(SL) && /futSessionBars\(0, dayStr\)/.test(SL),
     '3d the level set follows the day: its overnight, its prior session, its prior profile, its IB');
  ok(/function sweepEventsToday\(sym, dayStr\)/.test(SE) && /futSessionBars\(0, dayStr\)/.test(SE) && /sweepLevelsToday\(sym, dayStr\)/.test(SE), '3e …and the scan runs over that day\'s RTH');
  ok(/function overnightHL\(dayStr\)/.test(src) && /function priorProfile\(dayStr\)/.test(src), '3f overnightHL and priorProfile take the day (and behave as before without one)');
  ok(/SW=sweepEventsShown\(sym\)\|\|\[\]/.test(ex('dayCandleSvg')) && /ev=sweepEventsShown\(sym\)\|\|\[\]/.test(ex('sweptLineHtml')), '3g the candle AND the SWEPT line read through the one door — they cannot disagree about the day');
}

// ---------- 4. the installer: mockups ride by last commit, six of them ----------
{
  const bi=fs.readFileSync('tools/build-installer.py','utf8');
  ok(/def _git_ct\(path\)/.test(bi) && /_mk_bin\.sort\(key=lambda f: \(_git_ct\(os\.path\.join\('mockups', f\)\), f\), reverse=True\)/.test(bi), '4a mockups are ranked by their last commit — the age that survives a clone — an uncommitted one newest');
  ok(/_MOCKUPS_RIDE = 6/.test(bi) && /_mk_bin\[:_MOCKUPS_RIDE\]/.test(bi), '4b six ride, not twelve');
}

// ---------- 5. the record ----------
{
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=P.roadmap.filter(r=>r.status==='next');
  ok(P.roadmap.some(r=>r.v==='15.79' && /E ROW BELOW THE LADDER/.test(r.title) && (r.status==='shipped' || nx.some(x=>x.v==='15.79'))) && P.roadmap.some(r=>r.v==='15.78' && r.status==='shipped'),
     '5a the plan: v15.78 shipped, v15.79 this build or shipped', nx.map(x=>x.v));
  const seedJs=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(P), '5b PLAN_SEED equals the file');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r16=R.rows.find(r=>r.id==='R-16');
  ok(r16 && r16.status==='implemented' && r16.version==='15.79' && r16.by==='operator' && /below the ladder/i.test(r16.text), '5c R-16 on Rec, by operator, implemented in v15.79', r16&&[r16.status,r16.version]);
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '5d REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.79/.test(cl) && cl.indexOf('## v15.79')<cl.indexOf('## v15.78'), '5e the CHANGELOG has the v15.79 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.79/.test(ls.slice(logAt>=0?logAt:0)), '5f the lesson log carries the v15.79 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(79|[89]\d)/.test(rn.slice(0,600)) && /shown day/i.test(rn), '5g the resume note is at v15.79 or later and names the shown-day sweeps');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1579.js')>=0 && /below the ladder/i.test(cfg.theWhatAndTheHow.eRow||''), '5h .gex-config.json names the move and this test');
}

console.log('test_v1579: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
