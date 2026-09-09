// test_v1588.js — (v15.88 + companion v1.19) THE AGENDA, BATCHED — his decisions of 2026-09-09, one at a time:
//   5  AHI · ALO · LHI · LLO at the standard hours ("just use whatever is standard") — Asia 17:00–02:00, London 02:00–08:30 CT
//   9  the King latch dropped — "match skylit"
//   10 companion v1.19 — NQ's full night, the weekly 5-minute bars (WH / WL / WPOC), NQ's base rates couriered ("yes")
//   4' the NQ vendor corpus parsed (188 sessions) — "your recommendation"
//   6  the first-hour turns counted, tagged, "and tag other hours" — the hour on every leg; R-3 in the pattern table
//   8  ⓪a per market — on the NQ chart the candle, the A row, the E row and the levels read NQ ("this build")
//   F-23 the corpus's ONH / ONL looked ahead (the day's post-close bars were in "the night") — corrected
const fs = require('fs'); const cp = require('child_process');
const src = fs.readFileSync('./v10.js', 'utf8'); const comp = fs.readFileSync('./current/gex-if-levels.user.js', 'utf8');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); } else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){d--; if(d===0){e=k;break;}} } return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }
const py = (args) => { try { return cp.execFileSync('python3', args, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'pipe'] }); } catch (e) { return 'ERR ' + (e.stdout || '') + (e.stderr || ''); } };

ok(/@version\s+15\.(8[8-9]|9\d)/.test(src) && /var GPTS_VERSION='15\.(8[8-9]|9\d)';/.test(src), '0a v15.88 or later in both spots');
ok(/@version\s+1\.19/.test(comp), '0b companion v1.19');
const LS={}; global.localStorage={ getItem:k=>(k in LS)?LS[k]:null, setItem:(k,v)=>{ LS[k]=v; }, removeItem:k=>{ delete LS[k]; } };

