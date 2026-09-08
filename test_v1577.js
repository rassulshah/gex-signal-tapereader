#!/usr/bin/env node
// test_v1577.js — (v15.77) THE E ROW ON TOP OF THE HOD LINE, PER WEEKDAY (R-14). Operator, 2026-09-08, on his
// own tool's strip: "I want to see the expected row on top of the HOD at the top … I dont need Rly · Done · PB ·
// Num · Ret · Risk · Ext · Tgt · Rwd · Dur · Time." Then, on the two mockups: "use mockup 2, because it compares
// friday with fridays in the past and mondays with past mondays … this is a type of seasonality."
// Pinned here: the study's weekday blocks (pooled block untouched), the normaliser, the weekday picker, the
// basis switch (expected fields per weekday, the LADDER pooled), the row's chips and cells, the struck fields'
// absence, the render order, and the record.
// (sloppy mode on purpose: a direct eval must declare the panel's functions into this scope)
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } } return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }

// ---------- 0. the version ----------
ok(/@version\s+15\.(7[7-9]|[89]\d)/.test(src) && /var GPTS_VERSION='15\.(7[7-9]|[89]\d)';/.test(src), '0a v15.77 or later in both spots');

// ---------- 1. the study: one block per weekday, the pooled block untouched ----------
const BR=JSON.parse(fs.readFileSync('./data/es-1min/BASERATES.json','utf8'));
{
  ok(BR.byWeekday && ['Mon','Tue','Wed','Thu','Fri'].every(d=>BR.byWeekday[d]), '1a BASERATES.json carries byWeekday Mon..Fri');
  ok(Object.values(BR.byWeekday).every(v=>v.sessions>=40 && v.sessions<=70) && Object.values(BR.byWeekday).reduce((a,v)=>a+v.sessions,0)===BR.corpus.sessions,
     '1b the five blocks partition the corpus (55-60 each, summing to 284)', Object.values(BR.byWeekday).map(v=>v.sessions));
  ok(BR.expected.took_min===33.5 && BR.expected.rng_pts===61.4 && BR.expected.first_clock===32608.6 && BR.wickFamily.median.wick_n===252,
     '1c the POOLED block is byte-for-byte what it was — the ladder and the pooled E fields did not move', [BR.expected.took_min,BR.expected.rng_pts]);
  ok(Object.values(BR.byWeekday).every(v=>/trimmed mean/.test(v.expected.statistic||'') && v.expected.outliers_excluded && v.wickFamily && v.wickFamily.median),
     '1d every weekday block declares the same statistic and carries its own exclusions');
  ok(BR.byWeekday.Fri.expected.took_min===19.1 && BR.byWeekday.Tue.expected.took_min===45.6,
     '1e the weekdays differ — Fridays reach the first extreme in ~19m, Tuesdays in ~46m (the seasonality he named, measured)', [BR.byWeekday.Fri.expected.took_min, BR.byWeekday.Tue.expected.took_min]);
  ok(Object.values(BR.byWeekday).every(v=>v.recent && v.recent.n<=6 && v.recent.green+v.recent.red===v.recent.n && Array.isArray(v.recent.days) && v.recent.days.length===v.recent.n),
     '1f `recent` is his "last 6 per weekday": a COUNT of green and red with the days it stands on');
  const study=fs.readFileSync('./tools/study-hodlod.py','utf8');
  ok(/def expected_block\(rows\)/.test(study) && /def weekday_blocks\(rows\)/.test(study) && /res\['byWeekday'\] = weekday_blocks\(rows\)/.test(study),
     '1g the study computes the blocks with the pooled block\'s own functions');
  ok(/green=\(b\[-1\]\[3\] > b\[0\]\[4\]\)/.test(study), '1h a session\'s colour is RTH close against RTH open — his tool\'s "Red Day / Green Day"');
}

