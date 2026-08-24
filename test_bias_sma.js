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
  eq(B.dir,-1,'a down SMA is a BEARISH call'); eq(B.verdict,'DNTREND','named — v11.95 names the MACHINE STATE, not a mood'); }
{ TV={state:'up'}; const B=biasVotes('SPY');
  eq(B.dir,1,'an up SMA is BULLISH'); }
{ TV={state:'flat'}; const B=biasVotes('SPY');
  eq(B.dir,0,'a flat SMA has no side'); eq(B.verdict,'FLAT','and says FLAT rather than inventing one'); }

// (v11.95) FOUR STATES, NOT TWO. The 50-SMA machine has five and the face collapsed four into two.
{
  TV={state:'up',up:17,dn:1,win:20};        eq(biasVotes('SPY').verdict,'UPTREND','up is UPTREND');
  TV={state:'dn',up:1,dn:18,win:20};        eq(biasVotes('SPY').verdict,'DNTREND','dn is DNTREND');
  TV={state:'up-broken',up:13,dn:5,win:20}; eq(biasVotes('SPY').verdict,'UPTREND BRK','a broken uptrend says so');
  TV={state:'dn-broken',up:6,dn:12,win:20}; eq(biasVotes('SPY').verdict,'DNTREND BRK','and a broken downtrend');
  TV={state:'flat',up:6,dn:6,win:20};       eq(biasVotes('SPY').verdict,'FLAT','no side is FLAT');
  // a BROKEN trend still carries the side it broke FROM, or the confirms have nothing to agree with
  TV={state:'up-broken',up:13,dn:5,win:20}; eq(biasVotes('SPY').dir,1,'up-broken still leans up');
  TV={state:'dn-broken',up:6,dn:12,win:20}; eq(biasVotes('SPY').dir,-1,'dn-broken still leans down');
}
// (v11.95) THE GREY LINE COUNTS THE SIDE THE STATE IS ON. The old line always read `w.up` and only
// flipped the WORD, so a DNTREND could read "17 of 20 above".
{
  TV={state:'dn',up:2,dn:18,win:20};
  const B=biasVotes('SPY');
  ok(/18 of 20 bars below the 50-SMA/.test(B.why), 'a downtrend counts the bars BELOW', B.why);
  TV={state:'up',up:17,dn:2,win:20};
  ok(/17 of 20 bars above the 50-SMA/.test(biasVotes('SPY').why), 'an uptrend counts the bars ABOVE');
  TV={state:'up-broken',up:13,dn:5,win:20};
  const Bb=biasVotes('SPY');
  ok(/13 of 20 bars above/.test(Bb.why), 'a broken trend still counts its own side', Bb.why);
  ok(/lost 15, reversal needs 11/.test(Bb.why), 'and states what it lost and what a reversal now needs', Bb.why);
}
// (v11.95) BADGES READ THEIR OWN DIRECTION — up / down / sideways — not agreement with the call.
{
  const sb=ex('secBias');
  ok(/mark='↑'/.test(sb) && /mark='↓'/.test(sb) && /mark='→'/.test(sb),
     'the marks are arrows, not ticks and crosses');
  ok(!/mark='✓'/.test(sb) && !/mark='✗'/.test(sb), 'and the tick/cross marks are gone');
  ok(/cls=\(B\.dir>0\)\?' y'/.test(sb),
     'the COLOUR still carries agreement, so the confirm COUNT is unchanged — only the glyph moved');
}
// ---- the confirmers NEVER outvote it ----
{
  TV={state:'up'}; SK={dir:-1,err:null}; AC={dir:-1}; CX={ok:true,dir:-1}; KR={ok:true,dir:-1};
  const B=biasVotes('SPY');
  eq(B.verdict,'UPTREND','every confirmer disagreeing does NOT flip the call — that was the old tally\'s failure');
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
  // (v11.95) the line is built from tv.up / tv.dn / tv.win now, so the fixture must supply them —
  // it used to fall through to trendWindowRead and always report the UP count regardless of state.
  TV={state:'dn',up:4,dn:16,win:20}; const B=biasVotes('SPY');
  ok(/50-SMA/.test(B.why),'the reason names the 50-SMA',B.why);
  ok(/16 of 20 bars below/.test(B.why),'and how many bars are on the side the state is ON',B.why);
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


// ---------- (v11.90) DRIFT IS A BADGE NOW, AND IT STILL DOES NOT VOTE ----------
// Two decisions have to survive this: v11.44 made drift a GATE because its tick meant "the books agree
// with EACH OTHER" and the face read BULLISH beside DRIFT ✓ on a DOWN agreement; and the user shadowed
// drift on 2026-08-18 pending proof. Moving it onto the row is cosmetic — if it ever starts counting,
// these fail.
{
  const sb=ex('secBias');
  ok(!/class="g3gate"/.test(sb),      'the full-width gate row is gone');
  ok(/class="g3chip gate/.test(sb),   'DRIFT is a badge on the confirm row');
  ok(/g3sepv/.test(sb),               'behind a divider, so it does not read as one of the confirms');
  ok(/background:transparent!important/.test(src),
     'and the gate badge is OUTLINED, never filled — a filled one would look like a fifth confirm');

  // the count must still exclude it
  TV={state:'dn'}; SK={dir:-1,err:null}; AC={dir:-1}; CX={ok:true,dir:-1}; KR={ok:true,dir:-1};
  DR={verdict:'AGREE-DN',label:'DN·conf',overlap:true,dir:-1};
  const B=biasVotes('SPY');
  eq(B.confirms.length,4,'four confirms');
  eq(B.nConf,4,'all four agree');
  ok(!B.confirms.some(c=>c.k==='DRIFT'),'and DRIFT is NOT among them — it is a gate, not a vote', B.confirms.map(c=>c.k));

  // v11.44's actual bug: books agreeing with EACH OTHER on the wrong side must not show a tick
  ok(/withCall===false/.test(sb) && /against the call/.test(sb),
     'a DOWN agreement under an UP call is still marked against the call, which is the v11.44 fix');
  ok(/withCall===true/.test(sb),'and a tick needs agreement WITH the call, not merely between the books');
}
// ---------- (v11.90) EVERY BADGE HOVER LEADS WITH A QUESTION ----------
{
  const sb=ex('secBias'), bv=ex('biasVotes');
  const musts=[
    [bv,'Is protection being bought or sold?','SKEW'],
    [bv,'Which side of the book is growing?','ACCUM'],
    [bv,'Does the other index agree?','CROSS'],
    [bv,'Is the settlement magnet moving?','ROLL'],
    [sb,'Is anything structurally confirming the call?','DRIFT'],
    [sb,'How much conviction is behind this call?','the count'],
    [sb,'Which way, and on whose authority?','the verdict'],
  ];
  musts.forEach(m=>ok(m[0].indexOf(m[1])>=0, m[2]+' asks a question before it explains'));
  // and none of them still carries the stale three-confirm claim
  ok(!/Three of three/.test(sb),'the count hover no longer says "three of three" — there are four confirms now');
  ok(!/out of three secondary inputs/.test(sb),'nor does the verdict hover');
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

  // --- (v11.92) THE WARM-UP HORIZON ---
  // Live 2026-08-24: "SPY series too short (27 of 210 min)" at 09:56 — the full horizon needs 210
  // minutes and the spot series starts EMPTY at the open, so CROSS was silent until roughly 13:00 ET.
  global.CROSS_MA_SHORT=50; global.CROSS_WIN_SHORT=20;
  function ser(n, rising){ var a=[]; for(var i=0;i<n;i++) a.push(rising?100+i:100-i); return a; }
  function feed(spyN, qqqN){
    global.LASTFEED={ SPY:{j:{levels:ser(spyN,true).map(v=>({s:v,t:1}))}},
                      QQQ:{j:{levels:ser(qqqN,true).map(v=>({s:v,t:1}))}} };
  }
  feed(210+5, 210+5); eq(snapTrend('SPY').horizon,'full','210 minutes reaches the FULL horizon');
  feed(80, 80);       eq(snapTrend('SPY').horizon,'short','70 minutes falls back to the SHORT one, live about an hour into the session instead of three and a half');
  feed(60, 60);       ok(snapTrend('SPY').ok===false,'and below even that it abstains with a reason', snapTrend('SPY').why);
  feed(80,80);
  ok(snapTrend('SPY').dir===1,'the short horizon still produces a direction'); 
  ok(snapTrend('SPY').win===20 && snapTrend('SPY').ma===50,'and reports the window and average it actually used', [snapTrend('SPY').win, snapTrend('SPY').ma]);
  // ⚠ the two horizons are NOT the same measurement and must never be blended
  feed(215, 80);
  var MM=crossRead('SPY');
  ok(MM.ok===false && /horizon mismatch/.test(MM.why||''),
     'when the pair cannot reach the SAME horizon the read ABSTAINS — one side on 210 minutes and the other on 70 is not a like-for-like comparison',
     MM.why);
  feed(215,215); eq(crossRead('SPY').horizon,'full','matched horizons read normally');

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
