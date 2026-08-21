// (v11.24) SNAPSHOT SELECTION — the defect that invalidated every level we shipped this week.
//
// `levels` is a ~390-entry time series and the LAST entry is the CURRENT, STILL-FORMING bar. Measured on
// the live feed 2026-08-20, counting strikes where |net| < v (i.e. carrying a real call/put split):
//
//      snapshot   0 -> 84        195 -> 78        388 -> 82        389 -> 0
//
// We read 389. Every strike therefore looked 100% call or 100% put, our call wall picked 765 — a strike a
// published book shows as heavily PUT-dominant — and values appeared to swing 20x between fetches because
// I was comparing a degenerate frame against a complete one. Confirmed on BOTH the self-fetched sets and
// the passive feed: after the close, callPut() reported lt:0, eq:208, gt:0, decomposable:false.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.window={__gptsDebug:{}};
global.SNAP_MAX_STEPBACK=8;
eval(ex('pickSnapshot'));

const mixedRow=(k)=>({k:k, v:100e6, d:1, net:60e6});      // |net| < v  -> real split
const pureRow =(k)=>({k:k, v:100e6, d:1, net:100e6});     // |net| == v -> no split
const frame=(rows)=>({t:1, s:762, l:rows});

{
  // the live shape: many good frames, a degenerate newest one
  const snaps=[];
  for(let i=0;i<20;i++) snaps.push(frame([mixedRow(760),mixedRow(765),pureRow(770)]));
  snaps.push(frame([pureRow(760),pureRow(765),pureRow(770)]));   // the still-forming bar
  const P=pickSnapshot(snaps);
  ok(P.idx===snaps.length-2,'steps back off the still-forming bar to the last COMPLETE frame',P.idx);
  ok(P.steppedBack===1,'and reports how far it stepped',P.steppedBack);
  ok(P.degenerate===false,'the chosen frame is not degenerate');
  ok(P.mixed===2,'and how many strikes carry a real split',P.mixed);
}
{
  // a healthy newest frame is used as-is — no gratuitous stepping back
  const snaps=[frame([mixedRow(760)]), frame([mixedRow(760),mixedRow(765)])];
  const P=pickSnapshot(snaps);
  ok(P.idx===1 && P.steppedBack===0,'a complete newest frame is used directly',P);
}
{
  // several degenerate frames in a row — keep walking
  const snaps=[];
  for(let i=0;i<10;i++) snaps.push(frame([mixedRow(760),mixedRow(765)]));
  for(let i=0;i<4;i++) snaps.push(frame([pureRow(760),pureRow(765)]));
  const P=pickSnapshot(snaps);
  ok(P.steppedBack===4,'walks back past a run of degenerate frames',P.steppedBack);
  ok(P.degenerate===false,'and lands on a real one');
}
{
  // THE GUARD: never walk back forever. A frame from twenty minutes ago wearing a current timestamp is
  // worse than admitting we have nothing — so beyond the bound it returns the newest frame FLAGGED.
  const snaps=[frame([mixedRow(760)])];
  for(let i=0;i<30;i++) snaps.push(frame([pureRow(760),pureRow(765)]));
  const P=pickSnapshot(snaps);
  ok(P.degenerate===true,'beyond the step-back bound it gives up and FLAGS rather than reaching for stale data',P);
  ok(P.steppedBack===0,'handing back the newest frame, not a stale one',P.steppedBack);
  ok(P.idx===snaps.length-1,'which is the newest index',P.idx);
}
{
  // every frame degenerate (the after-hours case observed live)
  const snaps=[frame([pureRow(760)]), frame([pureRow(760),pureRow(765)])];
  const P=pickSnapshot(snaps);
  ok(P.degenerate===true,'an all-degenerate payload is flagged, not silently decomposed',P.degenerate);
  ok(P.snap.l.length===2,'and still yields rows — the MAGNET only needs |net|, so it survives',P.snap.l.length);
}
{
  // empty frames are skipped rather than chosen
  const snaps=[frame([mixedRow(760),mixedRow(765)]), frame([]), frame([])];
  const P=pickSnapshot(snaps);
  ok(P.idx===0 && P.degenerate===false,'empty frames are skipped',P);
}
{
  ok(pickSnapshot([])===null,'no snapshots, nothing to pick');
  ok(pickSnapshot(null)===null,'null is handled, not thrown');
  const P=pickSnapshot([frame([{k:760,v:100e6}])]);
  ok(P!==null,'a row missing net does not throw');
  ok(P.degenerate===true,'and counts as no decomposition',P.degenerate);
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
