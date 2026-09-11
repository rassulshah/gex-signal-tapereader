#!/usr/bin/env node
// test_v1578.js — (v15.78) THE DAY CANDLE BESIDE THE LADDER, WITH THE SWEEP LABELS (R-15). Operator, 2026-09-08, on a
// screenshot of his panel: "do you see the black space on the right of the app … I want the daily candle that is
// developing to be displayed there. you have the code for this and the labels also already." On the mockup: "you have
// to add the sweep labels. after doing that, build."
// Pinned here: the candle's frame option (the old 98x137 candle unchanged; the tall frame centred with bigger labels),
// the SWEPT line's labels on the right of the bar in its colours and the reversal names on the left, the slot after the
// grid, candleFit's measurement (room or nothing, the grid's height, the header, the wrap guard), the CSS, the record.
// (sloppy mode on purpose: a direct eval must declare the panel's functions into this scope)
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } } return src.slice(m.index,e+1); }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }

// ---------- 0. the version ----------
ok(/@version\s+15\.(7[8-9]|[89]\d)/.test(src) && /var GPTS_VERSION='15\.(7[8-9]|[89]\d)';/.test(src), '0a v15.78 or later in both spots');

// ---------- 1. the candle: the same marks, in a frame of the caller's size ----------
// ⚠ (v15.80) RE-PINNED. The v15.78 labels — the SWEPT line's events stacked at the wick tips on the right, the reversal
// names on the left — lasted one session. Operator, 2026-09-08: "the levels that are swept are the only ones that should
// be indicated … aligned based on the y axis which should be the price axis." The right column is the PRICE AXIS now
// (test_v1580 §2 pins its geometry); what THIS section keeps is the frame contract of v15.78: the same candle at the
// caller's size, the bar centred, the labels sized for the tall frame.
global.mul=(a,b)=>a*b; global.two=x=>{x=''+x;return x.length<2?'0'+x:x;};
global.g3esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
global.g3tip=t=>t?(' title="'+g3esc(t)+'"'):'';
global.swallow=(tag,e)=>{ global.__sw=(global.__sw||[]).concat([tag+': '+(e&&e.message||e)]); };
global.DAYCOL_ROW=13; global.DAYCOL_HD=16; global.DAYCOL_N=9; global.ES_USD_PER_PT=50;
global.frameNum=x=>(Math.round(x*100)/100).toString();
eval(['levelTier','hlClock','hlDur','hlMudLabel','dayCandleSvg','sweepEventsShown'].map(ex).join('\n'));   // (v15.79) the candle reads the shown day's sweeps; (v15.80) MU/MD
global.replayOn=()=>false; global.hlDayShown=()=>'2026-09-08'; global.ctTodayStr=()=>'2026-09-08';
global.LEVEL_TIER=val('LEVEL_TIER'); global.FUTMODE={ fam:'ES' }; global.irtRatio=()=>({ r:10.03 });
let SB={ open:7700, close:7712 }; global.sessionBody=()=>SB;
global.displayScale=()=>({ scale:10.05 });
let SW=[]; global.sweepEventsToday=()=>SW;
const D={ ok:true, open:7700, hod:7735, lod:7688, hodT:mul(11,3600)+mul(54,60), lodT:mul(12,3600)+mul(47,60), first:'HOD', second:'LOD', took:204, gap:53, mud:0, scale:1, rngUsd:2350, isFut:true };  // (v15.94) took = HOD's from the open (08:30→11:54 = 3h24); gap 53 → LOD at 12:47 = 4h17 from the open
const P={ ok:true, ptPx:7728, lcMin:130 };
{
  const s0=dayCandleSvg('SPY', D, P);
  ok(/^<svg class="g3cdl" viewBox="0 0 98 137" width="98" height="137">/.test(s0), '1a without opts the candle is the DAY table\'s 98 x 137, class g3cdl, unchanged', s0.slice(0,70));
  ok(/<line x1="48" y1=/.test(s0) && !/x="96" y=/.test(s0), '1b …bar at x=48; (v15.80) no reversal names in the right column');
  ok(!/SWEPT/.test(s0) && !/class="g3cx sw"/.test(s0), '1c …and no sweep labels while there are no sweeps');
  const s1=dayCandleSvg('SPY', D, P, { w:230, h:264 });
  ok(/^<svg class="g3cdl tall" viewBox="0 0 230 264" width="230" height="264">/.test(s1), '1d with {w,h} the frame is the caller\'s and the class says tall', s1.slice(0,70));
  ok(/<line x1="115" y1="30\.0" x2="115" y2="224\.0"/.test(s1), '1e the bar is centred (x=115) and runs from TOP 30 to 30+HGT (CH=264-4=260, HGT=260-66=194 → 224): two label lines above and below', (s1.match(/<line x1="115"[^>]*>/)||[])[0]);
  ok(/HOD 11:54<\/text>/.test(s1) && /LOD 12:47<\/text>/.test(s1) && />3h24<\/text>/.test(s1) && />4h17<\/text>/.test(s1) && /MD 0m/.test(s1), '1f the HOD/LOD clocks, the TOOK FROM THE OPEN under each (v15.94: "the difference between the open and 10:27 is not 4h30" — HOD 3h24, LOD 4h17, not the leg after), MD (v15.80: MUD reads MU / MD)');
  ok(/<text x="95" y="[\d.]+" class="g3cs" text-anchor="end" fill="#f0616d"><title>[^<]*<\/title>MD 0m<\/text>/.test(s1) && !/fill="#e3b341"><title>PDH/.test(s1), '1g (v15.80) MD sits LEFT of the bar at cx-20=95 beside the open tick, red; the reversal names are gone');
  ok(!/x="228"/.test(s1), '1h …and nothing is drawn in the right column while there are no sweeps');
  // the sweep labels — the KEY levels price swept, at their own price (test_v1580 §2 pins the axis geometry; here: the set)
  SW=[ {level:'POC',side:'LOD',status:'accepted',atBar:0,px:7702.5,at:'08:30'},                        // opened beyond — not drawn
       {level:'VAL',side:'LOD',status:'accepted',atBar:41,px:7699.25,at:'09:11'},                     // key · broke
       {level:'EML',side:'LOD',status:'reclaimed',atBar:55,px:7695.5,at:'09:25',speed:3},              // not a level
       {level:'KING',side:'LOD',status:'tested',atBar:60,px:7694,at:'09:30'},                          // not a key level
       {level:'VWAP',side:'LOD',status:'accepted',atBar:61,px:7693,at:'09:31'},                        // tier 3
       {level:'ONH',side:'HOD',status:'reclaimed',atBar:120,px:7732,at:'10:30',speed:2} ];             // key · reclaimed
  const s2=dayCandleSvg('SPY', D, P, { w:230, h:264 });
  const right=[...s2.matchAll(/<text x="140" y="([\d.]+)" class="g3cx sw" text-anchor="start" fill="(#[0-9a-f]{6})"><title>([^<]*)<\/title>([^<]*)<\/text>/g)].map(m=>({y:+m[1],col:m[2],tip:m[3],name:m[4]}));
  ok(right.length===2 && right.map(r=>r.name).join(' | ')==='ONH 10:30 | VAL 9:11', '1i the swept KEY levels only, top-down by price, with the minute: ONH · VAL (POC opened-beyond, EML, KING, VWAP out)', right.map(r=>r.name));
  const y=px=>30+(7735-px)/47*194;
  ok(Math.abs(right[0].y-(y(7732)+3))<0.06 && Math.abs(right[1].y-(y(7699.25)+3))<0.06, '1j each label sits on its own price line (the axis), not at a wick tip', right.map(r=>r.y));
  ok(right[0].col==='#2ec27e' && right[1].col==='#f0616d', '1k …green reclaimed · red broke', right.map(r=>r.name+':'+r.col));
  ok(/ONH 7732 — swept 10:30 · reclaimed in 2 bars \(making HOD\)/.test(right[0].tip) && /VAL 7699\.25 — swept 09:11 · broke \(making LOD\)/.test(right[1].tip), '1l each label\'s hover carries the price, the time, the status words and the side', right.map(r=>r.tip));
  ok(/class="g3cx" text-anchor="end" fill="#7cc7ff">SWEPT \/ TARGET ▸<\/text>/.test(s2), '1m a SWEPT / TARGET ▸ caption names the axis once any sweep is drawn (v15.94: the axis now also carries the target / next draw)');
  const s3=dayCandleSvg('SPY', D, P);
  ok((s3.match(/class="g3cx sw"/g)||[]).length===2 && /<title>[^<]*<\/title>ONH<\/text>/.test(s3) && !/ONH 10:30/.test(s3), '1n (v15.80) the small candle wears the same axis, names only');
  // (v15.94) EIGHTEEN DISTINCT key levels — the same label swept repeatedly now collapses to one row
  // (seen[] dedupe: a level has ONE canonical price), so the crowd test feeds his eighteen DISTINCT levels.
  const KL18=['PDH','VAH','ONH','PWH','WPOC','AHI','LHI','PFH','CW0','PDL','VAL','ONL','PWL','POC','ALO','LLO','PFL','PW0'];
  SW=KL18.map((lv,i)=> i<9
    ? { level:lv, side:'HOD', status:'accepted', atBar:20+i, px:7734-i*0.1, at:'10:'+two(i) }
    : { level:lv, side:'LOD', status:'accepted', atBar:1+(i-9), px:7691-(i-9)*0.1, at:'09:0'+(i-9) } );
  const s4=dayCandleSvg('SPY', D, P, { w:230, h:264 });
  const ys=[...s4.matchAll(/<text x="140" y="([\d.]+)" class="g3cx sw"/g)].map(m=>+m[1]);
  ok(ys.length===18 && ys.every((v,i)=>i===0 || v-ys[i-1]>=9.49) && ys[0]>=36 && ys[17]<=227, '1o (v15.94) no seven-a-side cap: all eighteen DISTINCT key levels draw, a line apart, between the HOD tip (30+3) and the LOD tip (224-1)', [ys.length, ys[0], ys[17]]);
  SW=[];
}

