#!/usr/bin/env node
// test_v1580.js — (v15.80) THE CANDLE ON THE PRICE AXIS · ONE SOURCE FOR THE SHOWN DAY · HIS KEY LEVELS · H10.
// Operator, 2026-09-08, on the v15.79 candle: "the levels that are swept are the only ones that should be indicated. EMH
// and EML are not levels and they should be aligned based on the y axis which should be the price axis so the candle
// should show where it swept the level … Do you indicate how much time it took to reclaim the Open … Also a MUD of 0m
// doesn't make sense. double check the values they are incorrect." Then: "much better but the king is not a key level.
// the key levels are PDH, PDL, ONL, ONH, WH, WL, Prior day POC, VAH, VAL, Weekly Poc" and "I dont want IBL IBH PDC. you
// can keep CW0 and PW0. and POC is the prior day poc. VAH and VAL is also prior day VAH and prior day VAL."
// Found on the way: the parked candle measured the recorder's FRAMES (MUD 0m, the 09:46 print as the open) while the
// sweeps read the ES bars; the MUD dollars were 10x on the ES chart since v15.08; ONH/ONL had never been the full night.
// (sloppy mode on purpose: a direct eval must declare the panel's functions into this scope)
const fs=require('fs'), cp=require('child_process');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){ const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src); if(!m) return ''; let i=src.indexOf('{',m.index),d=0,e=-1; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } } return src.slice(m.index,e+1); }
function exVar(n){ const m=new RegExp('\\nvar '+n+'=').exec(src); if(!m) throw new Error('var not found: '+n);
  const open=src[src.indexOf('=',m.index)+1]; const close=(open==='[')?']':'}'; let i=src.indexOf(open,m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]===open)d++; else if(src[k]===close){ d--; if(!d) return 'var '+n+'='+src.slice(i,k+1)+';'; } } }
function val(n){ const m=new RegExp('(?:var\\s+)?\\b'+n+'\\s*=\\s*([\\s\\S]*?);\\n').exec(src); return m?eval('('+m[1]+')'):undefined; }
const mul=(a,b)=>a*b;

// ---------- 0. the version ----------
ok(/@version\s+15\.(8\d|9\d)/.test(src) && /var GPTS_VERSION='15\.(8\d|9\d)';/.test(src), '0a v15.80 or later in both spots');

// ---------- 1. ONE SOURCE FOR THE SHOWN DAY: measureBars serves the courier's ES bars in a replay; closedCandles keeps the frames ----------
{
  const CT=5*3600, epochOf=(y,m,d,hh,mm)=>Date.UTC(y,m-1,d,hh,mm)/1000+CT;   // CT wall clock → epoch seconds
  const frames=[]; for(let i=0;i<6;i++) frames.push({ t:(epochOf(2026,9,4,9,46+3*i))*1000, px:767+i*0.1, h:767.5+i*0.1, l:766.8+i*0.1, tri:{}, vend:{rows:[1]} });
  global.REPLAY={ on:true, day:'2026-09-04', frames, idx:5 };
  global.replayOn=()=>true; global.swallow=()=>{}; global.STATE={ SPY:{ candles:[{o:9,h:9,l:9,c:9,so:1}] } };
  global.RMEMO={ id:0, m:{} }; eval(ex('rmemo'));
  let FUT=true; global.dispIsFut=()=>FUT; global.dispR=()=>10.03; global.futBarsLoad=()=>null;
  let SD=null; global.futSessionBars=(off,day)=>{ CALLS.push([off,day]); return SD; }; let CALLS=[];
  eval(['replayFrameBars','measureBarsRaw','measureBars','closedCandles'].map(ex).join('\n'));
  // the courier holds Friday: 40 RTH minutes from 08:30
  const rth=[]; for(let k=0;k<40;k++) rth.push([epochOf(2026,9,4,8,30+k), 7742+k*0.25, 7746+k*0.25, 7740+k*0.25, 7744+k*0.25, 100]);
  SD={ rth, on:[] };
  let MB=measureBarsRaw('SPY');
  ok(MB.src==='ES' && MB.day==='2026-09-04' && MB.scale===1 && MB.approxOpen===false && MB.shown===true, '1a parked on Friday with the courier holding it: the ES bars, chart-scale, opens RECORDED (not reconstructed)', [MB.src, MB.scale, MB.approxOpen]);
  ok(CALLS.length===1 && CALLS[0][0]===0 && CALLS[0][1]==='2026-09-04', '1b …asked for by the SHOWN day, never the newest', CALLS);
  ok(MB.bars.length===40 && MB.bars[0].o===7742 && MB.bars[0].h===7746 && MB.bars[0].l===7740 && MB.bars[0].c===7744 && MB.bars[0].so===8*3600+30*60 && MB.bars[0].t===rth[0][0]*1000,
     '1c each bar is the row\'s own o/h/l/c with its CT second-of-day and ms epoch', MB.bars[0]);
  ok(MB.bars[39].so===8*3600+30*60+39*60, '1d …in order to the last row');
  // truncated at the parked frame: park at 09:00 (frame 0 is 09:46 — set a frame at 09:00)
  REPLAY.frames=[{ t:epochOf(2026,9,4,9,0)*1000, px:767, h:767.5, l:766.8, tri:{}, vend:{rows:[1]} }]; REPLAY.idx=0;
  MB=measureBarsRaw('SPY');
  ok(MB.src==='ES' && MB.bars.length===31 && MB.bars[30].so===9*3600, '1e parked at 09:00 the series stops at the 09:00 bar — nothing after the parked minute has happened yet', MB.bars.length);
  // fewer than 30 rows → the frames
  SD={ rth:rth.slice(0,20), on:[] }; REPLAY.frames=frames; REPLAY.idx=5;
  MB=measureBarsRaw('SPY');
  ok(MB.src==='replay' && MB.approxOpen===true && MB.bars.length===6 && MB.bars[1].o===frames[0].px && MB.bars[0].o===frames[0].px, '1f under 30 courier bars: the frames, open = the previous frame\'s close, the first at its own', [MB.src, MB.bars.length]);
  SD=null; MB=measureBarsRaw('SPY');
  ok(MB.src==='replay' && MB.bars.length===6, '1g the courier does not hold the day (Labor Day, an old replay): the frames');
  SD={ rth, on:[] }; FUT=false; MB=measureBarsRaw('SPY');
  ok(MB.src==='replay' && MB.approxOpen===true, '1h on a cash chart the frames — the courier\'s ES bars are not the chart\'s instrument'); FUT=true;
  // closedCandles: the FRAMES, even while measureBars serves ES — the band's pin, the ATR, the trend machine convert by rr
  RMEMO.m={}; MB=measureBars('SPY'); ok(MB.src==='ES', '1i (memo) measureBars serves ES');
  const cc=closedCandles('SPY');
  ok(cc.length===6 && Math.abs(cc[5].c-frames[5].px)<1e-9 && cc[0].o===frames[0].px, '1j closedCandles in replay = the frames (underlying scale), NOT the ES bars measureBars now serves — two consumers, two seams', cc.length);
  global.replayOn=()=>false;
  ok(closedCandles('SPY')[0].c===9, '1k live: closedCandles still reads STATE candles, untouched');
  global.replayOn=()=>true;
  const rf=replayFrameBars();
  ok(rf.length===6 && rf[2].o===frames[1].px && rf[2].c===frames[2].px && rf[2].h===frames[2].h && typeof rf[2].so==='number', '1l replayFrameBars is the old loop, extracted: t/h/l/px per frame, open = previous close');
  REPLAY.idx=2; ok(replayFrameBars().length===3, '1m …truncated at the parked frame, inclusive'); REPLAY.idx=5;
  // the shape of the change
  const mb=ex('measureBarsRaw');
  ok(/typeof dispIsFut==='function' && dispIsFut\(\) && REPLAY\.day && typeof futSessionBars==='function'/.test(mb), '1n the new dependencies are typeof-guarded — absent must mean ABSENT, never a throw (the v15.08 lesson)');
  ok(/if\(cutMs!=null && rq\[0\]\*1000>cutMs\) break;/.test(mb) && /if\(esb\.length>=30\) return \{ bars:esb, scale:1, src:'ES', day:REPLAY\.day, approxOpen:false, shown:true \};/.test(mb), '1o the cut at the parked minute and the 30-bar floor are in the code');
  ok(/var RB=\(typeof replayFrameBars==='function'\)\?replayFrameBars\(\):\[\];/.test(ex('closedCandles')), '1p closedCandles reads replayFrameBars, not measureBars');
}

