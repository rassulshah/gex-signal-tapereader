// (v11.34) ROLL DETECTION — mass moving between strikes, and the pop-out visibility gate.
//
// The user's definition: one node DISSIPATING while another on the same side ACCUMULATES. Measured in
// DOLLARS, because %King has a moving denominator — if the King strike changes every node rebases at once
// and a whole cluster reads as dissipating together, which is the most convincing possible false positive.
//
// Thresholds were MEASURED on 2026-08-21 (390 snapshots x 209 strikes), not guessed:
//   30m window · 50% RELATIVE TO THE SESSION MEDIAN · $40M floor · +-5 strikes  -> 9 events/session.
//   35% gave 37 (noise), 70% gave 2 (silence). The median near-money strike grows 10-15% per 30m on its
//   own, so an absolute threshold calls ordinary book-building "accumulation" all morning.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

global.window={}; global.document={visibilityState:'visible'};
global.STATE={SPY:{price:766.0}};
global.LASTFEED={SPY:null};
global.ROLL_WIN_MIN=30; global.ROLL_TH=0.50; global.ROLL_MIN_V=40e6; global.ROLL_REACH=5;
eval(ex('rollMedian')); eval(ex('rollDetect'));
eval(ex('pipOpen')); eval(ex('panelVisible'));
global.PIPWIN=null;

const T0=1787319000;
// build a feed whose strikes each follow their own trajectory over `n` minutes
function feed(traj, n){
  n=n||40;
  const snaps=[];
  for(let i=0;i<n;i++){
    const l=[];
    for(const k in traj){ const f=traj[k]; const v=f(i/(n-1));
      if(v!=null) l.push({k:parseFloat(k), v:v, d:1, net:v*0.5}); }
    snaps.push({t:T0+i*60, s:766.0, l:l});
  }
  return { j:{levels:snaps}, ts:Date.now() };
}
const flat=(v)=>()=>v;
const ramp=(a,b)=>(x)=>a+(b-a)*x;