// ---------- 2. candleFit: measured, never assumed ----------
{
  global.CDL_MIN_W=val('CDL_MIN_W'); global.CDL_MAX_W=val('CDL_MAX_W');
  ok(CDL_MIN_W===110 && CDL_MAX_W===260, '2a the floor is 110px of free width, the cap 260px');
  eval(ex('candleFit'));
  // a fake DOM: the grid line's container, the grid, the slot
  function dom(o){
    const slot={ className:'g3ladcdl', innerHTML:'', title:'', attrs:{'data-sym':'SPY'}, getAttribute(k){ return this.attrs[k]; }, getBoundingClientRect(){ return { top:(o.wrap?600:265) }; } };
    const grid={ offsetWidth:o.gridW||734, offsetHeight:o.gridH||281, getBoundingClientRect(){ return { top:265 }; }, parentElement:{ clientWidth:o.f2W, columnGap:'8px' } };
    global.getComputedStyle=(el)=>({ columnGap:'8px', gap:'8px' });
    global.elBody={ querySelector(sel){ return sel==='.g3ladcdl'?slot:(sel==='.g3grid'?grid:null); } };
    return { slot, grid };
  }
  // ⚠ dayCandleSvg was eval'd into THIS scope in section 1 and shadows any global — reassign the binding itself
  let REQ=null; dayCandleSvg=(sym,D2,P2,o)=>{ REQ=o; return '<svg class="g3cdl tall"></svg>'; };
  global.hodLod=()=>({ ok:true }); global.hlPT=()=>null; global.hlDayShown=()=>'2026-09-04'; global.replayOn=()=>true;
  let d=dom({ f2W:976 }); candleFit();
  ok(d.slot.className==='g3ladcdl on' && REQ && REQ.w===230 && REQ.h===264, '2b 976 wide, the grid 734 + 8 gap → 234 free: the candle is built 230 wide (the slot\'s borders) and grid-height minus the header (281-17)', [d.slot.className, REQ]);
  ok(/^<div class="g3cdlhd">DAY · <b>FRI 4 SEP<\/b> · as it closed<\/div><svg/.test(d.slot.innerHTML), '2c the header names the shown day; in a replay or the closed state it reads "as it closed"', d.slot.innerHTML.slice(0,80));
  ok(/THE DAY.S CANDLE, measured on the ES 1-minute bars/.test(d.slot.title) && /RIGHT of the bar is the PRICE AXIS/.test(d.slot.title) && /the minute the open was reclaimed and BOP/.test(d.slot.title), '2d (v15.80) the hover explains every mark: the source, the axis, the reclaim line');
  global.replayOn=()=>false; d=dom({ f2W:976 }); candleFit();
  ok(/· developing<\/div>/.test(d.slot.innerHTML), '2e live, the header reads "developing"');
  REQ=null; d=dom({ f2W:840 }); candleFit();
  ok(d.slot.className==='g3ladcdl' && d.slot.innerHTML==='' && REQ===null, '2f 840 wide → 98 free, under the floor: the slot stays hidden and empty, the candle is not even built');
  REQ=null; d=dom({ f2W:1400 }); candleFit();
  ok(REQ && REQ.w===260, '2g on a very wide panel the candle is capped at 260 (the slot stretches, the candle is centred)');
  d=dom({ f2W:976, wrap:true }); candleFit();
  ok(d.slot.className==='g3ladcdl' && d.slot.innerHTML==='', '2h if the slot still wraps under the grid after filling, it is hidden again — a candle below the ladder is not what he asked for');
  global.hodLod=()=>({ ok:false, why:'no candles' }); d=dom({ f2W:976 }); candleFit();
  ok(d.slot.className==='g3ladcdl', '2i no session read → no candle, no throw');
  global.hodLod=()=>({ ok:true }); dayCandleSvg=()=>''; d=dom({ f2W:976 }); candleFit();
  ok(d.slot.className==='g3ladcdl', '2j an empty svg → hidden');
  global.elBody=null; candleFit();
  ok(true, '2k no body → returns');
  global.elBody={ querySelector(){ return null; } }; candleFit(); ok(true, '2l no slot → returns');
}