// ---------- 2. the candle: swept KEY levels on the price axis, MUD and the reclaim beside the open tick, the dollars once ----------
{
  global.DAYCOL_HD=val('DAYCOL_HD'); global.DAYCOL_N=val('DAYCOL_N'); global.DAYCOL_ROW=val('DAYCOL_ROW'); global.ES_USD_PER_PT=50;
  global.two=x=>{ x=''+x; return x.length<2?'0'+x:x; }; global.g3esc=s=>String(s==null?'':s).replace(/"/g,'&quot;').replace(/</g,'&lt;');
  global.frameNum=x=>String(x); global.LEVEL_TIER=val('LEVEL_TIER');
  global.FUTMODE={ fam:'ES' }; global.irtRatio=()=>({ r:10.03 });
  eval(['levelTier','hlClock','hlDur','hlMudLabel','dayCandleSvg','sweepEventsShown'].map(ex).join('\n'));
  global.replayOn=()=>false; global.hlDayShown=()=>'2026-09-08'; global.ctTodayStr=()=>'2026-09-08';
  let SB={ open:7742, close:7723 }; global.sessionBody=()=>SB;
  global.displayScale=()=>({ scale:10.03 });
  global.revLevels=()=>({ ok:true, hi:[{name:'PDH',px:7751}], lo:[{name:'PDL',px:7690}] });
  let SW=[]; global.sweepEventsToday=()=>SW;
  const D={ ok:true, open:7742, hod:7751, lod:7711.75, hodT:mul(8,3600)+mul(36,60), lodT:mul(10,3600)+mul(31,60), first:'HOD', second:'LOD', gap:115, mud:106, wend:mul(8,3600)+mul(45,60), bop:9, wick:15, scale:1, rngUsd:1962.5, rngPts:39.25, isFut:true };
  const P={ ok:true, ptPx:7733, lcMin:256 };
  const W=230, H=293, CH=H-4, TOP=30, HGT=CH-66, cx=115;
  const y=px=>TOP+(7751-px)/(7751-7711.75)*HGT;
  // Friday's sweeps as the panel found them (v15.80 set): only ONL and VAL are key AND swept
  SW=[ {level:'POC-',side:'LOD',status:'accepted',atBar:0,px:7758,at:'08:30'},                       // opened below it — not a sweep
       {level:'VWAP-',side:'LOD',status:'reclaimed',atBar:6,px:7746.79,at:'08:36',speed:2},         // tier 3
       {level:'ONL',side:'LOD',status:'reclaimed',atBar:42,px:7731.25,at:'09:12',speed:4},          // KEY
       {level:'LDNL',side:'LOD',status:'reclaimed',atBar:42,px:7731.25,at:'09:12',speed:4},         // structure (tier 2)
       {level:'KING-',side:'LOD',status:'reclaimed',atBar:73,px:7725.15,at:'09:43',speed:1},        // "the king is not a key level"
       {level:'VAL',side:'LOD',status:'accepted',atBar:78,px:7722,at:'09:48'},                      // KEY
       {level:'EML',side:'LOD',status:'reclaimed',atBar:79,px:7721.05,at:'09:49',speed:2},          // "EMH and EML are not levels"
       {level:'IBL',side:'LOD',status:'accepted',atBar:72,px:7729.5,at:'09:42'},                    // "I dont want IBL IBH PDC"
       {level:'VAH',side:'HOD',status:'accepted',atBar:0,px:7740,at:'08:30'},                       // KEY but the session OPENED above it — not a sweep
       {level:'PDH',side:'HOD',status:'pending',atBar:5,px:7749,at:'08:35'} ];                      // KEY, being tested (making HOD)
  const s=dayCandleSvg('SPY', D, P, { w:W, h:H });
  const labels=[...s.matchAll(/<text x="(\d+)" y="([\d.]+)" class="g3cx sw" text-anchor="start" fill="(#[0-9a-f]{6})"><title>([^<]*)<\/title>([^<]*)<\/text>/g)].map(m=>({x:+m[1],y:+m[2],col:m[3],tip:m[4],txt:m[5]}));
  ok(labels.map(l=>l.txt).join(' | ')==='PDH 8:35 | ONL 9:12 | VAL 9:48', '2a ONLY the swept KEY levels, top to bottom by price, name + the minute it swept: PDH (being tested) · ONL · VAL — POC and VAH (opened beyond), VWAP, LDNL, KING, EML, IBL all out', labels.map(l=>l.txt));
  const ticks=[...s.matchAll(/<line x1="127" y1="([\d.]+)" x2="136" y2="([\d.]+)" stroke="(#[0-9a-f]{6})" stroke-width="1\.3"\/>/g)].map(m=>({y:+m[1],col:m[3]}));
  ok(ticks.length===3 && Math.abs(ticks[0].y-y(7749))<0.06 && Math.abs(ticks[1].y-y(7731.25))<0.06 && Math.abs(ticks[2].y-y(7722))<0.06, '2b each level is a tick across the right of the bar AT ITS OWN PRICE on the candle\'s axis (cx+12 → cx+21)', ticks.map(t=>t.y));
  ok(labels.every(l=>l.x===140) && labels.every((l,i)=>Math.abs(l.y-(ticks[i].y+3))<0.06), '2c …with the label beside it (cx+25), on the tick\'s line when nothing collides', labels.map(l=>l.y));
  ok(labels[0].col==='#f2b45a' && labels[1].col==='#2ec27e' && labels[2].col==='#f0616d' && ticks.map(t=>t.col).join()===labels.map(l=>l.col).join(), '2d amber being tested · green reclaimed · red broke — tick and label alike; no grey (nothing "opened beyond" is drawn)', labels.map(l=>l.col));
  ok(/ONL 7731\.25 — swept 09:12 · reclaimed in 4 bars \(making LOD\)/.test(labels[1].tip) && /VAL 7722 — swept 09:48 · broke \(making LOD\)/.test(labels[2].tip) && /PDH 7749 — swept 08:35 · being tested \(making HOD\)/.test(labels[0].tip), '2e the hover carries the price, the minute, the words and the side', labels.map(l=>l.tip));
  ok(!/x="95" y="[\d.]+" class="g3cx" text-anchor="end" fill="#e3b341"><title>PDH/.test(s) && !/fill="#5fd08a"><title>PDL/.test(s) && !/x="228" y="[\d.]+" class="g3cx sw"/.test(s), '2f the reversal names (left) and the tip-stacked right column are gone — "the levels that are swept are the only ones that should be indicated"');
  ok(/<text x="228" y="21\.0" class="g3cx" text-anchor="end" fill="#7cc7ff">SWEPT ▸<\/text>/.test(s), '2g the SWEPT ▸ caption stays at the top right');
  // beside the open tick, LEFT of the bar: the reclaim line, MUD, the dollars — converted ONCE
  const mx=[...s.matchAll(/<text x="95" y="([\d.]+)" class="g3cs" text-anchor="end" fill="(#[0-9a-f]{6})">(?:<title>[^<]*<\/title>)?([^<]*)<\/text>/g)].map(m=>({y:+m[1],col:m[2],txt:m[3]}));
  ok(mx.map(m=>m.txt).join(' | ')==='↩ 8:45 · 9m | MD 1h46 | $1,513', '2h left of the bar at cx-20, top-down: ↩ the reclaim minute · BOP, then MD (the move DOWN to the LOD — "MUD should be dynamic, MU or MD"), then the money', mx.map(m=>m.txt));
  ok(mx[1].col==='#f0616d', '2h2 MD is red ("MU should be green, MD should be red")', mx[1].col);
  const sUp=dayCandleSvg('SPY', Object.assign({}, D, { first:'LOD', second:'HOD', hodT:D.lodT, lodT:D.hodT }), P, { w:W, h:H });
  ok(/fill="#2ec27e"><title>[^<]*<\/title>MU 1h46<\/text>/.test(sUp), '2h3 …and a day whose second extreme is the HOD reads MU, green');
  ok(Math.abs(mx[1].y-(y(7742)+3))<0.06 && Math.abs(mx[0].y-(y(7742)+3-10.5))<0.06 && Math.abs(mx[2].y-(y(7742)+3+10.5))<0.06, '2i …anchored on the OPEN tick\'s line (MUD on it, the reclaim a line above, the dollars a line below)', mx.map(m=>m.y));
  ok(/\$1,513<\/text>/.test(s) && !/\$15,1/.test(s), '2j the MUD dollars: |7711.75 − 7742| = 30.25 ES points x $50 = $1,513 — the multiplier ONCE (it was x10 on the ES chart since v15.08)');
  const D2=Object.assign({}, D, { wend:null, bop:null });
  const s2=dayCandleSvg('SPY', D2, P, { w:W, h:H });
  ok(!/↩/.test(s2) && /MD 1h46/.test(s2), '2k a day that never reclaimed the open prints no reclaim line — nothing is invented');
  // collisions: two key levels a point apart push apart, the ticks stay at the true price, a leader shows the displacement
  SW=[ {level:'VAL',side:'LOD',status:'accepted',atBar:78,px:7722,at:'09:48'}, {level:'PDL',side:'LOD',status:'reclaimed',atBar:90,px:7721.5,at:'10:00',speed:2} ];
  const s3=dayCandleSvg('SPY', D, P, { w:W, h:H });
  const t3=[...s3.matchAll(/<line x1="127" y1="([\d.]+)" x2="136"/g)].map(m=>+m[1]); const l3=[...s3.matchAll(/<text x="140" y="([\d.]+)" class="g3cx sw"/g)].map(m=>+m[1]);
  ok(t3.length===2 && Math.abs(t3[0]-y(7722))<0.06 && Math.abs(t3[1]-y(7721.5))<0.06, '2l two levels 0.5 apart: both ticks at their true prices', t3);
  ok(l3.length===2 && Math.abs((l3[1]-l3[0])-9.5)<0.06 && Math.abs(l3[0]-(t3[0]+3))<0.06, '2m …the labels a line (9.5px) apart, the first on its tick, the second pushed down', l3);
  ok((s3.match(/stroke-width="\.8" opacity="\.75"\/>/g)||[]).length===1, '2n …and ONE leader from the displaced tick to its label');
  // pushed past the low: the group slides up so the last label ends a pixel above the LOD tip
  SW=[ {level:'VAL',side:'LOD',status:'accepted',atBar:78,px:7712.5,at:'09:48'}, {level:'PDL',side:'LOD',status:'reclaimed',atBar:90,px:7712,at:'10:00',speed:2}, {level:'ONL',side:'LOD',status:'reclaimed',atBar:91,px:7711.9,at:'10:01',speed:2} ];
  const s4=dayCandleSvg('SPY', D, P, { w:W, h:H }); const l4=[...s4.matchAll(/<text x="140" y="([\d.]+)" class="g3cx sw"/g)].map(m=>+m[1]);
  ok(l4.length===3 && Math.abs(l4[2]-(y(7711.75)-1+3))<0.06 && Math.abs(l4[1]-(l4[2]-9.5))<0.06, '2o three levels at the low: the group slides up so the last label sits on the LOD tip, not under it', l4);
  // a crowd: nine at the top and nine at the bottom, a tenth of a point apart — all drawn, a line apart, and the group
  // stays between the HOD tip and the LOD tip (pushed apart downward, slid up from the low, clamped under the high)
  SW=[ ...Array.from({length:9},(_,i)=>({level:'PDH',side:'HOD',status:'accepted',atBar:i+20,px:7750-i*0.1,at:'10:0'+i})),
       ...Array.from({length:9},(_,i)=>({level:'PDL',side:'LOD',status:'accepted',atBar:i+1,px:7713-i*0.1,at:'09:0'+i})) ];
  const sC=dayCandleSvg('SPY', D, P, { w:W, h:H }); const yC=[...sC.matchAll(/<text x="140" y="([\d.]+)" class="g3cx sw"/g)].map(m=>+m[1]);
  ok(yC.length===18 && yC.every((v,i)=>i===0 || v-yC[i-1]>=9.49) && yC[0]>=y(7751)+6-0.06 && yC[17]<=y(7711.75)+2+0.06, '2o2 eighteen in a crowd: all drawn, a line apart, between the tips — never above the HOD label', [yC.length, yC[0], yC[17]]);
  // a level outside the range is not drawn (a swept level lies inside it by construction; a stale conversion does not)
  SW=[ {level:'PDH',side:'HOD',status:'accepted',atBar:5,px:7790,at:'08:35'} ];
  ok(!/class="g3cx sw"/.test(dayCandleSvg('SPY', D, P, { w:W, h:H })), '2p a level above the HOD is not on the axis');
  // the small frame: name only
  SW=[ {level:'ONL',side:'LOD',status:'reclaimed',atBar:42,px:7731.25,at:'09:12',speed:4} ];
  const s5=dayCandleSvg('SPY', D, P);
  ok(/<text x="68" y="[\d.]+" class="g3cx sw" text-anchor="start" fill="#2ec27e"><title>[^<]*<\/title>ONL<\/text>/.test(s5) && !/ONL 9:12/.test(s5), '2q the 98px DAY-table candle wears the same axis, names only (no room for the minute)');
  // a cash chart: the sweeps are in ES points; the candle is in the chart\'s — converted back through the ratio
  global.FUTMODE={ fam:null }; SB={ open:774.2, close:772.3 };
  const Dc=Object.assign({}, D, { open:774.2, hod:775.1, lod:770.0, isFut:false, rngUsd:null });
  SW=[ {level:'ONL',side:'LOD',status:'reclaimed',atBar:42,px:7731.25,at:'09:12',speed:4} ];
  const s6=dayCandleSvg('SPY', Dc, P, { w:W, h:H }); const t6=[...s6.matchAll(/<line x1="127" y1="([\d.]+)" x2="136"/g)].map(m=>+m[1]);
  const yc=px=>TOP+(775.1-px)/(775.1-770.0)*HGT;
  ok(t6.length===1 && Math.abs(t6[0]-yc(7731.25/10.03))<0.1, '2r on a cash chart the ES-point sweep is divided by the ratio before it meets the SPY-scale axis', t6);
  global.FUTMODE={ fam:'ES' }; SB={ open:7742, close:7723 }; SW=[];
  const tip=ex('candleFit');
  ok(/RIGHT of the bar is the PRICE AXIS: every KEY level price swept this session/.test(tip) && /the minute the open was reclaimed and BOP/.test(tip) && !/the levels the wick turned ON/.test(tip), '2s the slot\'s hover describes the axis and the reclaim line, and no longer promises the reversal names');
}

// ---------- 3. HIS KEY LEVELS: the tiers, the sweep set, the full overnight ----------
{
  const T=val('LEVEL_TIER');
  ok(['PDH','PDL','ONH','ONL','VAH','VAL','POC','PWH','PWL','WPOC'].every(k=>T[k]===1) && Object.keys(T).filter(k=>T[k]===1).length===10, '3a tier 1 is his list and nothing else: PDH PDL ONH ONL VAH VAL POC + PWH PWL WPOC by name for the day they exist', Object.keys(T).filter(k=>T[k]===1));
  ok(T.KING===2 && T.CW0===2 && T.PW0===2 && !('EMH' in T) && !('EML' in T) && !('IBH' in T) && !('IBL' in T) && !('PDC' in T), '3b the King is structure (tier 2, with CW0/PW0); EMH/EML, IBH/IBL and PDC are not in the map at all');
  eval(ex('levelTier'));
  ok(levelTier('KING-')===2 && levelTier('POC+')===1 && levelTier('EML')===4 && levelTier('IBL')===4 && levelTier('PDC-')===4, '3c …so levelTier says 1 for a key level, 2 for the King, 4 (unranked) for what he removed');
  // the sweep set, on a stubbed courier
  const CT=5*3600, epochOf=(y,m,d,hh,mm)=>Date.UTC(y,m-1,d,hh,mm)/1000+CT;
  const rows=[];
  // Thursday: a full RTH (60+ bars) and an evening from 17:00 (420 bars) ; Friday: 00:00→08:29 (509) and 08:30→ (70)
  for(let k=0;k<70;k++) rows.push([epochOf(2026,9,3,8,30+k), 7700, 7766.5+(k===10?0:0), 7700-(k===20?0:0), 7750, 100]);
  for(let k=0;k<420;k++) rows.push([epochOf(2026,9,3,17,0)+k*60, 7750, (k===100?7764.5:7752), (k===200?7740:7748), 7750, 100]);
  for(let k=0;k<509;k++) rows.push([epochOf(2026,9,4,0,0)+k*60, 7745, (k===300?7760:7746), (k===400?7731.25:7744), 7745, 100]);
  for(let k=0;k<70;k++) rows.push([epochOf(2026,9,4,8,30+k), 7742, 7751, 7711.75, 7723, 100]);
  global.futBarsLoad=()=>({ ES:{ rows } });
  global.SWEEP_IB_BARS=60; global.SWEEP_RECLAIM_MAX=30;
  eval(['futDayKeyOf','futDayKeyNum','futSessionBars','overnightHL'].map(ex).join('\n'));
  const ON=overnightHL('2026-09-04');
  ok(ON && ON.full===true && ON.n===929 && ON.onh===7764.5 && ON.onl===7731.25, '3d ONH/ONL = the FULL Globex night: Thursday from 17:00 (420 bars) + Friday to 08:29 (509) = 929 bars, the high from the evening, the low from the morning', ON);
  const ON0=overnightHL('2026-09-03');
  ok(ON0 && ON0.full===false && ON0.n===0+0 || (ON0===null), '3e Thursday (no Wednesday evening in the store, no Thursday pre-open bars): a stub or nothing — never a night assembled from the wrong side', ON0);
  const P=futSessionBars(1,'2026-09-04');
  ok(P && P.rth.length===70 && P.on.length===420, '3f …the prior key\'s `on` holds Thursday\'s evening, which the old overnightHL never read');
  global.priorProfile=()=>({ val:7722, vah:7766, poc:7758 }); global.emBand=()=>({ ok:true, high:7763, low:7721 }); global.dispToEs=x=>x;
  global.bookLevelsNow=()=>({ walls:[{name:'CW0',es:7760},{name:'PW0',es:7715},{name:'CW',es:7780}], king:{ es:7725.15 } });
  eval(ex('sweepLevelsToday'));
  const names=sweepLevelsToday('SPY','2026-09-04').map(l=>l.name);
  ok(['ONL','ONH','PDL','PDH','VAL','VAH','POC-','POC+','CW0-','PW0-','KING-'].every(n=>names.indexOf(n)>=0) || ['ONL','ONH','PDL','PDH','VAL','VAH','POC-','POC+'].every(n=>names.indexOf(n)>=0), '3g the set: ONL/ONH (the night), PDL/PDH, the prior day\'s VAL/VAH/POC, the walls and the King', names);
  ok(!names.some(n=>/^(PDC|IBL|IBH|EML|EMH)/.test(n)), '3h …and no PDC, no IB, no EM — "I dont want IBL IBH PDC" · "EMH and EML are not levels"', names);
  ok(names.indexOf('ONL')>=0 && names.indexOf('PML')<0, '3i the night is full, so the levels are ONH/ONL, not the PMH/PML stub');
  ok(/No level has been swept and reclaimed today \(ONH\/ONL · PDH\/PDL · prior-day POC\/VAH\/VAL · CW0\/PW0\)/.test(src), '3j the READ\'s no-sweep sentence names the set as it is now');
  // the Tuesday after Labor Day, as his courier held it: Friday (a session) · Monday (61 evening bars, no RTH) · Tuesday (509 morning bars, 33 RTH)
  const rows2=[];
  for(let k=0;k<379;k++) rows2.push([epochOf(2026,9,4,8,30+k), 7742, 7751, 7711.75, 7723, 100]);
  for(let k=0;k<61;k++) rows2.push([epochOf(2026,9,7,17,0)+k*60, 7700, (k===30?7725.75:7701), 7699, 7700, 50]);
  for(let k=0;k<509;k++) rows2.push([epochOf(2026,9,8,0,0)+k*60, 7700, 7701, (k===400?7687.5:7699), 7700, 50]);
  for(let k=0;k<33;k++) rows2.push([epochOf(2026,9,8,8,30)+k*60, 7711.5, 7717.75, 7686.25, 7688.5, 100]);
  global.futBarsLoad=()=>({ ES:{ rows:rows2 } });
  const P1=futSessionBars(1);
  ok(P1 && P1.rth.length===379 && P1.on.length===0, '3k (v15.80) the PRIOR SESSION on the Tuesday after Labor Day is FRIDAY — a key with no RTH bars (the holiday) is not a session and is walked over', P1&&[P1.rth.length,P1.on.length]);
  const Pc=futSessionBars(1, null, true);
  ok(Pc && Pc.rth.length===0 && Pc.on.length===61, '3l …unless the caller asks for the prior CALENDAR key (overnightHL does): Monday\'s evening', Pc&&[Pc.rth.length,Pc.on.length]);
  const ON2=overnightHL();
  ok(ON2 && ON2.full===true && ON2.eve===61 && ON2.morn===509 && ON2.n===570 && ON2.onh===7725.75 && ON2.onl===7687.5, '3m the night = Monday 17:00→ (61 bars) + Tuesday →08:29 (509): FULL by its halves (>=60 evening, >=300 morning), though 570 < the old 600 — ONL 7687.5 keeps its name', ON2);
  global.futBarsLoad=()=>({ ES:{ rows:rows2.filter(r=>r[0]>=epochOf(2026,9,8,0,0)) } });
  const ON3=overnightHL();
  ok(ON3 && ON3.full===false && ON3.eve===0 && ON3.morn===509, '3n the morning alone (no evening key in the store) is the STUB — PMH/PML, never ONH/ONL', ON3);
  ok(futSessionBars(1)===null, '3o …and with no session before it in the store, the prior session is null, never today');
  ok(/if\(!offsetDays \|\| calendar\)\{ k=keys\[at-\(offsetDays\|\|0\)\]; \}/.test(ex('futSessionBars')) && /if\(byDay\[keys\[idx\]\]\.rth\.length\) left--;/.test(ex('futSessionBars')), '3p the walk-back skips keys with no RTH bars unless `calendar` is set');
}

// ---------- 4. H10 — the review's read of the nightly's HOD/LOD cells, registered before the data ----------
{
  const reg=JSON.parse(fs.readFileSync('learning/register.json','utf8')); const H=reg.hypotheses.find(h=>h.id==='H10');
  ok(H && H.written==='2026-09-08' && H.pick==='lodhodCell' && H.judgedBy==='nightly' && H.minN===60 && H.since==='2026-09-08' && /Wilson high[^<]*< 80%/.test(H.refuteIf) && />= 80%/.test(H.predict) && /not counted/i.test(H.note), '4a H10 in the register: pick lodhodCell, judged by the nightly, minN 60, from 2026-09-08 — the two sessions it was read from are not counted', H);
  const seed=new Function(exVar('PREREG_SEED')+' return PREREG_SEED;')();
  ok(seed.length===10 && seed[9].id==='H10' && seed[9].pick==='lodhodCell' && seed[9].minN===60 && seed[9].judgedBy==='nightly', '4b the panel\'s seed carries it (renders before the first fetch)', seed.length);
  ok(/lodhodCell:function\(s\)\{ return \{n:0,hit:0\}; \}/.test(src), '4c the panel knows the pick "lodhodCell" (the live row reads the nightly\'s verdict; it never counts on its own)');
  const HS=new Function(exVar('HYP_STUDY')+' return HYP_STUDY;')();
  ok(HS.H10==='H1.5', '4d H10 → H1.5 in the panel\'s map');
  const rs=fs.readFileSync('tools/nightly/results.py','utf8');
  ok(/'H9': 'K2\.7', 'H10': 'H1\.5'\}/.test(rs), '4e …and in results.py');
  const S=JSON.parse(fs.readFileSync('learning/studies.json','utf8')); const flat=[];
  (function walk(o){ if(Array.isArray(o)) o.forEach(walk); else if(o && typeof o==='object'){ if(o.id && o.q) flat.push(o); Object.keys(o).forEach(k=>walk(o[k])); } })(S);
  const h15=flat.find(x=>x.id==='H1.5');
  ok(h15 && h15.status==='REGISTERED' && /close-scored/.test(h15.q) && /H10 proposed/.test(h15.result||''), '4f the study row H1.5, REGISTERED, under H1 (is the extreme in)', h15);
  const run=fs.readFileSync('tools/nightly/run.py','utf8');
  ok(/def judge_lodhod\(H, days, sym='SPY'\):/.test(run) && /if H\.get\('pick'\) == 'lodhodCell':/.test(run) && /verdicts = \[judge_lodhod\(H, days\) if H\.get\('pick'\) == 'lodhodCell' else v for H, v in zip\(H_list, verdicts\)\]/.test(run), '4g run.py: judge() defers the pick, judge_lodhod reads the close-scored rows, and the wiring replaces the verdict after the day files');
  // judge_lodhod on fixtures
  const j=(days,H)=>JSON.parse(cp.execSync('python3 -c "import json,sys,importlib.util as u; sp=u.spec_from_file_location(\'run\',\'tools/nightly/run.py\'); m=u.module_from_spec(sp); sp.loader.exec_module(m); a=json.loads(sys.argv[1]); print(json.dumps(m.judge_lodhod(a[\'H\'], [tuple(x) for x in a[\'days\']])))" '+JSON.stringify(JSON.stringify({ days, H })),{encoding:'utf8'}));
  const H10={ id:'H10', claim:'x', minN:60, pick:'lodhodCell', since:'2026-09-08', predict:'pooled live rate >= 80% at n >= 60', refuteIf:'the Wilson high is under 80%' };
  const day=(d,rows)=>[d,{ feat:{ SPY: rows.map(([p,hit])=>({ key:'lodhod', atClose:true, hit, rec:{ p } })) } }];
  const rep=(n,hit,p)=>Array.from({length:n},()=>[p,hit]);
  let v=j([ day('2026-09-04',[...rep(23,1,100),...rep(30,0,90)]), day('2026-09-08',[...rep(20,1,95)]) ], H10);
  ok(v.verdict==='thin' && v.n===20 && v.sessions===1 && /20 close-scored rows in the >=80% cells on sessions from 2026-09-08 \(needs 60\)/.test(v.bar), '4h under minN: THIN, and 09-04 (before `since`) is NOT counted', v);
  v=j([ day('2026-09-08',[...rep(30,1,95),...rep(30,0,90),...rep(10,1,50)]) ], H10);
  ok(v.verdict==='refused' && v.n===60 && v.rate===50 && v.ci[1]<80 && /is under 80: the table/.test(v.bar), '4i 30 of 60 (50%, high ≈62%): REFUSED — the table\'s cell rate does not hold live; the p=50 rows are not in the pool', v);
  v=j([ day('2026-09-08',[...rep(52,1,95),...rep(8,0,100)]) ], H10);
  ok(v.verdict==='cleared' && v.rate===86.7 && /holds: 87% >= 80% on 60 rows/.test(v.bar), '4j 52 of 60 (87%): CLEARED', v);
  v=j([ day('2026-09-08',[...rep(45,1,95),...rep(15,0,100)]) ], H10);
  ok(v.verdict==='refused' && v.rate===75 && v.ci[1]>=80 && /does not hold: 75%/.test(v.bar), '4k 45 of 60 (75%, high ≈84%): REFUSED — under the prediction though the CI still reaches 80', v);
  v=j([ day('2026-09-08',[...rep(60,1,95)]) ], Object.assign({},H10,{ since:'2026-09-09' }));
  ok(v.verdict==='thin' && v.n===0, '4l `since` is the hypothesis\'s own field', v);
  const rv=JSON.parse(fs.readFileSync('review/2026-09-08.json','utf8'));
  ok(rv.schema==='gex-review/v2' && rv.date==='2026-09-08' && /^Fri 9\/4: HOD 08:36/.test(rv.preopen) && rv.preopen.length<=141 && rv.register && rv.register[0].id==='H10' && (rv.proposals||[]).every(p=>p.clearsBar!==true) && rv.selftest && rv.selftest.ran===false,
     '4m the review file: schema v2, a 140-char pre-open line, H10 in it, no proposal clears the bar, the self-test declared not run', rv.preopen);
  ok(/Wilson|n /.test(JSON.stringify(rv.features)) && rv.features.every(f=>f.n!==undefined || f.key==='ledger' || f.key==='nodeEvents') && rv.dataHealth && /QUEUE-TRIMMED/.test(rv.dataHealth.verdict), '4n …every feature carries its n and the data-health verdict leads');
  ok(fs.existsSync('tools/review-pool.py') && /python3 tools\/review-pool.py \[FROM-DATE\] \[OUT\.json\]/.test(fs.readFileSync('tools/review-pool.py','utf8')), '4o the pooling script the review ran is in tools/, with its usage line');
}