// ---------- 2. the panel: the normaliser, the weekday picker, the basis switch ----------
global.mul=(a,b)=>a*b; global.two=x=>{x=''+x;return x.length<2?'0'+x:x;};
global.g3esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
global.g3tip=t=>t?(' title="'+g3esc(t)+'"'):'';
global.swallow=(tag,e)=>{ global.__sw=(global.__sw||[]).concat([tag+': '+(e&&e.message||e)]); };
global.HODLOD_BASE=val('HODLOD_BASE'); global.HLBASE_MIN_SESSIONS=val('HLBASE_MIN_SESSIONS'); global.HLBASE_MIN_BUCKET=val('HLBASE_MIN_BUCKET');
global.HLBASE_MIN_DOW=val('HLBASE_MIN_DOW'); global.HL_DOWS=val('HL_DOWS'); global.HLBASE_KEY=val('HLBASE_KEY');
let LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,v)=>{LS[k]=String(v);} };
let REPLAY_ON=false; global.REPLAY={ on:false, day:null }; global.replayOn=()=>REPLAY_ON;
global.sessionDayStr=()=>'2026-09-08'; global.ctTodayStr=()=>'2026-09-08';
eval(['hlBaseNormalise','hlBaseByDow','hlDowOf','hlDayShown','hodlodBaseFor','hodlodBase','hlDur','hlClock12','hlERowState','hlERowHtml'].map(ex).join('\n'));
{
  ok(HLBASE_MIN_DOW===40 && HL_DOWS.join()==='Mon,Tue,Wed,Thu,Fri', '2a the floor is 40 sessions per weekday; the five names');
  const N=hlBaseNormalise(BR);
  ok(N && N.byDow && Object.keys(N.byDow).length===5 && N.byDow.Fri.n===55 && N.byDow.Fri.tookMin===19.1 && N.byDow.Fri.wick.bop===BR.byWeekday.Fri.wickFamily.median.bop,
     '2b the courier payload normalises into byDow with the pooled base\'s field names', N&&N.byDow&&N.byDow.Fri);
  ok(N.byDow.Fri.recent && N.byDow.Fri.recent.n===6 && N.byDow.Fri.recent.last==='2026-08-21', '2c …with the recent six and their newest day', N.byDow.Fri.recent);
  ok(N.n===284 && N.tookMin===33.5 && N.ladder.length===5, '2d the pooled fields and the ladder are untouched by the weekday blocks');
  // a thin weekday is left out; a malformed one is skipped, never half-read
  const thin=JSON.parse(JSON.stringify(BR)); thin.byWeekday.Tue.sessions=30; thin.byWeekday.Wed.expected.first_clock='x';
  const Nt=hlBaseNormalise(thin);
  ok(Nt && !Nt.byDow.Tue && !Nt.byDow.Wed && Nt.byDow.Mon && Nt.byDow.Fri, '2e a weekday under 40 sessions or with a malformed clock is left out; the others stay', Object.keys(Nt.byDow));
  ok(HODLOD_BASE.byDow && Object.keys(HODLOD_BASE.byDow).length===5 && HODLOD_BASE.byDow.Fri.n===BR.byWeekday.Fri.sessions && HODLOD_BASE.byDow.Fri.tookMin===BR.byWeekday.Fri.expected.took_min,
     '2f the BAKED base carries the same five blocks as the file (the courier is not needed for the row to be right)');
  ok(hlDowOf('2026-09-04')==='Fri' && hlDowOf('2026-09-08')==='Tue' && hlDowOf('2026-09-07')==='Mon' && hlDowOf('2026-09-06')===null && hlDowOf('garbage')===null && hlDowOf(null)===null,
     '2g hlDowOf: a weekday name, null on a weekend or garbage');
  ok(hlDayShown()==='2026-09-08', '2h live, the shown day is the recorder\'s session day');
  REPLAY_ON=true; REPLAY.day='2026-09-04';
  ok(hlDayShown()==='2026-09-04', '2i in a replay (his, or the closed state\'s park) it is the replayed day');
  REPLAY_ON=false; REPLAY.day=null;
  // the basis switch
  const F=hodlodBaseFor('Fri');
  ok(F.basis && F.basis.dow==='Fri' && F.basis.n===55 && F.basis.pooled===false, '2j hodlodBaseFor(Fri) stands on the 55 Fridays', F.basis);
  ok(F.tookMin===19.1 && F.firstClock===BR.byWeekday.Fri.expected.first_clock && F.wick.bop===BR.byWeekday.Fri.wickFamily.median.bop && F.lodFirstPct===BR.byWeekday.Fri.sequence.pct_LOD_first,
     '2k …every EXPECTED field is the weekday\'s', [F.tookMin, F.firstClock]);
  ok(JSON.stringify(F.ladder)===JSON.stringify(hodlodBase().ladder) && F.n===284, '2l …and the LADDER (hold rates by age) and its n stay POOLED — split five ways the rungs go thin');
  ok(F.pool && F.pool.tookMin===33.5, '2m the pooled base rides along as .pool');
  const P=hodlodBaseFor(null), S=hodlodBaseFor('Sat');
  ok(P.basis.pooled===true && P.tookMin===33.5 && S.basis.pooled===true, '2n no weekday (a weekend, a bad string) → the pooled row, and basis says so');
  // a courier payload without this weekday → pooled, honestly
  LS[HLBASE_KEY]=JSON.stringify({ at:Date.now(), base:thin });
  const T=hodlodBaseFor('Tue');
  ok(T.basis.pooled===true && T.basis.dow==='Tue' && T.tookMin===33.5, '2o a weekday the courier could not deliver falls back to the pooled row with basis.pooled=true', T.basis);
  LS={};
  // the baked base is generated without the floor — hodlodBaseFor applies it itself
  const keepWed=HODLOD_BASE.byDow.Wed; HODLOD_BASE.byDow.Wed=Object.assign({}, keepWed, { n:30 });
  const Wd=hodlodBaseFor('Wed');
  ok(Wd.basis.pooled===true && Wd.tookMin===33.5, '2p a baked weekday block under the floor (n=30) is not stood on either', Wd.basis);
  HODLOD_BASE.byDow.Wed=keepWed;
}