// ---- the core case: a ceiling rolling down ----
{
  LASTFEED.SPY=feed({
    '768': ramp(300e6, 60e6),     // dissipating, above price
    '767': ramp(100e6, 320e6),    // accumulating, above price
    '765': flat(120e6), '764': flat(110e6), '763': flat(100e6)   // a stable baseline
  });
  const R=rollDetect('SPY');
  ok(!R.err,'a full session produces a read',R.err);
  ok(!!R.ceil,'a ceiling roll is detected');
  eq(R.ceil.from,768,'from the dissipating strike');
  eq(R.ceil.to,767,'to the accumulating one');
  ok(R.ceil.toward===true,'and it is flagged as moving TOWARD price');
  ok(R.ceil.fromPct<-50,'the origin lost more than the threshold',R.ceil.fromPct);
  ok(R.ceil.toPct>50,'and the destination gained more',R.ceil.toPct);
  ok(!R.flr,'nothing is claimed on the floor side');
}
// ---- a floor rolling up ----
{
  LASTFEED.SPY=feed({
    '764': ramp(300e6, 60e6),     // dissipating, below price
    '765': ramp(100e6, 320e6),    // accumulating, below price
    '768': flat(120e6), '769': flat(110e6), '770': flat(100e6)
  });
  const R=rollDetect('SPY');
  ok(!!R.flr,'a floor roll is detected');
  eq(R.flr.from,764,'from the dying strike');
  eq(R.flr.to,765,'to the growing one');
  ok(R.flr.toward===true,'flagged as moving TOWARD price');
}
// ---- THE MEDIAN SUBTRACTION: ordinary book-building is not a roll ----
// The median near-money strike grows 10-15% per 30m. Without removing that, every strike "accumulates"
// in the morning and "dissipates" into the close.
{
  LASTFEED.SPY=feed({
    '768': ramp(100e6, 160e6), '767': ramp(100e6, 160e6),
    '765': ramp(100e6, 160e6), '764': ramp(100e6, 160e6), '763': ramp(100e6, 160e6)
  });
  const R=rollDetect('SPY');
  ok(!R.ceil && !R.flr,'a book where EVERY strike grows 60% together produces no roll at all');
  ok(R.base>=35,'and the session baseline is reported as the large number it is — the 30m window covers part of the ramp, so it is the windowed rise, not the full one',R.base);
}
{
  // expiry decay: everything collapses together — also not a roll
  LASTFEED.SPY=feed({
    '768': ramp(300e6, 40e6), '767': ramp(300e6, 40e6),
    '765': ramp(300e6, 40e6), '764': ramp(300e6, 40e6), '763': ramp(300e6, 40e6)
  });
  const R=rollDetect('SPY');
  ok(!R.ceil && !R.flr,'and a book where every strike decays together — the expiry-day case — produces none either');
}
{
  // one node genuinely outrunning a rising tide IS a roll
  LASTFEED.SPY=feed({
    '768': ramp(300e6, 150e6),    // falling while the book rises = strongly negative relative
    '767': ramp(100e6, 400e6),
    '765': ramp(100e6, 130e6), '764': ramp(100e6, 130e6), '763': ramp(100e6, 130e6)
  });
  const R=rollDetect('SPY');
  ok(!!R.ceil,'a node moving against the tide is still caught once the baseline is removed');
}
// ---- guards ----
{
  LASTFEED.SPY=feed({ '768': ramp(300e6,60e6), '765':flat(120e6),'764':flat(110e6),'763':flat(100e6) });
  const R=rollDetect('SPY');
  ok(!R.ceil,'a node dissipating with NOTHING growing is a wall evaporating, not a roll — no arrow to nowhere');
}
{
  LASTFEED.SPY=feed({ '768': ramp(30e6,5e6), '767': ramp(5e6,30e6),
                      '765':flat(120e6),'764':flat(110e6),'763':flat(100e6) });
  ok(!rollDetect('SPY').ceil,'nodes under the $40M floor cannot produce a roll');
}
{
  LASTFEED.SPY=feed({ '790': ramp(300e6,60e6), '789': ramp(100e6,320e6),
                      '765':flat(120e6),'764':flat(110e6),'763':flat(100e6) });
  ok(!rollDetect('SPY').ceil,'a transfer beyond the reach window is out of scope');
}
{
  LASTFEED.SPY=feed({ '768': ramp(300e6,60e6), '764': ramp(100e6,320e6),
                      '765':flat(120e6),'763':flat(110e6),'762':flat(100e6) });
  const R=rollDetect('SPY');
  ok(!R.ceil && !R.flr,'a dissipating CEILING paired with an accumulating FLOOR is not a roll — pairing is same-side only');
}
{
  LASTFEED.SPY=feed({'768':flat(100e6)}, 40);
  ok(!!rollDetect('SPY').err,'too thin a book reports why rather than returning a phantom roll');
}
{
  LASTFEED.SPY=feed({'768':ramp(300e6,60e6),'767':ramp(100e6,320e6),'765':flat(120e6),'764':flat(110e6),'763':flat(100e6)}, 6);
  ok(!!rollDetect('SPY').err,'and too little session elapsed does the same');
}
{
  LASTFEED.SPY=null;
  ok(rollDetect('SPY')===null,'no feed at all is null, not an error object pretending to be a read');
}
// ---- the window is walked by TIME, not by index ----
{
  // half-cadence samples: 30 minutes must still mean 30 minutes
  const snaps=[];
  for(let i=0;i<80;i++){
    const x=i/79;
    snaps.push({t:T0+i*30, s:766.0, l:[
      {k:768, v:300e6+(60e6-300e6)*x, d:1, net:0},
      {k:767, v:100e6+(320e6-100e6)*x, d:1, net:0},
      {k:765, v:120e6, d:1, net:0},{k:764, v:110e6, d:1, net:0},{k:763, v:100e6, d:1, net:0}]});
  }
  LASTFEED.SPY={ j:{levels:snaps}, ts:Date.now() };
  const R=rollDetect('SPY');
  ok(!!R.ceil,'at double cadence the roll is still found — the window is walked by timestamp, not by index');
}
// ---- the pop-out visibility gate ----
{
  global.PIPWIN=null; document.visibilityState='visible';
  ok(panelVisible()===true,'a visible tab is visible');
  document.visibilityState='hidden';
  ok(panelVisible()===false,'a hidden tab with no pop-out is not');
  global.PIPWIN={closed:false};
  ok(panelVisible()===true,'but a hidden tab WITH the panel popped out counts as visible — otherwise both books quietly stop refreshing while the panel keeps showing its last values');
  global.PIPWIN={closed:true};
  ok(panelVisible()===false,'a closed pop-out no longer counts');
  global.PIPWIN=null; document.visibilityState='visible';
}
console.log('\n'+pass+' pass / '+fail+' fail');
