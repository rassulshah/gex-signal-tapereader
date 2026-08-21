// (v11.15) THE CALL/PUT DECOMPOSITION PROBE.
// I told the user the call wall was unreachable because the feed has no call/put split. That skipped the
// `net` field the strike rows already carry. If v is TOTAL gamma and net is NET gamma then
//        call = (v + net)/2      |put| = (v - net)/2
// and the call wall is computable with no scraping at all. Their own page header satisfies exactly this
// identity (Call 8.3 + |Put| 51.7 = Total 60.0; Call 8.3 - |Put| 51.7 = Net -43.4), so it is very likely
// how they derive it too.
//
// The probe must be equally willing to return the BAD answer. These tests pin all three branches, because
// a probe that only recognises the convenient case is worse than no probe.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.window={__gptsDebug:{}};
eval(ex('callPutProbe'));
function feed(rows){ global.LASTFEED={ SPY:{ j:{ levels:[{t:1,l:[]},{t:2,l:rows}] } } }; }

// ---------- the DECOMPOSABLE case ----------
{
  // strike: call and put both present -> v = call + put, net = call - put
  // net is POSITIVE on a put-dominated strike, so net = put - call (v11.17 sign correction)
  const mk=(k,call,put)=>({k:k, v:call+put, net:put-call, d:(call>=put?1:-1)});
  feed([ mk(760, 100, 900), mk(765, 200, 1800), mk(770, 900, 300), mk(775, 1500, 200) ]);
  global.STATE={SPY:{price:767}};
  const p=callPutProbe('SPY');
  ok(p.decomposable===true,'a feed carrying both total and net is recognised as decomposable',p.verdict);
  ok(p.absNetGreaterThanV===0,'and no strike violates |net| <= v');
  ok(p.sample[0].call===100 && p.sample[0].put===900,'call and put are recovered exactly',p.sample[0]);
  ok(p.callWall===775,'THE CALL WALL: heaviest CALL gamma above spot, their definition',p.callWall);
  ok(p.putWall===765,'and the put wall from put gamma alone',p.putWall);
  ok(p.totalCall===2700 && p.totalPut===3200,'the totals are summed so they can be checked against their header',[p.totalCall,p.totalPut]);
  ok(p.ratio===0.84,'and the call/put ratio, which is the cross-check that proves the decomposition',p.ratio);
  // the point of it all: net-based and call-based walls pick DIFFERENT strikes
  ok(p.callWall!==770,'the call wall is not merely the heaviest net strike above spot',p.callWall);
}
// ---------- the "no extra information" case, which must NOT be dressed up ----------
{
  // net is just the signed magnitude: |net| === v everywhere
  feed([ {k:760,v:900,net:-900,d:-1}, {k:765,v:1800,net:-1800,d:-1}, {k:770,v:300,net:300,d:1} ]);
  global.STATE={SPY:{price:767}};
  const p=callPutProbe('SPY');
  ok(p.decomposable===false,'when |net| equals v there is no split to recover',p.decomposable);
  ok(p.absNetEqualsV===3,'every strike is counted in that bucket',p.absNetEqualsV);
  ok(p.callWall===undefined,'and NO call wall is produced rather than a fabricated one',p.callWall);
  ok(/genuinely absent/.test(p.verdict),'the verdict says so plainly',p.verdict);
}
// ---------- the "fields mean something else" case: report and stop ----------
{
  feed([ {k:760,v:100,net:-900,d:-1}, {k:765,v:200,net:-1800,d:-1}, {k:770,v:50,net:300,d:1} ]);
  global.STATE={SPY:{price:767}};
  const p=callPutProbe('SPY');
  ok(p.absNetGreaterThanV===3,'|net| exceeding v is detected',p.absNetGreaterThanV);
  ok(p.decomposable===false,'and blocks decomposition');
  ok(/do NOT decompose/.test(p.verdict),'with a verdict that refuses to guess at the meaning',p.verdict);
  ok(p.callWall===undefined,'no wall is emitted from fields we do not understand');
}
// ---------- mixed: some strikes carry no net at all ----------
{
  const mk=(k,call,put)=>({k:k, v:call+put, net:put-call, d:1});
  feed([ mk(760,100,900), {k:765,v:2000,d:-1}, mk(770,900,300), mk(775,1500,200), mk(780,50,10) ]);
  global.STATE={SPY:{price:767}};
  const p=callPutProbe('SPY');
  ok(p.noNet===1,'strikes without a net field are counted, not silently dropped',p.noNet);
  ok(p.withNet===4,'and the rest are used',p.withNet);
  ok(p.decomposable===true,'a partial net series still decomposes the strikes that have one',p.decomposable);
}
// ---------- refusals ----------
{
  global.LASTFEED={};
  ok(/no feed/.test(callPutProbe('SPY').err||''),'no feed says so');
  feed([]);
  ok(/no strike rows/.test(callPutProbe('SPY').err||''),'an empty snapshot says so');
  const mk=(k,call,put)=>({k:k, v:call+put, net:put-call, d:1});
  feed([mk(760,100,900),mk(770,900,300),mk(775,1500,200),mk(780,50,10)]);
  global.STATE={SPY:{price:null}};
  const p=callPutProbe('SPY');
  ok(p.decomposable===true && p.callWall===null,'without a price the split still works but no wall is placed',[p.decomposable,p.callWall]);
}
// ---------- the identity itself, on their published numbers ----------
{
  // Call 8.3 + |Put| 51.7 = Total 60.0 ; Call 8.3 - |Put| 51.7 = Net -43.4  (their SPX 0DTE header)
  const total=60.0, net=-43.4;
  ok(Math.abs(((total+net)/2)-8.3)<0.05,'call recovers from total and net on their own published figures');
  ok(Math.abs(((total-net)/2)-51.7)<0.05,'and put does too — which is why this is worth probing at all');
}
// ---------- (v11.16) cpRows / cpLevels against the REAL payload shape ----------
// Fixture rows are verbatim from the live SPY book, 2026-08-20, captured off the wire:
//   {k:760, v:283651666.375,  d:1,  net:278399177.125}   -> nearly all call
//   {k:765, v:174886547.5078, d:-1, net:-174886547.5078} -> |net| == v, all put
//   {k:766, v:61620230.75,    d:-1, net:-51428766.25}    -> mixed
//   {k:761, v:56723968.15625, d:1,  net:55396191.53125}
//   {k:756, v:53983356.5,     d:-1, net:-29581118.5}     -> mixed
{
  // (v11.20) cpLevels now DELEGATES to cpFromPayload — one implementation of the wall/crossing rules
  // instead of two, which is why the nearest-spot zero-gamma fix only half-landed in v11.18.
  global.SIDE_MIN_SHARE=0.05; global.HVL_MAX_DIST=0.03;
  eval(ex('cpRows')); eval(ex('cpFromPayload')); eval(ex('cpLevels'));
  const REAL=[ {k:760,v:283651666.375,d:1,net:278399177.125},
               {k:765,v:174886547.5078125,d:-1,net:-174886547.5078125},
               {k:766,v:61620230.75,d:-1,net:-51428766.25},
               {k:761,v:56723968.15625,d:1,net:55396191.53125},
               {k:756,v:53983356.5,d:-1,net:-29581118.5} ];
  global.LASTFEED={ SPY:{ j:{ levels:[{t:1,l:[]},{t:2,l:REAL}], expirations:['2026-08-20','2026-08-21'], strike_interval:1 } } };
  global.STATE={ SPY:{ price:762.57 } };
  const D=cpRows('SPY');
  ok(D.ok===true,'the real payload decomposes',D);
  ok(D.gt===0,'no strike has |net| > v — the signature of total-and-net',D.gt);
  ok(D.eq===1,'765 is all put, so |net| equals v there',D.eq);
  ok(D.lt===4,'the rest are genuinely mixed',D.lt);
  ok(D.exps.length===2,'the expirations list rides along',D.exps);
  ok(D.step===1,'as does the strike interval',D.step);
  // (v11.17) net is POSITIVE on a put-dominated strike, so put=(v+net)/2. 765 has net = -v exactly,
  // which under the CORRECT convention makes it entirely CALL, not entirely put.
  const r765=D.rows.filter(r=>r.k===765)[0];
  ok(Math.round(r765.put)===0,'a strike with net = -v decomposes to zero PUT',r765.put);
  ok(Math.round(r765.call)===174886548,'and its full magnitude as call',Math.round(r765.call));
  const r766=D.rows.filter(r=>r.k===766)[0];
  ok(Math.round(r766.put/1e6)===5,'a mixed strike splits: 766 is ~5M put',Math.round(r766.put/1e6));
  ok(Math.round(r766.call/1e6)===57,'...and ~57M call',Math.round(r766.call/1e6));
  ok(Math.abs((r766.call+r766.put)-r766.v)<1,'call + put reconstructs the total exactly',[r766.call+r766.put,r766.v]);
  ok(Math.abs((r766.put-r766.call)-r766.net)<1,'and put - call reconstructs the net',[r766.put-r766.call,r766.net]);

  const L=cpLevels('SPY');
  // THE CROSS-CHECK THAT PROVES THE SIGN. On the live book this convention puts the put wall on 760 —
  // exactly the number their SPY page reports, in BOTH its 0DTE and next-week views. The other sign gave
  // 756 and a call-heavy book (ratio 1.44) against their put-heavy 0.02 / 0.36. That is how it was caught.
  ok(L.putWall===760,'the PUT WALL lands on 760 — their published number',L.putWall);
  ok(L.callWall===765,'the call wall is the heaviest CALL strike above spot',L.callWall);
  ok(L.ratio<1,'and the book reads put-heavy, the same direction their header reports',L.ratio);
  ok(L.n===5 && L.kMin===756 && L.kMax===766,'range and count are reported',[L.n,L.kMin,L.kMax]);
  ok(L.subset===true,'and the read is FLAGGED as a subset — the app requests top-N nodes, not the chain',L.subset);
  ok(L.ratio!=null,'a call/put ratio is emitted as the cross-check against a full-chain page',L.ratio);
}
{
  // the refusal paths must still refuse at this layer, not just in the probe
  global.LASTFEED={ SPY:{ j:{ levels:[{t:1,l:[{k:760,v:100,d:1,net:-900}]}] } } };
  global.STATE={ SPY:{ price:762 } };
  ok(/not a total/.test(cpLevels('SPY').err||''),'|net| > v refuses at the levels layer too',cpLevels('SPY'));
  // (v11.18) An all-pure book is NO LONGER refused — on expiry day |net| == v nearly everywhere and that
  // guard is what left the 0DTE set null. (v11.19) But a book with gamma on one side only names no wall on
  // the other: here everything is call, so the PUT wall is suppressed with a 0% share rather than invented.
  global.LASTFEED={ SPY:{ j:{ levels:[{t:1,l:[{k:760,v:100,d:-1,net:-100},{k:765,v:50,d:-1,net:-50}]}] } } };
  const allCall=cpLevels('SPY');
  ok(!allCall.err,'an all-pure book decomposes instead of refusing',allCall.err);
  ok(allCall.callWall===765,'the call side still gets its wall',allCall.callWall);
  ok(allCall.putWall===null,'the empty put side gets none',allCall.putWall);
  ok(allCall.psSuppressed && allCall.psSuppressed.share===0,'and records the 0% share that disqualified it',allCall.psSuppressed);
  global.LASTFEED={};
  ok(cpLevels('SPY')===null,'no feed, no levels');
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