// ---------- 3. the row ----------
{
  global.sessionDayStr=()=>'2026-09-04';   // the shown day is the Friday the corpus ends on, so the recent six are 14 days old — not yet stale
  const D=(first)=>({ ok:true, first:first, clock:36000 });
  const baseF=hodlodBaseFor('Fri');
  const st=hlERowState(D('HOD'), baseF, false);
  ok(st.ok && st.f1==='HOD' && st.f2==='LOD' && st.first==='HOD', '3a with bars, the row reads today\'s first extreme first (HOD then LOD), as his strip does');
  ok(st.firstClock===baseF.firstClock && st.took===19.1 && st.wend===mul(8,3600)+mul(30,60)+baseF.wick.wick*60, '3b W.End = 08:30 + the expected wick, the other fields the weekday\'s', [st.took, st.wend]);
  const h=hlERowHtml('SPY', D('HOD'), baseF, false);
  ok(/<span class="et">E · FRI n=55<\/span>/.test(h), '3c the E tag names the weekday and its n', h.slice(0,120));
  ok(/1ST HOD<\/span>/.test(h), '3d the first-extreme chip');
  const cells=[...h.matchAll(/<span class="ec"><i>([^<]+)<\/i><b[^>]*>([^<]+)<\/b><\/span>/g)].map(m=>[m[1],m[2]]);
  ok(cells.map(c=>c[0]).join('|')==='HOD|took|BOP|wick|W.End|wick%|MUD|LOD|HL gap|HL rng', '3e the cells, in his strip\'s order, minus the eleven he struck', cells.map(c=>c[0]));
  ok(cells.every(c=>/^~/.test(c[1])), '3f every value wears the ~ — a trimmed mean, never a forecast', cells.map(c=>c[1]));
  ok(cells[0][1]==='~'+hlClock12(baseF.firstClock) && cells[1][1]==='~19m' && cells[9][1]==='~63.7pts · $'+Math.round(baseF.rngUsd).toLocaleString(),
     '3g the clock in 12h, took in minutes, the range in points and dollars', [cells[0][1], cells[1][1], cells[9][1]]);
  ok(!/Rly|Done|\bPB\b|\bNum\b|\bRet\b|Risk|\bExt\b|Tgt|Rwd|\bDur\b|>Time</.test(h.replace(/title="[^"]*"/g,'')), '3h none of Rly · Done · PB · Num · Ret · Risk · Ext · Tgt · Rwd · Dur · Time is on the row');
  ok(/chip ev"[^>]*>3\/3 EVEN</.test(h), '3i the colour chip: the last six Fridays are 3 green / 3 red → "3/3 EVEN", a count');
  // a leaning six, a stale six, no six
  const bG=hodlodBaseFor('Fri'); bG.recent={ n:6, green:4, red:2, last:'2026-09-04' };
  ok(/chip gd"[^>]*>GREEN 4\/6</.test(hlERowHtml('SPY', D('HOD'), bG, false)), '3j 4 green of 6 → "GREEN 4/6" in green');
  const bR=hodlodBaseFor('Fri'); bR.recent={ n:6, green:1, red:5, last:'2026-09-04' };
  ok(/chip rd"[^>]*>RED 5\/6</.test(hlERowHtml('SPY', D('HOD'), bR, false)), '3k 5 red of 6 → "RED 5/6" in red');
  const bS=hodlodBaseFor('Fri'); bS.recent={ n:6, green:4, red:2, last:'2026-07-17' };
  const hS=hlERowHtml('SPY', D('HOD'), bS, false);
  ok(/chip gd stale"/.test(hS) && /STALE: the corpus has not been appended/.test(hS), '3l a six whose newest day is more than 14 days old is marked STALE and the hover says why');
  const bN=hodlodBaseFor('Fri'); bN.recent=null;
  ok(!/class="chip (gd|rd|ev)/.test(hlERowHtml('SPY', D('HOD'), bN, false)), '3m no recent six → no colour chip, never a guess');
  // before the first bar: the weekday's LOD-first share stands in for the first extreme
  const hN=hlERowHtml('SPY', { ok:false, why:'no candles' }, baseF, true);
  ok(/1ST LOD 51%<\/span>/.test(hN) && /<i>LOD<\/i>/.test(hN.split('<span class="ec">')[1]||''), '3n no bars: "1ST LOD 51%" (Fridays) and the LOD clock reads first', hN.slice(0,400));
  // the pooled fallback says ALL
  ok(/<span class="et">E · ALL n=284<\/span>/.test(hlERowHtml('SPY', D('LOD'), hodlodBaseFor(null), false)), '3o the pooled row is tagged ALL with its n');
  ok(/his seasonality: Fridays against past Fridays/.test(h) && /BOP 47 · wick 45 · MUD 51/.test(h) && /hold rates in the line below stay pooled/.test(h),
     '3p the hover states the basis, the wick n per field, and that the hold rates stay pooled');
  ok(hlERowHtml('SPY', D('HOD'), { }, false)==='' && hlERowHtml('SPY', D('HOD'), null, false)==='', '3q no base → no row, no throw');
  global.sessionDayStr=()=>'2026-09-18';
  ok(/chip ev stale"/.test(hlERowHtml('SPY', D('HOD'), baseF, false)), '3r …and on 2026-09-18 the same six (newest 08-21) ARE stale: 28 days, corpus not appended');
  global.sessionDayStr=()=>'2026-09-08';
}

// ---------- 4. the face ----------
{
  const sd=ex('secDay');
  ok(/var base=hodlodBaseFor\(dow\)/.test(sd) && /dow=hlDowOf\(dayShown\)/.test(sd) && /dayShown=hlDayShown\(\)/.test(sd), '4a secDay stands its base on the shown day\'s weekday');
  const iE=sd.indexOf("hlERowHtml(sym, D, base, NOREAD)"), iR=sd.indexOf('<div class="g3dayread"');
  ok(iE>0 && iR>iE, '4b the E row is emitted BEFORE the HOD line — on top of it');
  ok(/expected gap '\+hlDur\(base\.gapMin\)/.test(sd) && /the E row(\\u2019|’)s basis/.test(sd) && /\(pooled, n='\+T\.n\+'\)/.test(sd),
     '4c the read\'s timing prose says "expected gap", names the E row\'s basis, and marks the replacement rate as pooled');
  ok(/#gpts-body \.g3erow\{display:flex;flex-wrap:wrap/.test(src) && /\.g3erow \.chip\.stale\{opacity:\.55\}/.test(src) && /\.g3erow \.et\{/.test(src), '4d the stylesheet carries the row (one line at 760px, mockup-e-row)');
  ok(/try\{ h\+=hlERowHtml\(sym, D, base, NOREAD\); \}catch\(eER\)\{ swallow\('hlERowHtml', eER\); \}/.test(sd), '4e the row cannot take the section down with it');
}

// ---------- 5. the record ----------
{
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=P.roadmap.filter(r=>r.status==='next');
  ok(P.roadmap.some(r=>r.v==='15.77' && /E ROW/.test(r.title) && (r.status==='shipped' || nx.some(x=>x.v==='15.77'))) && P.roadmap.some(r=>r.v==='15.76' && r.status==='shipped'),
     '5a the plan: v15.76 shipped, v15.77 this build or shipped', nx.map(x=>x.v));
  ok(P.roadmap.some(r=>/SEASONALITY TRACKED/.test(r.title) && /^15\.(7[8-9]|[89]\d)$/.test(r.v) && r.status!=='shipped' || P.roadmap.some(r=>/SEASONALITY TRACKED/.test(r.title) && r.status==='shipped')), '5b the seasonality tracking (his two charts) sits after this build, awaiting its mockup (or shipped)');
  const seedJs=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(P), '5c PLAN_SEED equals the file');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r14=R.rows.find(r=>r.id==='R-14');
  ok(r14 && r14.status==='implemented' && r14.version==='15.77' && r14.by==='operator' && /weekday/i.test(r14.text), '5d R-14 on Rec, by operator, implemented in v15.77', r14&&[r14.status,r14.version]);
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '5e REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.77/.test(cl) && cl.indexOf('## v15.77')<cl.indexOf('## v15.76'), '5f the CHANGELOG has the v15.77 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.77/.test(ls.slice(logAt>=0?logAt:0)), '5g the lesson log carries the v15.77 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(7[7-9]|[89]\d)/.test(rn.slice(0,600)) && /seasonality/i.test(rn), '5h the resume note is at v15.77 or later and names the seasonality');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/weekday/i.test(cfg.theWhatAndTheHow.eRow||'') && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1577.js')>=0, '5i .gex-config.json names the E row and this test');
  ok(fs.existsSync('mockups/mockup-e-row.png') && fs.existsSync('mockups/mockup-e-row.js'), '5j the mockup he chose from is in the repo');
}

console.log('test_v1577: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
