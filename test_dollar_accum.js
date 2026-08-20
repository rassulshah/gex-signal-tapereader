// (v11.5) ACCUMULATION IN DOLLARS (shadow) + ONE RECORD PER ROLL STEP + the edge path.
// Fixtures reproduce the live 2026-08-20 session: the King (765) grew 26% in dollars while its %King sat
// pinned at 100, and every floor node around it read "dec" purely because the denominator moved.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.ACM_UP=8; global.ACM_DN=-8; global.ACM_DROP=25;
global.MAP_ACM=8; global.MAP_DEC=-8; global.MAP_DROP=25; global.FEED_M15_SAMPLES=15; global.mul=(a,b)=>a*b;
eval(['mapNodeState','feedSampleAt','feedGoneAt','ledgerStateAt','ledgerStateAtAbs','mapTransfersOf'].map(ex).join('\n'));

// ---------- 1. the King: dollars up 26%, %King pinned at 100
const N=40;
const kingPct=new Array(N).fill(100);                                   // the denominator can never move
const kingAbs=[]; for(let i=0;i<N;i++) kingAbs.push(Math.round(112611*Math.pow(142214/112611, i/(N-1))));   // +26% over the window, live 10:18→10:54
ok(ledgerStateAt(kingPct, N-1).st==='hold' && ledgerStateAt(kingPct,N-1).m15===0, '1a %King basis: the King is forever "hold", m15 0 — it IS the denominator', ledgerStateAt(kingPct,N-1));
const ka=ledgerStateAtAbs(kingAbs, N-1);
ok(ka.st==='acm' && ka.m15>=8, '1b dollar basis: the same King reads ACM (+'+ka.m15+'% over 15 samples) — the roll INTO the King is now visible', [ka.st,ka.m15]);

// ---------- 2. phantom dissipation: flat dollars, shrinking percentage
const flatAbs=new Array(N).fill(50000);
const flatPct=[]; for(let i=0;i<N;i++) flatPct.push(Math.round(100*50000/kingAbs[i]));   // ratio to the growing King
ok(ledgerStateAt(flatPct, N-1).st==='dec', '2a %King basis calls a node with FLAT dollars "dec" — phantom dissipation', ledgerStateAt(flatPct,N-1));
ok(ledgerStateAtAbs(flatAbs, N-1).st==='hold', '2b dollar basis calls the same node "hold" — the two are now separable', ledgerStateAtAbs(flatAbs,N-1));

// ---------- 3. a genuinely bleeding node reads dec on BOTH bases
const bleedAbs=[]; for(let i=0;i<N;i++) bleedAbs.push(Math.round(90000*(1-0.6*i/(N-1))));
const bleedPct=[]; for(let i=0;i<N;i++) bleedPct.push(Math.round(100*bleedAbs[i]/kingAbs[i]));
ok(ledgerStateAt(bleedPct,N-1).st==='dec' && ledgerStateAtAbs(bleedAbs,N-1).st==='dec', '3a real bleeding is "dec" on both bases — the fix does not hide genuine decay');

// ---------- 4. the transfer logic on dollar states finds the FLOOR roll the %King basis missed
const pxNow=766.4;
const pctNodes=[{k:768,side:'above',state:'dec'},{k:767,side:'above',state:'acm'},{k:766,side:'below',state:'dec'},
                {k:765,side:'below',state:'hold'},{k:764,side:'below',state:'dec'},{k:763,side:'below',state:'dec'}];
const absNodes=[{k:768,side:'above',state:'dec'},{k:767,side:'above',state:'acm'},{k:766,side:'below',state:'hold'},
                {k:765,side:'below',state:'acm'},{k:764,side:'below',state:'dec'},{k:763,side:'below',state:'dec'}];
const tp=mapTransfersOf(pctNodes,pxNow), ta=mapTransfersOf(absNodes,pxNow);
ok(tp.transfers.filter(t=>t.side==='flr').length===0, '4a %King basis: NO floor transfer (nothing below can be acm)', tp.transfers.map(t=>t.side+' '+t.from+'->'+t.to));
const flrA=ta.transfers.filter(t=>t.side==='flr');
ok(flrA.length===2 && flrA.every(t=>t.to===765 && t.dir==='up'), '4b dollar basis: the floor rolls UP into 765 from 764 and 763 — the roll drawn on the chart', flrA.map(t=>t.from+'->'+t.to+' '+t.dir));

// ---------- 5. one record per STEP, not per bar
const MAP_STEP={SPY:null};
function step(t){ // the record's stepNew logic, verbatim from the feature
  const stepKey=t?(t.side+'|'+t.from+'|'+t.to):null;
  const prev=MAP_STEP.SPY||null;
  const isNew=!!(stepKey && (!prev || prev.key!==stepKey));
  if(stepKey){ if(isNew) MAP_STEP.SPY={key:stepKey,t:1,bars:1}; else MAP_STEP.SPY.bars++; } else if(prev) MAP_STEP.SPY=null;
  return {isNew, bars:MAP_STEP.SPY?MAP_STEP.SPY.bars:0};
}
const roll={side:'ceil',from:767,to:768};
let news=0, last=null; for(let i=0;i<8;i++){ last=step(roll); if(last.isNew) news++; }
ok(news===1 && last.bars===8, '5a a roll held 8 bars = ONE record (stepNew), barsActive 8 — not 8 observations', [news,last.bars]);
const r2=step({side:'ceil',from:768,to:769});
ok(r2.isNew && r2.bars===1, '5b a NEW (from,to) starts a new step');
const r3=step(null);
ok(!r3.isNew && MAP_STEP.SPY===null, '5c the step clears when no roll is active');
ok(step(roll).isNew, '5d ...and the same roll re-appearing later is a fresh step');

// ---------- 6. the edge path collapses a bar series into a trajectory
global.TODAY='2026-08-20';
global.fcHistOf=()=>[{d:'2026-08-20',t:1,flr:763,ceil:768},{d:'2026-08-20',t:2,flr:763,ceil:768},{d:'2026-08-20',t:3,flr:764,ceil:768},
                     {d:'2026-08-20',t:4,flr:764,ceil:767},{d:'2026-08-20',t:5,flr:764,ceil:767},{d:'2026-08-20',t:6,flr:765,ceil:767},
                     {d:'2026-08-19',t:0,flr:700,ceil:701}];
eval(ex('edgePath'));
const ep=edgePath('SPY');
ok(ep && ep.flr.length===3 && ep.flr.map(x=>x.k).join(',')==='763,764,765', '6a six bars of floor samples collapse to three steps 763 → 764 → 765', ep&&ep.flr);
ok(ep.flrSteps===2 && ep.ceilSteps===1 && ep.flrDir==='up' && ep.ceilDir==='dn', '6b step counts and directions', [ep.flrSteps,ep.ceilSteps,ep.flrDir,ep.ceilDir]);
ok(ep.read==='compression', '6c floor up + ceiling down = compression', ep.read);
ok(ep.bars===6, '6d yesterday\'s samples are not mixed in', ep.bars);

console.log('test_dollar_accum: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
