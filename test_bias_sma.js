// (v11.36) THE 50-SMA IS THE DIRECTION; everything else confirms it.
//
// The six-vote tally printed NEUTRAL on a 2-2 split even when the SMA was plainly sloped — the panel
// arguing with the chart. It also let PATH, MASS and VEX vote on direction, and all three are gamma-
// family reads: conditional, not directional. Confirmation is the more useful job for the supplementary
// reads, because TREND alone measured 34% and "TREND with 3 of 3" is a different proposition.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

global.window={};
let TV={state:'dn'}, SK={dir:-1,err:null}, AC={dir:-1}, PA={ok:true,dir:-1}, DR={verdict:'AGREE-DN',label:'DN·conf',overlap:true};
// (v11.88) PA no longer votes; CROSS (the other index) and ROLL (King migration) do.
let CX={ok:true,dir:-1,self:-1,same:true}, KR={ok:true,dir:-1};
global.trendVerdict=()=>TV;
global.trendWindowRead=()=>({win:20, up:4});
global.skewRead=()=>SK; global.accumAsym=()=>AC; global.paRead=()=>PA; global.driftRead=()=>DR;
global.crossRead=()=>CX; global.kingRollRead=()=>KR; global.kingRoll=()=>KR.dir;
eval(ex('biasVotes'));

// ---- the SMA decides, always ----
{ TV={state:'dn'}; const B=biasVotes('SPY');
  eq(B.dir,-1,'a down SMA is a BEARISH call'); eq(B.verdict,'BEARISH','named'); }
{ TV={state:'up'}; const B=biasVotes('SPY');
  eq(B.dir,1,'an up SMA is BULLISH'); }
{ TV={state:'flat'}; const B=biasVotes('SPY');
  eq(B.dir,0,'a flat SMA has no side'); eq(B.verdict,'FLAT','and says FLAT rather than inventing one'); }

