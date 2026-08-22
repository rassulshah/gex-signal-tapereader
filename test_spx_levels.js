// (v11.14) LEVELS COMPUTED NATIVELY ON THE SPXW LANE, IN SPX POINTS.
// Why this exists: comparing our levels to an SPX gamma page always ran through a ratio, so any disagreement
// could be blamed on the conversion. Skylit's `derived` SPXW lane carries SPX-scale strikes with absolute
// dollar values, so the levels can be computed in SPX points directly and the conversion leaves the argument.
//
// Ground truth for the expectations below — their SPX page on 2026-08-20, spot 7642.21:
//     view        Call Wall  Put Wall  Zero Gamma  ratio
//     0DTE          7760       7640      7695.17   0.16
//     next week     7775       7640      7672.98   0.44
//     all exps      7900       7640      7660.22   0.82
// The put wall is 7640 in ALL THREE. The call wall moves 140 points across them and is defined on CALL gamma,
// which this feed does not decompose — so ours is reported as CR(net) and must never be labelled Call Resistance.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.window={__gptsDebug:{}};
eval(['spxwLane','spxLevels'].map(ex).join('\n'));

const RATIO=0.09950;                       // SPX -> SPY, as the feed ships it
const SPY_SPOT=+(7642.21*RATIO).toFixed(2);
function feed(rows){
  global.LASTFEED={ SPY:{ j:{ derived:[ {source:'SPXW', ratio:RATIO,
    levels:[ {t:1,l:[]}, {t:2,l:rows.map(r=>({k:r[0], v:Math.abs(r[1]), d:(r[1]>=0?1:-1)}))} ] } ] } } };
  global.STATE={ SPY:{ price:SPY_SPOT } };
}
// a put-dominated book shaped like the real one: heavy puts at/below spot, thin calls above
// NOTE 7660: a heavy PUT strike sitting ABOVE spot. That is not a contrivance — in a book where puts
// outweigh calls better than 6 to 1, put strikes above spot routinely carry more gamma than the call
// strikes do, and this is exactly the case that makes a net-based "call wall" the wrong number.
const BOOK=[[7600,-800e6],[7620,-1200e6],[7640,-3000e6],[7660,-1500e6],[7680,-400e6],
            [7700,120e6],[7720,180e6],[7760,900e6],[7800,300e6]];

{
  feed(BOOK);
  const S=spxLevels();
  ok(!!S,'a level set is produced from the SPXW lane alone',!!S);
  ok(Math.abs(S.px-7642.21)<0.5,'spot is expressed in SPX points, not SPY',S.px);
  ok(S.ps===7640,'PS lands on 7640 — the strike their page reports in all three expiry views',S.ps);
  ok(S.mag===7640,'and 7640 is also the heaviest strike in the book',S.mag);
  // THE POINT OF THE WHOLE EXERCISE: the heaviest NET strike above spot is 7660 — a PUT strike carrying
  // 1.5B — while the heaviest CALL strike above spot is 7760, which is what their page reports as the
  // 0DTE Call Wall. Ours is not a rough version of theirs; it is a different strike, 100 points away,
  // and it always will be until the feed decomposes calls from puts.
  ok(S.crNet===7660,'CR(net) is the heaviest NET strike above spot — here a PUT strike',S.crNet);
  ok(S.crNet!==7760,'and so it is NOT their 0DTE Call Wall of 7760',S.crNet);
  ok(S.crNetM===1500000000,'the mass behind that choice is reported, so the divergence is auditable',S.crNetM);
  // cumulative: -800,-2000,-5000,-5900,-6300,-6180,-6000,-5100,-4800 -> never crosses zero
  ok(S.hvl===null,'a book that never flips sign has no FLIP — not a nearest-strike guess',S.hvl);
  ok(S.net<0,'and the net is short, as a 6:1 put book should be',S.net);
}
{
  // REACH is the first thing that decides whether their wall is even computable
  feed(BOOK);
  const S=spxLevels();
  ok(S.kMin===7600 && S.kMax===7800,'the lane range is reported',[S.kMin,S.kMax]);
  ok(S.reachAbovePct>2 && S.reachAbovePct<2.5,'as a percentage above spot, so "does it reach 7900" is answerable',S.reachAbovePct);
  ok(S.reachBelowPct<0,'and below spot',S.reachBelowPct);
  ok(S.step===20,'the strike step says how fine the grid is',S.step);
  ok(S.n===9,'with the strike count',S.n);
  // their all-expiration call wall at 7900 is OUTSIDE this lane — no computation recovers it
  ok(S.kMax<7900,'this lane does not reach 7900, so their full-chain wall is not in our data at all',S.kMax);
}
{
  // a book that DOES flip: calls above outweigh the puts below
  feed([[7600,-500e6],[7640,-700e6],[7700,900e6],[7760,800e6]]);
  const S=spxLevels();
  // cumulative: -500, -1200, -300, +500 -> crossing 7700 -> 7760 at 300/800
  ok(S.hvl===7722.5,'FLIP interpolates the true crossing bracket in SPX points',S.hvl);
  ok(S.hvl>S.px,'and sits above spot here');
}
{
  // CR(net) must follow MASS, not proximity — the whole point of the net-vs-call caveat
  feed([[7640,-3000e6],[7700,100e6],[7760,900e6],[7800,50e6]]);
  const S=spxLevels();
  ok(S.crNet===7760,'the heaviest strike above spot wins over the nearest one',S.crNet);
  ok(S.crNetM===900000000,'its mass is reported so the choice is auditable',S.crNetM);
}
{
  // refusals — never a fabricated level
  global.LASTFEED={ SPY:{ j:{ derived:[] } } };
  ok(spxwLane()===null && spxLevels()===null,'no SPXW lane, no SPX levels');
  feed(BOOK); global.STATE={ SPY:{ price:null } };
  ok(spxLevels()===null,'no price means no levels rather than levels against nothing');
  feed(BOOK); global.STATE={ SPY:{ price:SPY_SPOT } };
  global.LASTFEED.SPY.j.derived[0].ratio=0;
  ok(spxLevels()===null,'a zero ratio is refused rather than dividing by it');
  global.LASTFEED={ SPY:{ j:{ derived:[ {source:'SPXW', ratio:RATIO, levels:[{t:1,l:[{k:7600,v:1,d:-1}]}]} ] } } };
  ok(spxwLane()===null,'a lane with too few strikes is not enough to place a wall');
  global.LASTFEED={};
  ok(spxwLane()===null,'a missing feed is handled, not thrown');
}
{
  // the lane is a TIME SERIES — the latest snapshot is the one that counts
  global.LASTFEED={ SPY:{ j:{ derived:[ {source:'SPXW', ratio:RATIO, levels:[
    {t:1,l:[[7000,-1],[7010,-1],[7020,-1],[7030,-1]].map(r=>({k:r[0],v:1,d:-1}))},
    {t:2,l:BOOK.map(r=>({k:r[0], v:Math.abs(r[1]), d:(r[1]>=0?1:-1)}))} ] } ] } } };
  global.STATE={ SPY:{ price:SPY_SPOT } };
  const S=spxLevels();
  ok(S.kMin===7600,'the LAST snapshot is read, not the first — a stale first frame cannot set the levels',S.kMin);
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