// ---------- 3. the face ----------
{
  ok(/h\+='<div class="g3ladcdl" data-sym="'\+g3esc\(sym\)\+'"><\/div>';/.test(src), '3a the slot is emitted right after the grid, a flex sibling inside .g3f2');
  const i=src.indexOf("kingStripHtml(sym, RAILROLLS)+ladderGridHtml("), j=src.indexOf("h+='<div class=\"g3ladcdl\"");
  ok(i>0 && j>i && j-i<400, '3b …immediately after ladderGridHtml');
  ok(/try\{ ladderFit\(\); \}catch\(eLF\)\{\}\n[^\n]*\n  try\{ candleFit\(\); \}catch\(eCF\)\{\}/.test(src), '3c candleFit runs after ladderFit in render(), after layout');
  ok(/#gpts-body \.g3ladcdl\{display:none;flex:1 1 auto;align-self:stretch/.test(src) && /#gpts-body \.g3ladcdl\.on\{display:flex\}/.test(src), '3d hidden by default, shown only with .on, stretched to the grid line');
  ok(/#gpts-body \.g3cdlhd\{/.test(src) && /#gpts-body \.g3cdl\.tall \.g3cl\{font-size:10px\}/.test(src) && /\.g3cdl\.tall \.g3cx\.sw\{font-weight:900\}/.test(src), '3e the header and the tall frame\'s label sizes are in the stylesheet');
  ok(fs.existsSync('mockups/mockup-day-candle.png') && fs.existsSync('design/render-v1578-face.png') && fs.existsSync('design/render-v1578-narrow.png'), '3f the mockup he approved and the renders (wide with sweeps; narrow without the column) are in the repo');
}

// ---------- 4. the record ----------
{
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=P.roadmap.filter(r=>r.status==='next');
  ok(P.roadmap.some(r=>r.v==='15.78' && /DAY CANDLE/.test(r.title) && (r.status==='shipped' || nx.some(x=>x.v==='15.78'))) && P.roadmap.some(r=>r.v==='15.77' && r.status==='shipped'),
     '4a the plan: v15.77 shipped, v15.78 this build or shipped', nx.map(x=>x.v));
  ok(P.roadmap.some(r=>/SEASONALITY TRACKED/.test(r.title) && /^(15\.(79|[89]\d)|16\.\d\d)$/.test(r.v)), '4b the seasonality tracking sits after this build (v15.79 or later), still mockup-first');
  const seedJs=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(P), '4c PLAN_SEED equals the file');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r15=R.rows.find(r=>r.id==='R-15');
  ok(r15 && r15.status==='implemented' && r15.version==='15.78' && r15.by==='operator' && /candle/i.test(r15.text) && /swept|sweep/i.test(r15.text), '4d R-15 on Rec, by operator, implemented in v15.78', r15&&[r15.status,r15.version]);
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '4e REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.78/.test(cl) && cl.indexOf('## v15.78')<cl.indexOf('## v15.77'), '4f the CHANGELOG has the v15.78 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.78/.test(ls.slice(logAt>=0?logAt:0)), '4g the lesson log carries the v15.78 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(7[8-9]|[89]\d)/.test(rn.slice(0,600)) && /candle/i.test(rn), '4h the resume note is at v15.78 or later and names the candle');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/candle/i.test(cfg.theWhatAndTheHow.dayCandle||'') && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1578.js')>=0, '4i .gex-config.json names the candle and this test');
  const inv=fs.readFileSync('design/DASHBOARD-INVENTORY.md','utf8'); ok(/## 0l · v15\.78/.test(inv), '4j INVENTORY §0l');
}

console.log('test_v1578: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