// ---- 1 · AHI · ALO · LHI · LLO ------------------------------------------------------------------------------------
global.mul=(a,b)=>a/(1/b); global.two=x=>{x=''+x;return x.length<2?'0'+x:x;};
eval((src.match(/var SESS_ASIA_A=[^\n]*;\n/)||[''])[0]);
const DAY=JSON.parse(fs.readFileSync('./data/2026-09-08.json','utf8')); let FB=DAY.futBars; global.futBarsLoad=()=>FB;
let MK='ES'; global.dispMarket=()=>MK; global.ctTodayStr=()=>'2026-09-08';
eval(ex('futDayKeyNum')); eval(ex('futDayKeyOf')); eval(ex('futSessionBars')); eval(ex('overnightHL')); eval(ex('sessionHL'));
{
  ok(SESS_ASIA_A===17*60 && SESS_ASIA_B===2*60 && SESS_LDN_B===8*60+30, '1a the standard hours: Asia 17:00–02:00 CT, London 02:00–08:30 CT', [SESS_ASIA_A,SESS_ASIA_B,SESS_LDN_B]);
  const S=sessionHL('2026-09-08');
  ok(S && S.ahi===7725.75 && S.alo===7694.75 && S.lhi===7718.25 && S.llo===7687.5, '1b 2026-09-08 from the courier\'s bars: AHI 7725.75 · ALO 7694.75 · LHI 7718.25 · LLO 7687.50 (= ONL)', S);
  ok(S.asia.n===180 && S.london.n===390, '1c the Labor Day evening began at 23:00 in Yahoo\'s window — 180 Asia bars still count (floor 150); London 390', [S.asia.n,S.london.n]);
  const T=val('LEVEL_TIER'); ok(T.AHI===1 && T.ALO===1 && T.LHI===1 && T.LLO===1 && T.LDNH===undefined && T.LDNL===undefined, '1d the four are tier 1 by his names; LDNH / LDNL are gone (renamed, not duplicated)');
  const sl=ex('sweepLevelsToday'); ok(/add\('ALO', SH\.alo, true, 0\); add\('AHI', SH\.ahi, false, 0\)/.test(sl) && /add\('LLO', SH\.llo, true, 0\); add\('LHI', SH\.lhi, false, 0\)/.test(sl) && !/add\('LDNL'/.test(sl), '1e the sweep set (the candle\'s labels, the SWEPT line, the read) carries the four');
  ok(/add\(SHs\.ahi,'AHI'\)/.test(ex('hlLevelHit')) && /\['ALO','ALO → LOD'\],\['AHI','AHI → HOD'\],\['LLO','LLO → LOD'\],\['LHI','LHI → HOD'\]/.test(src), '1f ⓪a\'s level lists and the H2 table rows carry them');
  const W=JSON.parse(fs.readFileSync('./data/es-1min/SWEEPS.json','utf8')); const L=W.lookup.level;
  ok(['ALO','AHI','LLO','LHI'].every(k=>L[k] && L[k].n>=100) && !L.LDNL && !L.LDNH, '1g SWEEPS.json (the corpus, 284 sessions) has the four by name with n ≥ 100', ['ALO','AHI','LLO','LHI'].map(k=>L[k]&&L[k].n));
  const ss=fs.readFileSync('./tools/study-sweeps.py','utf8'); ok(/asia = \[b for b in on if b\[0\] >= NIGHT or b\[0\] < LONDON\]/.test(ss) && /night = \[b for b in on if b\[0\] >= NIGHT or b\[0\] < RTH_A\] or on/.test(ss), '1h the corpus study: the Asia window, and (F-23) the night is BEFORE the session — the post-close bars are out of ONH / ONL');
  ok(L.ONL.n>=125 && Math.round(100*L.ONL.rate)<=22 && L.ONH.n>=154, '1i F-23: ONL ≤22% n≥125 (was 29% n=113 with the look-ahead), ONH n≥154 (was 140) — the corpus appends since v15.90, so ≥', [L.ONL.n, L.ONL.rate, L.ONH.n]);
  const fnd=fs.readFileSync('./skylit-docs/FINDINGS.md','utf8'); ok(/^## F-23 · THE CORPUS'S ONH \/ ONL LOOKED AHEAD/m.test(fnd) && /PARTLY SUPERSEDED by F-23/.test(fnd), '1j FINDINGS: F-23 written, F-14 flagged partly superseded');
}

// ---- 2 · THE KING LATCH IS OFF — match Skylit ----------------------------------------------------------------------
{
  global.KING_LATCH_KEY='k'; global.KING_LATCH_MS=Number((/var KING_LATCH_MS=(\d+);/.exec(src)||[])[1]);
  ok(KING_LATCH_MS===0, '2a KING_LATCH_MS is 0 — his word 2026-09-09: "match skylit"', KING_LATCH_MS);
  global.Date.now=()=>1788900000000; eval(ex('kingLatchTick'));
  let r=kingLatchTick(7700); ok(r.k===7700 && r.flap===false, '2b the first tick seats the crown');
  r=kingLatchTick(7705); ok(r.k===7705 && r.flap===false && r.rolledFrom===7700, '2c the tape\'s top moves → the crown moves on the SAME tick (no one-tick lag, no two-minute hold), rolledFrom kept', r);
  r=kingLatchTick(7700); ok(r.k===7700 && r.rolledFrom===7705, '2d …and back — the flap shows as Skylit shows it', r);
  ok(/if\(!\(KING_LATCH_MS>0\)\)/.test(ex('kingLatchTick')), '2e the hold can be turned back on by one number (the branch is on KING_LATCH_MS>0)');
}

// ---- 3 · COMPANION v1.19 — NQ's night, the weekly bars, NQ's base rates -----------------------------------------------
{
  ok(/\{ k:'NQ', y:'NQ=F', full:true \}/.test(comp) && /\{ k:'GC', y:'GC=F' \}/.test(comp), '3a NQ keeps the whole Globex day; GC / CL stay RTH-only');
  ok(/var FUT_WEEK_KEY='gpts_futweek_v1';/.test(comp) && /interval=5m&range=1mo/.test(comp) && /futParse\(res\.responseText\|\|'', false\)/.test(comp) && /FUT_WEEK_POLL_MS=6\*60\*60\*1000/.test(comp), '3b the weekly courier: 5-minute, one month, RTH-only rows, every 6 hours, its own key');
  ok(/futWeekCourier\(\); hlBaseCourier\(\);/.test(comp) && /FUT_WEEK_MARKETS=\[ \{ k:'ES', y:'ES=F' \}, \{ k:'NQ', y:'NQ=F' \} \]/.test(comp), '3c …on the tick, for ES and NQ');
  ok(/HLBASE_NQ_KEY='gpts_hodlod_base_nq_v1'/.test(comp) && /data\/futures\/NQ\/BASERATES\.json/.test(comp) && /hlBaseFetch\(HLBASE_NQ_URL, HLBASE_NQ_KEY, 'NQ base rates'\)/.test(comp) && /if\(!j \|\| !j\.corpus \|\| !\(j\.corpus\.sessions>0\) \|\| !j\.ladder \|\| !j\.ladder\.both\) return;/.test(comp), '3d NQ\'s BASERATES couriered under its own key, validated the same way');
  ok(/futWeek:function\(\)/.test(comp) && /hlBaseNq:function\(\)/.test(comp) && /futWeekPull:function\(\)/.test(comp), '3e the debug surface: __gexif.futWeek() · hlBaseNq() · futWeekPull()');
  // the panel's prior week from a weekly store: the ISO week before 2026-09-08 (W37) is W36 — Aug 31 → Sep 4
  eval((src.match(/var FUT_WEEK_KEY=[^\n]*;\n/)||[''])[0]); eval(ex('futWeekLoad')); eval(ex('isoWeekOf')); eval(ex('priorWeek'));
  global.hlDayShown=()=>'2026-09-08';
  const rows=[]; const ct=(y,mo,d,h,mi)=>Math.floor(Date.UTC(y,mo-1,d,h,mi)/1000)+5*3600;
  // W36: Mon Aug 31 … Fri Sep 4, 5-minute RTH rows (08:30–14:55), a marked high on Wed, a low on Mon, the busiest price 7740
  const days=[[8,31,7735,7745,7720],[9,1,7738,7752,7728],[9,2,7740,7761,7733],[9,3,7742,7755,7736],[9,4,7739,7751,7711.75]];
  days.forEach(([mo,d,o,h,l])=>{ for(let m=510;m<900;m+=5){ const hh=Math.floor(m/60), mi=m%60; const isHi=(m===600), isLo=(m===630);
    const heavy=(m>=600&&m<=660); rows.push([ct(2026,mo,d,hh,mi), 7740, isHi?h:(heavy?7740:7741), isLo?l:(heavy?7740:7739), 7740, heavy?500:100]); } });
  // and this week's rows (W37), which must NOT be read
  for(let m=510;m<900;m+=5){ rows.push([ct(2026,9,8,Math.floor(m/60),m%60), 7700, 7790, 7650, 7700, 100]); }
  LS['gpts_futweek_v1']=JSON.stringify({ ES:{ rows:rows, at:1 }, _v:1, _gran:'5m' });
  const PW=priorWeek('2026-09-08','ES');
  ok(PW && PW.pwh===7761 && PW.pwl===7711.75 && PW.wpoc===7740 && PW.days===5 && PW.week==='2026-W36' && PW.of==='2026-W37', '3f the prior ISO week (W36, Aug 31 → Sep 4; Sep 8 is in W37): PWH 7761 · PWL 7711.75 · WPOC 7740 — this week\'s 7790 / 7650 not read', PW);
  ok(priorWeek('2026-09-08','NQ')===null || (priorWeek('2026-09-08','NQ')&&priorWeek('2026-09-08','NQ').market), '3g a market the store lacks falls back to ES\'s rows (never invents)');
  LS['gpts_futweek_v1']=JSON.stringify({ ES:{ rows:rows.slice(0,40), at:1 } }); ok(priorWeek('2026-09-08','ES')===null, '3h fewer than three sessions in the prior week → no level (a stub is not a week)');
  delete LS['gpts_futweek_v1']; ok(priorWeek('2026-09-08','ES')===null, '3i no weekly store → null, no throw');
  ok(/add\('PWL', PWk\.pwl, true, 0\); add\('PWH', PWk\.pwh, false, 0\); add\('WPOC-', PWk\.wpoc, true, 0\); add\('WPOC\+', PWk\.wpoc, false, 0\)/.test(ex('sweepLevelsToday')), '3j the sweep set carries PWH / PWL / WPOC (both sides)');
  const W=JSON.parse(fs.readFileSync('./data/es-1min/SWEEPS.json','utf8')); ok(W.lookup.level['WPOC-'] && W.lookup.level['WPOC+'] && W.lookup.level['WPOC-'].n>=30, '3k the corpus study has WPOC- / WPOC+ (the week\'s profile on the 1-point grid)', [W.lookup.level['WPOC-']&&W.lookup.level['WPOC-'].n]);
}

// ---- 4 · THE NQ VENDOR CORPUS + ⓪a PER MARKET ----------------------------------------------------------------------
{
  const NQ=JSON.parse(fs.readFileSync('./data/futures/NQ/BASERATES.json','utf8'));
  ok(NQ.corpus.sessions>=195 && NQ.corpus.sources['NQ TestingData.txt']===188 && NQ.corpus.first==='2025-11-24' && NQ.corpus.last>='2026-09-08', '4a NQ BASERATES: the vendor\'s 188 sessions + the Yahoo days (195 through 09-08), provenance per session', [NQ.corpus.sessions, NQ.corpus.first, NQ.corpus.last]);
  ok(NQ.expected.rng_usd===Math.round(NQ.expected.rng_pts*20*10)/10 || Math.abs(NQ.expected.rng_usd-NQ.expected.rng_pts*20)<1, '4b NQ dollars are $20 a point', [NQ.expected.rng_pts, NQ.expected.rng_usd]);
  const sh=fs.readFileSync('./tools/study-hodlod.py','utf8'); ok(/if '\\t' in head and not head\.lower\(\)\.startswith\('symbol'\):/.test(sh) && /data\/es-1min\/NQ TestingData\.txt/.test(sh), '4c study-hodlod sniffs the tab / ISO vendor format and lists the NQ file first');
  const run=fs.readFileSync('./tools/nightly/run.py','utf8'); ok(/for mk in \('ES', 'NQ'\):/.test(run) && /the vendor wins each; the two read side by side below/.test(run), '4d the nightly reports the vendor / Yahoo overlap per market (NQ has four shared days: 08-24 → 08-27)');
  ok(/RE-BAKED BY tools\/bake-hodlod\.py/.test(src) && /bake-hodlod: the literal EQUALS the file/.test(py(['tools/bake-hodlod.py','--check'])), '4e the ES literal still equals its file');
  // per market
  ok(/function dispMarket\(\)/.test(src) && /var mk=\(typeof dispMarket==='function'\)\?dispMarket\(\):'ES';\n    var M=o\[mk\]\|\|o\.ES/.test(ex('futSessionBars')), '4f futSessionBars reads the chart\'s market');
  const PTU=val('PT_USD'); ok(PTU.ES===50 && PTU.NQ===20 && /\(typeof ptUsd==='function'\?ptUsd\(\):ES_USD_PER_PT\)/.test(ex('hodLod')), '4g the contract multiplier per market, typeof-guarded in the hot path');
  // hodLod on the NQ bars of 2026-09-08 (the courier's RTH-only rows in the day file)
  global.HL_TOOL_A=8*3600+27*60; global.HL_TOOL_B=15*3600; global.HL_TOOL_BAR=180; global.ES_USD_PER_PT=50;
  global.ctNowSecOfDay=()=>15*3600; global.inReplay=()=>false; global.showingStaleBook=()=>false; global.dispIsFut=()=>true; global.dispR=()=>41.12;
  global.rmemo=(k,f)=>f(); global.rmemoNext=()=>{}; global.HLBASE_KEY='gpts_hodlod_base_v1'; global.HLBASE_MIN_SESSIONS=120; global.HLBASE_MIN_BUCKET=50; global.HODLOD_BASE=val('HODLOD_BASE');
  global.closedCandles=()=>[]; global.replayOn=()=>false;
  eval((src.match(/var HLBASE_NQ_KEY=[^\n]*;\n/)||[''])[0]); eval((src.match(/var PT_USD=[^\n]*;\n/)||[''])[0]);
  eval(ex('ptUsd')); eval(ex('hlBaseNormalise')); eval(ex('hodlodBase')); eval(ex('hlNoBase')); eval(ex('hodlodBaseFor')); eval(ex('hlTier')); eval(ex('measureBars')); eval(ex('measureBarsRaw')); eval(ex('hlToolBars')); eval(ex('hlClock')); eval(ex('hlDur')); eval(ex('hodLod'));
  MK='NQ'; const Dn=hodLod('QQQ');
  ok(Dn.ok && Dn.src==='NQ' && Dn.hod===29686.5 && Dn.lod===29424.75 && Dn.lodT===9*3600+12*60, '4h on the NQ chart hodLod measures NQ\'s own bars: HOD 29686.50 · LOD 29424.75 in the bar ending 09:12 (src NQ)', [Dn.ok, Dn.src, Dn.hod, Dn.lod, Dn.lodT, Dn.why]);
  ok(Math.abs(Dn.rngUsd-(29686.5-29424.75)*20)<1e-6, '4i …and the range in NQ dollars ($20 a point): $'+Dn.rngUsd, Dn.rngUsd);
  ok(hodlodBase('NQ')===null && hodlodBaseFor('Tue').none===true && hodlodBaseFor('Tue').n===0 && hlTier(200, null)===null, '4j no NQ base before the courier delivers → hodlodBase null, hodlodBaseFor the empty base (none:true, n 0), hlTier null — never a borrowed ES number');
  LS['gpts_hodlod_base_nq_v1']=JSON.stringify({ at:1, base:NQ }); const Bn=hodlodBase('NQ');
  ok(Bn && Bn.src==='courier' && Bn.market==='NQ' && Bn.n===NQ.corpus.sessions && Bn.ladder.length===5, '4k with the courier\'s NQ file the base is NQ\'s: n '+(Bn&&Bn.n)+', its own ladder', Bn&&[Bn.src,Bn.market,Bn.n]);
  MK='ES'; const Be=hodlodBase(); ok(Be && Be.market==='ES' && Be.src==='baked' && Be.n===val('HODLOD_BASE').n, '4l on the ES chart the base is ES\'s (baked until the courier delivers)');
  const De=hodLod('SPY'); ok(De.ok && De.src==='ES' && De.open===7715 && De.hod===7717.75 && De.lod===7676.75, '4m …and the ES read is unchanged (open 7715, HOD 7717.75; the day file\'s bars end 14:46, so its low is 7676.75 — the 14:59 LOD lives in the corpus CSV)', [De.src, De.open, De.lod]);
  ok(/\+g3esc\(base\.market\|\|'ES'\)\+' 1-min/.test(src) && /base\.none\?'no base for this market yet'/.test(src), '4n the ⓪a header names the market and says when it has no base');
  // the nightly on his machine after v15.87: it re-ran every ten minutes (the run's own BASERATES was "older than the log" by seconds)
  const tk=fs.readFileSync('./tools/nightly/tick.py','utf8'); ok(/STALE_S = 3600/.test(tk) && /os\.path\.getmtime\(lp\) - STALE_S/.test(tk) && /tick\.py selftest ok/.test(py(['tools/nightly/tick.py','--selftest'])), '4o tick.py: an output is pasted only when it is an HOUR older than the log (v15.87\'s one-second fence re-ran the nightly every ten minutes on 09-08 evening); the selftest holds the run\'s own 30-second-older output as current');
  const run2=fs.readFileSync('./tools/nightly/run.py','utf8'); ok(/def refresh_futures\(days, keep_last=None\)/.test(run2) && /_glob\.glob\(os\.path\.join\('data', 'futures', '\*-tail\.json'\)\)/.test(run2), '4p the nightly harvests EVERY day file (the installer does not carry data/futures/) plus the tail supplements');
  const bi=fs.readFileSync('./tools/build-installer.py','utf8'); ok(/_glob\.glob\('data\/futures\/\*-tail\.json'\)/.test(bi) && fs.existsSync('./data/futures/2026-09-08-tail.json'), '4q the tail supplement rides the installer (the 09-08 14:44–14:59 minutes from the live store)');
}

// ---- 5 · THE HOUR ON EVERY DEFLECTION; THE FIVE FIRST-HOUR TURNS; R-3 -------------------------------------------------
{
  const E=JSON.parse(fs.readFileSync('./learning/deflections/examples.json','utf8'));
  const legs=[].concat(...E.examples.map(e=>e.legs));
  ok(legs.length===31 && legs.every(l=>/^H[1-7]$/.test(l.hour)), '5a 31 legs, every one tagged H1–H7', legs.filter(l=>!/^H[1-7]$/.test(l.hour)).map(l=>l.leg));
  const E5=E.examples.find(e=>e.id==='E005'), E6=E.examples.find(e=>e.id==='E006');
  ok(E5.legs.filter(l=>l.uncircled).map(l=>l.leg).join()==='m1,m2,m3,m4' && E6.legs.filter(l=>l.uncircled).map(l=>l.leg).join()==='m1', '5b the four ES turns and the NQ LOD are legs, marked uncircled');
  ok(E5.legs.find(l=>l.leg==='m2').data.indexOf('ONL / LLO 7687.50 swept 7.5 pts')>=0 && E6.legs.find(l=>l.leg==='m1').px.indexOf('43 pts through the PDL 29468')>=0, '5c the two sweep-turns carry their liquidity level and the sweep depth');
  ok(/HIS RULING 2026-09-09/.test(E5.open) && /tag other hours/.test(E5.open), '5d E005.open carries his ruling verbatim');
  const L7=E.rules.find(r=>r.id==='L7'); ok(L7 && /CLASS, NOT A RULE/.test(L7.rule) && L7.n===31 && /Withdrawn: 'the first 30 minutes matter/.test(L7.note), '5e L7 is the hour class over 31 legs; the corpus\'s first-30-minutes number withdrawn (F-23)');
  const seedJs=JSON.parse(/var LEARN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(E), '5f LEARN_SEED equals the file');
  ok(/<th title="the session hour/.test(src) && /g\.uncircled\?' <span class="dm"/.test(src), '5g the Learn tab\'s leg table has the hour column and the ⚐ on an uncircled leg');
  // R-3: the clock as a class, in both twins
  global.PAT_GROW_PCT=val('PAT_GROW_PCT'); const PC=val('PAT_CLASSES'); ok(PC.some(p=>p[0]==='hour:1') && PC.some(p=>p[0]==='hour:7') && PC.some(p=>p[0]==='clock:first') && PC.some(p=>p[0]==='clock:mid') && PC.some(p=>p[0]==='clock:last'), '5h PAT_CLASSES has the seven hours and the three roll-ups');
  eval(ex('hourClasses'));
  const t0=Date.UTC(2026,8,8,13,30); // 08:30 CT
  ok(hourClasses(t0).join()==='hour:1,clock:first' && hourClasses(t0+59*60000).join()==='hour:1,clock:first' && hourClasses(t0+60*60000).join()==='hour:2,clock:mid' && hourClasses(t0+300*60000).join()==='hour:6,clock:mid' && hourClasses(t0+330*60000).join()==='hour:7,clock:last' && hourClasses(t0+389*60000).join()==='hour:7,clock:last' && hourClasses(t0+390*60000).length===0 && hourClasses(null).length===0,
     '5i hourClasses: 08:30 → H1 first · 09:30 → H2 mid · 13:30 → H6 mid · 14:00 → H7 last · 15:00 → none · null → none');
  const pyH=py(['-c','import sys; sys.path.insert(0,"tools/nightly"); import patterns as P; import json; t0='+t0+'; print(json.dumps([P.hour_classes(t0),P.hour_classes(t0+59*60000),P.hour_classes(t0+60*60000),P.hour_classes(t0+300*60000),P.hour_classes(t0+330*60000),P.hour_classes(t0+389*60000),P.hour_classes(t0+390*60000),P.hour_classes(None)]))']).trim();
  ok(JSON.stringify(JSON.parse(pyH))===JSON.stringify([hourClasses(t0),hourClasses(t0+59*60000),hourClasses(t0+60*60000),hourClasses(t0+300*60000),hourClasses(t0+330*60000),hourClasses(t0+389*60000),hourClasses(t0+390*60000),hourClasses(null)]), '5j tools/nightly/patterns.py hour_classes gives the same classes on the same eight clocks (one definition, two languages)', pyH.slice(0,200));
  const R=JSON.parse(fs.readFileSync('./learning/recommendations.json','utf8')); const r3=R.rows.find(r=>r.id==='R-3');
  ok(r3 && r3.status==='implemented' && r3.version==='15.88', '5k R-3 implemented in v15.88', r3&&[r3.status,r3.version]);
}

// ---- 6 · THE RECORDS ----------------------------------------------------------------------------------------------
{
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); const head=cl.split('## v15.88')[1].split('\n## ')[0];
  ok(/^## v15\.88 /m.test(cl) && /match skylit/.test(head) && /whatever is standard/.test(head) && /tag other hours/.test(head) && /F-23/.test(head), '6a CHANGELOG v15.88 — his words and F-23');
  const de=fs.readFileSync('session-state/DECISIONS.md','utf8'); ok(/"match skylit"/.test(de) && /"just use whatever is standard"/.test(de) && /tag other hours/.test(de), '6b DECISIONS carries the three words');
  const le=fs.readFileSync('session-state/LESSONS.md','utf8'); const top=(/^### (v[\d.]+)/m.exec(le)||[])[1]; ok(/^### v15\.88 /m.test(le), '6c LESSONS carries the v15.88 entry (the newest-first log under §2)', top);
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/^2026-09-09/.test(cfg.version||'') && cfg.version!=='2026-09-09a', '6d .gex-config.json re-stamped', cfg.version);
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=(P.roadmap||[]).find(r=>r.status==='next'); const r88=(P.roadmap||[]).find(r=>r.v==='15.88'); ok(r88 && /agenda|batched/i.test(r88.title||'') && /shipped|next/.test(r88.status||''), '6e the plan carries 15.88 (shipped or next)', r88&&[r88.v,r88.status,(r88.title||'').slice(0,60)]);
  const li=fs.readFileSync('session-state/LOCKED-ITEMS.md','utf8'); ok(/AHI · ALO · LHI · LLO/.test(li) && /BUILT.*v15\.88|v15\.88.*BUILT/i.test(li), '6f LOCKED-ITEMS: the four levels leave the list as BUILT in v15.88');
  const R=JSON.parse(fs.readFileSync('./learning/recommendations.json','utf8')); const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '6g REC_SEED equals the file');
}
console.log('test_v1588: ' + pass + ' passed, ' + fail + ' failed');
process.exit(fail ? 1 : 0);
