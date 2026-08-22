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
global.trendVerdict=()=>TV;
global.trendWindowRead=()=>({win:20, up:4});
global.skewRead=()=>SK; global.accumAsym=()=>AC; global.paRead=()=>PA; global.driftRead=()=>DR;
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
  TV={state:'up'}; SK={dir:-1,err:null}; AC={dir:-1}; PA={ok:true,dir:-1};
  const B=biasVotes('SPY');
  eq(B.verdict,'BULLISH','all three confirmers disagreeing does NOT flip the call — that was the old tally\'s failure');
  eq(B.nConf,0,'they are simply recorded as not confirming');
}
{
  TV={state:'flat'}; SK={dir:1,err:null}; AC={dir:1}; PA={ok:true,dir:1};
  const B=biasVotes('SPY');
  eq(B.verdict,'FLAT','and three agreeing confirmers cannot manufacture a direction the SMA does not have');
  eq(B.nConf,0,'with no side, nothing counts as confirming');
}
// ---- the count is the confidence ----
{
  TV={state:'dn'}; SK={dir:-1,err:null}; AC={dir:-1}; PA={ok:true,dir:-1};
  const B=biasVotes('SPY');
  eq(B.nConf,3,'three of three confirm'); eq(B.confirms.length,3,'and there are exactly three confirmers');
  eq(B.confirms.map(c=>c.k),['SKEW','ACCUM','PA'],'SKEW, ACCUM, PA — no gamma-family read votes on direction');
}
{
  TV={state:'dn'}; SK={dir:1,err:null}; AC={dir:-1}; PA={ok:true,dir:0};
  const B=biasVotes('SPY');
  eq(B.nConf,1,'one confirms, one contradicts, one is flat');
  eq(B.nLive,3,'all three had an opinion to give');
}
// ---- absent inputs are absent, not zero ----
{
  TV={state:'dn'}; SK={err:'they do not expose a skew value in the payload'}; AC=null; PA={ok:false};
  const B=biasVotes('SPY');
  eq(B.nLive,0,'an unavailable read does not silently count as neutral');
  eq(B.nConf,0,'nor as confirming');
  eq(B.confirms.map(c=>c.d),[null,null,null],'each is null, so the face can show a dash rather than a tick');
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
console.log('\n'+pass+' pass / '+fail+' fail');