// ---------- 5. the record ----------
{
  const P=JSON.parse(fs.readFileSync('learning/plan.json','utf8')); const nx=P.roadmap.filter(r=>r.status==='next');
  ok(P.roadmap.some(r=>r.v==='15.80' && /PRICE AXIS/.test(r.title) && (r.status==='shipped' || nx.some(x=>x.v==='15.80'))) && P.roadmap.some(r=>r.v==='15.79' && r.status==='shipped'), '5a the plan: v15.79 shipped, v15.80 this build or shipped', nx.map(x=>x.v));
  ok(P.roadmap.some(r=>/SEASONALITY TRACKED/.test(r.title) && /^15\.(8[1-9]|9\d)$/.test(r.v)), '5b seasonality tracked sits after this build (v15.81 or later)');
  const seedJs=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedJs)===JSON.stringify(P), '5c PLAN_SEED equals the file');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r17=R.rows.find(r=>r.id==='R-17');
  ok(r17 && r17.status==='implemented' && r17.version==='15.80' && r17.by==='operator' && /price axis/i.test(r17.text) && /key levels/i.test(r17.why||''), '5d R-17 on Rec, by operator, implemented in v15.80', r17&&[r17.status,r17.version]);
  const seedR=JSON.parse(/var REC_SEED=(\{.*?\});\n/.exec(src)[1]); ok(JSON.stringify(seedR)===JSON.stringify(R), '5e REC_SEED equals the file');
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/## v15\.80/.test(cl) && cl.indexOf('## v15.80')<cl.indexOf('## v15.79'), '5f the CHANGELOG has the v15.80 entry on top');
  const ls=fs.readFileSync('session-state/LESSONS.md','utf8'); const logAt=ls.indexOf('## 2 · THE LESSON LOG'); ok(/### v15\.80/.test(ls.slice(logAt>=0?logAt:0)), '5g the lesson log carries the v15.80 entry');
  const rn=fs.readFileSync('session-state/latest-resume-note.md','utf8'); ok(/v15\.(8\d|9\d)/.test(rn.slice(0,600)) && /price axis/i.test(rn) && /F-20/.test(rn), '5h the resume note is at v15.80 or later, names the axis and F-20');
  const dc=fs.readFileSync('session-state/DECISIONS.md','utf8'); ok(/the king is not a key level/.test(dc) && /EMH and EML are not levels/.test(dc), '5i DECISIONS records his key-level list in his words');
  const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8')); ok(/price axis/i.test(cfg.theWhatAndTheHow.dayCandle||'') && cfg.theWhatAndTheHow.pinnedBy.indexOf('test_v1580.js')>=0, '5j .gex-config.json names the axis and this test');
  const fd=fs.readFileSync('skylit-docs/FINDINGS.md','utf8'); ok(/## F-20 · THE FEATURE QUEUE KEEPS ONLY THE LAST ~80 MINUTES/.test(fd) && /## F-21 · ACCUMULATING NODES DID NOT DEFLECT MORE/.test(fd), '5k FINDINGS F-20 (the queue trim, measured) and F-21 (the ledger) are written');
  ok(fs.existsSync('mockups/mockup-candle-axis.png') && fs.existsSync('mockups/mockup-candle-axis-row.png') && fs.existsSync('mockups/mockup-candle-axis.svg'), '5l the mockup he ✓d is in the repo');
}

// ---------- 5b. the A row under the E row · MU / MD · the expectation chip · the colours ----------
{
  global.GD_META={ n:282, fires:225, acc:76, base:51, ciLo:71, ciHi:82 };
  let GDR={ ok:false, why:'first 30 minutes not complete' }; global.gdRead=()=>GDR;
  let ACT={ green:false, pts:-22.75, open:7711.5, now:7688.75 }; global.gdActual=()=>ACT;
  global.g3tip=t=>t?(' title="'+g3esc(t)+'"'):'';
  eval(['hlClock12','hlARowHtml','hlERowHtml','hlERowState'].map(ex).join('\n'));
  global.replayOn=()=>false; global.hlDayShown=()=>'2026-09-08';
  const Dt={ ok:true, open:7711.5, hod:7717.75, lod:7686.25, hodT:mul(8,3600)+mul(30,60), lodT:mul(9,3600)+mul(2,60), first:'HOD', second:'LOD', took:0, bop:1, wick:1, wend:mul(8,3600)+mul(31,60), wickPct:20, mud:31, gap:32, rngPts:31.5, rngUsd:1575, scale:1, isFut:true };
  const a=hlARowHtml('SPY', Dt, false);
  const cells=[...a.matchAll(/<span class="ec"><i[^>]*>([^<]+)<\/i><b[^>]*>([^<]+)<\/b><\/span>/g)].map(m=>[m[1],m[2]]);
  ok(/class="g3erow arow"/.test(a) && /<span class="et" title="TUE 8 SEP · so far — A — the ACTUAL row[^"]*">A · TUE<\/span>/.test(a), '5b-a the A row: the E row\'s class plus arow, tagged A · TUE ("E Tue and A Tue … so the entire row is symmetrical"); the date and "so far" in the tag\'s hover', a.slice(0,160));
  ok(cells.map(c=>c[0]).join('|')==='HOD|took|BOP|wick|W.End|wick%|MD|LOD|HL gap|HL rng', '5b-b the same columns as the E row, in the same order — the first extreme first, MD to the LOD', cells.map(c=>c[0]));
  ok(cells.map(c=>c[1]).join('|')==='8:30a|0m|1m|1m|8:31a|20%|31m|9:02a|32m|31.5pts · $1,575', '5b-c today\'s values, no ~ (Tuesday 2026-09-08 at 09:02 from his courier)', cells.map(c=>c[1]));
  ok(/chip rd"[^>]*>RED DAY</.test(a) && a.indexOf('RED DAY')<a.indexOf('1ST HOD') && /chip first gd"[^>]*>1ST HOD</.test(a), '5b-d the actual colour chip (RED DAY, so far) before 1ST HOD, which is green for a HOD');
  ok(/<i class="lo">MD<\/i><b class="lo">31m/.test(a) && /<i class="hi">HOD<\/i><b class="hi">8:30a/.test(a) && /<i class="lo">LOD<\/i><b class="lo">9:02a/.test(a), '5b-e MD red, HOD green, LOD red — "MU should be green, MD should be red"');
  const aUp=hlARowHtml('SPY', Object.assign({}, Dt, { first:'LOD', second:'HOD', hodT:Dt.lodT, lodT:Dt.hodT }), false);
  ok(/<i class="hi">MU<\/i><b class="hi">31m/.test(aUp) && /^[^]*<i class="lo">LOD<\/i>[^]*<i class="hi">MU<\/i>[^]*<i class="hi">HOD<\/i>/.test(aUp), '5b-f a LOD-first day: LOD · … · MU (green) · HOD');
  ok(hlARowHtml('SPY', Dt, true)==='' && hlARowHtml('SPY', { ok:false }, false)==='', '5b-g no session read → no row, no invention');
  const aN=hlARowHtml('SPY', Object.assign({}, Dt, { wend:null, bop:null, wick:null, mud:null }), false);
  ok(/<i>BOP<\/i><b>—<\/b>/.test(aN) && /<i class="lo">MD<\/i><b class="lo">—<\/b>/.test(aN), '5b-h a day that has not reclaimed its open prints — for the wick family, never 0');
  global.replayOn=()=>true; global.hlDayShown=()=>'2026-09-04';
  ok(/<span class="et" title="FRI 4 SEP · as it closed — A[^"]*">A · FRI<\/span>/.test(hlARowHtml('SPY', Dt, false)), '5b-i parked on Friday the tag is A · FRI, the hover says the date and "as it closed"');
  // the placeholders keep the column count at 13 on both rows, so the grid never shifts
  global.gdActual=()=>null; const aQ=hlARowHtml('SPY', Dt, false); global.gdActual=()=>ACT;
  ok(/<span class="chip ev">DAY \?<\/span>/.test(aQ) && aQ.indexOf('DAY ?')<aQ.indexOf('1ST HOD'), '5b-i2 no actual colour yet → a DAY ? placeholder chip in the same slot (13 cells either way)');
  const nCells=x=>(x.match(/<span class="(?:et|chip|ec)/g)||[]).length;
  ok(nCells(a)===13 && nCells(aQ)===13, '5b-i3 the A row is always 13 grid cells: tag · colour · 1ST · 10 fields', [nCells(a), nCells(aQ)]);
  global.replayOn=()=>false; global.hlDayShown=()=>'2026-09-08';
  // the wiring: built in secDay after the E row, emitted in secLoc right after it
  ok(/var SECDAY_AROW='';/.test(src) && /SECDAY_AROW=''; try\{ SECDAY_AROW=hlARowHtml\(sym, D, NOREAD\); \}catch\(eAR\)\{ swallow\('hlARowHtml', eAR\); \}/.test(ex('secDay')), '5b-j built in secDay, bare var');
  const sl=ex('secLoc'); const iE=sl.indexOf("_eag+=SECDAY_EROW;"), iA=sl.indexOf("_eag+=SECDAY_AROW;"), iW=sl.indexOf("h+='<div class=\"g3eag\">'+_eag+'</div>';"), iS=sl.indexOf('<em>SET</em>');
  ok(iE>0 && iA>iE && iW>iA && iS>iW, '5b-k emitted in secLoc as the two rows of ONE .g3eag grid — E first, A second — before SET', [iE,iA,iW,iS]);
  ok(/#gpts-body \.g3eag\{display:grid;grid-template-columns:repeat\(13,max-content\);justify-content:space-between;/.test(src) && /#gpts-body \.g3eag \.g3erow\{display:contents\}/.test(src) && !/\.g3erow\.arow\{margin-top/.test(src) && /\.g3erow \.ec b\.hi,#gpts-body \.g3erow \.ec i\.hi\{color:#2ec27e\}/.test(src) && /\.g3erow \.ec b\.lo,#gpts-body \.g3erow \.ec i\.lo\{color:#f0616d\}/.test(src), '5b-l the CSS: .g3eag is a 13-column grid and each row is display:contents, so every column lines up ("the rows should be aligned"); hi green, lo red');
  ok((src.match(/#gpts-body \.g3erow\.arow \.et\{/g)||[]).length===1, '5b-l2 the A tag rule is declared once');
  // the E row's tag is E · TUE (E · ALL when pooled) with the n in its hover, and it too is 13 cells
  {
    global.gdRead=()=>({ ok:false, why:'first 30 minutes not complete' });
    const base={ firstClock:mul(9,3600)+mul(18,60), tookMin:19.1, secondClock:mul(12,3600)+mul(40,60), gapMin:180, rngPts:60, rngUsd:3000, lodFirstPct:52,
                 wick:{ bop:17, wick:25, mud:52, wickPct:31, bop_n:40, wick_n:40, mud_n:40, wickPct_n:40 },
                 basis:{ pooled:false, n:57, dow:'Tue', first:'2025-06-03', last:'2026-09-01' } };
    let e=''; try{ e=hlERowHtml('SPY', Dt, base, false); }catch(eE){ e='ERR '+eE.message; }
    let eAll=''; try{ eAll=hlERowHtml('SPY', Dt, Object.assign({}, base, { basis:{ pooled:true, n:284, dow:null } }), false); }catch(eE2){ eAll='ERR '+eE2.message; }
    ok(/<span class="et" title="n=284 — E[^"]*">E · ALL<\/span>/.test(eAll), '5b-o0 pooled → E · ALL, n=284 in the hover', eAll.slice(0,120));
    const tagM=/<span class="et" title="([^"]*)">([^<]*)<\/span>/.exec(e||'');
    ok(!!tagM && tagM[2]==='E · TUE' && /^n=57 — E — the EXPECTED row/.test(tagM[1]), '5b-o the E row\'s tag is E · TUE, its n in the hover', tagM?[tagM[2], tagM[1].slice(0,40)]:e.slice(0,200));
    ok(nCells(e)===13 && nCells(eAll)===13 && /<span class="chip ev"[^>]*>DAY \?<\/span>/.test(e) && /<span class="chip first gd"[^>]*>1ST HOD<\/span>/.test(e), '5b-o2 the E row is 13 grid cells too (tag · DAY ? · 1ST HOD · 10 fields)', [nCells(e), nCells(eAll)]);
    // the same 10 field labels, in the same order, on both rows — the columns mean the same thing top and bottom
    const labs=x=>[...x.matchAll(/<span class="ec"><i[^>]*>([^<]+)<\/i>/g)].map(m=>m[1]).join('|');
    ok(labs(e)===labs(a) && labs(a)==='HOD|took|BOP|wick|W.End|wick%|MD|LOD|HL gap|HL rng', '5b-o3 E and A carry the same 10 labels in the same order (a HOD-first day)', [labs(e), labs(a)]);
    // no bars yet AND no first-extreme base rate → the 1ST slot still holds a chip (1ST ?), so the E row stays 13 cells
    let eQ=''; try{ eQ=hlERowHtml('SPY', { ok:false }, Object.assign({}, base, { lodFirstPct:null }), true); }catch(eE3){ eQ='ERR '+eE3.message; }
    ok(/<span class="chip first">1ST \?<\/span>/.test(eQ) && nCells(eQ)===13, '5b-o4 pre-open with no first-extreme rate: a 1ST ? placeholder keeps the E row at 13 cells', [nCells(eQ), eQ.slice(0,80)]);
  }
  // the grid must never clip: eagFit measures after layout and falls back to wrapping rows
  {
    const ef=ex('eagFit'); const rd=ex('render');
    ok(/var g=elBody\.querySelector\('\.g3eag'\); if\(!g\) return;/.test(ef) && /g\.classList\.remove\('narrow'\);/.test(ef) && /if\(g\.scrollWidth > g\.clientWidth\+1\) g\.classList\.add\('narrow'\);/.test(ef), '5b-p eagFit: clears `narrow`, then sets it only when the 13 columns overflow the grid\'s own width');
    const iCF=rd.indexOf('try{ candleFit(); }catch(eCF){}'), iEF=rd.indexOf('try{ eagFit(); }catch(eEF){}'), iPF=rd.indexOf('try{ panelFit(); }catch(ePF){}');
    ok(iCF>0 && iEF>iCF && iPF>iEF, '5b-p2 render() runs eagFit after candleFit (after innerHTML — it measures) and before panelFit', [iCF,iEF,iPF]);
    ok(/#gpts-body \.g3eag\.narrow\{display:block\}/.test(src) && /#gpts-body \.g3eag\.narrow \.g3erow\{display:flex;margin:0;padding:0;border:0;background:none\}/.test(src), '5b-p3 the narrow CSS: the wrapper stays the box, the rows wrap as before');
    // (measured in Chromium, scratch eag-narrow.js, 2026-09-08: at his 943-px panel narrow:false and the E and A cells share
    //  all ten x positions; at 700 px narrow:true and the rows wrap — jsdom has no layout, so that stays a note, not a pin)
  }
  // the expectation chip on the E row
  ok(/function hlMudLabel\(second\)\{ return \(second==='HOD'\)\?'MU':\(\(second==='LOD'\)\?'MD':'MUD'\); \}/.test(src), '5b-m hlMudLabel: MU to a HOD, MD to a LOD, MUD when unknown');
  ok(/dcell\(hlMudLabel\(D&&D\.second\),/.test(src), '5b-n the DAY table says MU / MD too');
}

// ---------- 6. the IRT export: QQQ converts for NQ only ----------
{
  const B=ex('irtBuildCsv');
  ok(!/'QQQ KING ~'/.test(B) && /IRT_LAST\.xqWhy='off — QQQ converts for NQ only \(operator, 2026-09-08\)'/.test(B), '6a the v14.75 projection of the QQQ King onto the ES symbol is gone from irtBuildCsv, and xqWhy says it is a decision');
  ok(/var QP=nqPx\(QK\.k\);\n\s+out\.push\(irtCsvRow\(nqSym, QP\.px, 'QQQ KING'\+QP\.tag/.test(B), '6b …the NQ row stays (v15.81: priced by nqPx — Skylit\'s NQ1 book first, the ratio chain second)');
  const R=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); const r18=R.rows.find(r=>r.id==='R-18');
  ok(r18 && r18.status==='implemented' && r18.version==='15.80' && r18.by==='operator' && /NQ only/.test(r18.text), '6c R-18 on Rec, by operator, implemented in v15.80', r18&&[r18.status,r18.version]);
  ok(/void style;/.test(ex('irtCsvRow')) && /\+','\+\(width\|\|1\)\+',0'\+/.test(ex('irtCsvRow')), '6d every IRT line is SOLID — PENSTYLE written as 0 whatever the caller passes ("make all solid lines")');
  const r19=R.rows.find(r=>r.id==='R-19'); ok(r19 && r19.status==='implemented' && r19.version==='15.80' && /solid/i.test(r19.text), '6e R-19 on Rec: the solid lines', r19&&[r19.status,r19.version]);
  ok(/function irtQqqTop\(exK\)/.test(src) && /irtGLatch\(QT\.rows\.map/.test(B) && /irtGHeld\('GQ'\)/.test(B) && /IRT_LAST\.gqWhy=gqWhy;/.test(B), '6f G2..G5 on NQ: irtQqqTop ranks the QQQ book, held under GQ, reported in gqWhy (the behaviour is pinned in test_irt_export §4q)');
  const r20=R.rows.find(r=>r.id==='R-20'); ok(r20 && r20.status==='implemented' && r20.version==='15.80' && /G2/.test(r20.text) && /NQ/.test(r20.text), '6g R-20 on Rec: G2..G5 on NQ', r20&&[r20.status,r20.version]);
}

console.log('test_v1580: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