// ---- the confirmers NEVER outvote it ----
{
  TV={state:'up'}; SK={dir:-1,err:null}; AC={dir:-1}; CX={ok:true,dir:-1}; KR={ok:true,dir:-1};
  const B=biasVotes('SPY');
  eq(B.verdict,'BULLISH','every confirmer disagreeing does NOT flip the call — that was the old tally\'s failure');
  eq(B.nConf,0,'they are simply recorded as not confirming');
}
{
  TV={state:'flat'}; SK={dir:1,err:null}; AC={dir:1}; CX={ok:true,dir:1}; KR={ok:true,dir:1};
  const B=biasVotes('SPY');
  eq(B.verdict,'FLAT','and agreeing confirmers cannot manufacture a direction the SMA does not have');
  eq(B.nConf,0,'with no side, nothing counts as confirming');
}
// ---- the count is the confidence ----
{
  TV={state:'dn'}; SK={dir:-1,err:null}; AC={dir:-1}; CX={ok:true,dir:-1}; KR={ok:true,dir:-1};
  const B=biasVotes('SPY');
  eq(B.nConf,4,'four of four confirm'); eq(B.confirms.length,4,'and there are exactly four confirmers');
  eq(B.confirms.map(c=>c.k),['SKEW','ACCUM','CROSS','ROLL'],
     'SKEW, ACCUM, CROSS, ROLL — PA is gone (correlated with the SMA it confirms) and no gamma-family read votes on direction');
  ok(!/conf\('PA'/.test(ex('biasVotes')),'PA is not a confirm any more');
  ok(/out\.pa=pa/.test(ex('biasVotes')),'but it is still computed and carried, so it can be recorded in shadow');
}
{
  TV={state:'dn'}; SK={dir:1,err:null}; AC={dir:-1}; CX={ok:true,dir:0}; KR={ok:true,dir:-1};
  const B=biasVotes('SPY');
  eq(B.nConf,2,'two confirm, one contradicts, one is flat');
  eq(B.nLive,4,'all four had an opinion to give — flat is an opinion, absent is not');
}
// ---- absent inputs are absent, not zero ----
{
  TV={state:'dn'}; SK={err:'they do not expose a skew value in the payload'}; AC=null;
  CX={ok:false,dir:0,why:'QQQ series too short'}; KR={ok:false,dir:null,why:'need 3 King samples, have 1'};
  const B=biasVotes('SPY');
  eq(B.nLive,0,'an unavailable read does not silently count as neutral');
  eq(B.nConf,0,'nor as confirming');
  eq(B.confirms.map(c=>c.d),[null,null,null,null],'each is null, so the face can show a dash rather than a tick');
  // ⚠ kingRoll() returns 0 for BOTH "the King has not moved" and "no King history". Once ROLL votes,
  // that 0 counts as a live neutral confirm and inflates nLive. kingRollRead separates them.
  KR={ok:true,dir:0};
  eq(biasVotes('SPY').nLive,1,'a King that genuinely has NOT moved is live-and-neutral, which is not the same as absent');
}
// ---- the call explains itself from the SMA ----
{
  TV={state:'dn'}; const B=biasVotes('SPY');
  ok(/50-SMA/.test(B.why),'the reason names the 50-SMA',B.why);
  ok(/4 of 20/.test(B.why),'and how many bars are on which side',B.why);
}
// ---- drift is carried, and it is a gate not a fourth confirmer ----
{
  TV={state:'dn'}; const B=biasVotes('SPY');
  ok(B.drift && B.drift.verdict==='AGREE-DN','drift is read');
  ok(B.confirms.every(c=>c.k!=='DRIFT'),'but it is NOT one of the confirmation chips — gamma gates, it does not vote');
}
// ---- gamma-family reads are gone from the tally entirely ----
{
  const fn=ex('biasVotes');
  ok(!/'PATH'/.test(fn) && !/'MASS'/.test(fn) && !/'VEX'/.test(fn),
     'PATH, MASS and VEX no longer appear as votes — gamma is conditional, not directional');
}

// ---------- (v11.88) THE THREE THINGS THAT SURVIVED MUTATION UNTIL THEY WERE EXECUTED ----------
// B3/B4/B5 in the mutation sweep each fired ZERO assertions: cross could compare a symbol against
// ITSELF, the colour rule could go back to a hardcoded `>=3`, and the tally could stop recording PA —
// all silently. Pattern 8: a test that greps cannot see any of that. These run the code.
{
  // --- the colour rule must not know the denominator ---
  eval(ex('confColour'));
  eq(confColour(4,4),'#2ec27e','all four confirming is green');
  eq(confColour(3,3),'#2ec27e','and so is all THREE — green cannot depend on the list being 4 long');
  eq(confColour(2,2),'#2ec27e','or all two');
  eq(confColour(0,4),'#f0616d','nothing confirming is red');
  eq(confColour(2,4),'#8b98a9','a partial count is neither');
  eq(confColour(0,0),'#8b98a9','nothing live has no verdict to colour — NOT red');
  ok(!/>=\s*3/.test(ex('confColour')),'and no hardcoded count survives in it');
}
{
  // --- the tally record must carry PA even though PA no longer votes ---
  TV={state:'dn'}; SK={dir:-1,err:null}; AC={dir:-1}; CX={ok:true,dir:-1,self:-1,same:true}; KR={ok:true,dir:-1};
  PA={ok:true,dir:-1};
  eval(ex('biasConfirmRecord'));
  const R=biasConfirmRecord('SPY');
  eq(R.ok,true,'the tally records');
  eq(R.nConf,4,'with the live count'); eq(R.tier,'all','and a tier a when-clause can match');
  eq(R.skew,-1,'skew'); eq(R.accum,-1,'accum'); eq(R.cross,-1,'cross'); eq(R.roll,-1,'roll');
  eq(R.pa,-1,                'PA IS RECORDED though it does not vote — a removal you cannot measure is a preference, not a decision');
  eq(R.paWouldConfirm,1,     'and whether it WOULD have confirmed, which is the question bias_pa_shadow asks');
  PA={ok:true,dir:1};
  eq(biasConfirmRecord('SPY').paWouldConfirm,0,'0 when it would have disagreed — the field spans both answers');
  PA={ok:false};
  eq(biasConfirmRecord('SPY').pa,null,'null when PA itself is unavailable');
}

// ⚠ THIS BLOCK RUNS LAST, DELIBERATELY. `eval(ex('crossRead'))` creates a LOCAL binding that shadows
// the `global.crossRead` stub every biasVotes test above depends on — and a shadowed stub fails
// SILENTLY, as a null read that looks like an unavailable feed. Anything that eval's the real
// implementation of something the other tests stub must come after them.
{
  // --- CROSS must read the OTHER instrument, never this one ---
  const SER={ SPY:[], QQQ:[] };
  const need=150+60;
  for(let i=0;i<need+5;i++){ SER.SPY.push(100+i); SER.QQQ.push(200-i); }   // SPY rising, QQQ falling
  global.LASTFEED={ SPY:{j:{levels:SER.SPY.map(v=>({s:v,t:1}))}},
                    QQQ:{j:{levels:SER.QQQ.map(v=>({s:v,t:1}))}} };
  global.CROSS_PAIR={SPY:'QQQ',QQQ:'SPY'};
  global.CROSS_MA_MIN=150; global.CROSS_WIN_MIN=60; global.CROSS_DOM=0.75;
  eval(ex('spotSeries')); eval(ex('snapTrend')); eval(ex('crossRead'));
  eq(snapTrend('SPY').dir, 1,'a rising series trends up by the shared rule');
  eq(snapTrend('QQQ').dir,-1,'a falling one trends down');
  const CR=crossRead('SPY');
  eq(CR.other,'QQQ',                 'CROSS on SPY reads QQQ');
  eq(CR.dir,-1,                      'and reports the OTHER instrument\'s direction, not its own — comparing a symbol against itself would always agree');
  eq(CR.self,1,                      'while still measuring THIS symbol by the identical rule, so the pair is comparable');
  eq(CR.same,false,                  'and says plainly that they disagree');
  // --- THE DOMINANCE THRESHOLD, on a fixture that can tell it from a simple majority ---
  // ⚠ The rising/falling series above CANNOT: 60 of 60 above the mean satisfies both `up>dn` and the
  // 75% rule, so dropping the threshold entirely would pass. This one sits between the two: 35 of 60
  // above is a majority and is NOT dominance, and 75% is what TREND_DOM means at 15-of-20.
  {
    const flat=[]; for(let i=0;i<150;i++) flat.push(100);
    const tail=[]; for(let i=0;i<60;i++) tail.push(i<35 ? 100.5 : 99.5);   // 35 up / 25 dn = 58%
    global.LASTFEED.QQQ={j:{levels:flat.concat(tail).map(v=>({s:v,t:1}))}};
    const T=snapTrend('QQQ');
    eq(T.up,35,'35 of the 60 sit above their own SMA'); eq(T.dn,25,'25 below');
    eq(T.dir,0,'a 58% majority is NOT a trend — the 75% dominance rule is what makes it a call, and a plain up>dn would have said UP here');
  }
  // a series too short must ABSTAIN, never return 0
  global.LASTFEED.QQQ={j:{levels:[{s:1,t:1},{s:2,t:1}]}};
  const CS=crossRead('SPY');
  ok(CS.ok===false && /too short/.test(CS.why||''),'a short series abstains with a reason instead of voting neutral', CS);
}

console.log('\n'+pass+' pass / '+fail+' fail');
