// Issue C regression: continuous SMA-50 available at session open + chart-aligned.
// Extracts the pure SMA functions from v10.js and drives them with a synthetic
// STATE that has only a FEW today bars but a full multi-session continuous series.
const fs = require('fs');
const src = fs.readFileSync('./v10.js','utf8');

// ---- minimal globals the extracted fns reference ----
global.CFG = { trendMA:{SPY:'50',QQQ:'50'}, trendOn:true };
global.TREND_WINDOW = 20; global.TREND_DOM = 16; global.TREND_BANDX = 0.25;
global.TREND_LAST = { SPY:null }; global.SMA_CONT_FLAG = { SPY:false };
function mul(a,b){ return a/(1/b); }
global.mul = mul;

// ---- extract the functions we need by name ----
function extract(name){
  const re = new RegExp('function\\s+'+name+'\\s*\\(', 'g');
  const m = re.exec(src);
  if(!m) throw new Error('not found: '+name);
  let i = src.indexOf('{', m.index), depth=0, end=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')depth++; else if(src[k]==='}'){depth--; if(depth===0){end=k;break;}} }
  return src.slice(m.index, end+1);
}
const code = ['contSMA','contSMAAtTodayIdx','todayOnlySMA','smaVal','closedCandles','atr','trendVerdict']
  .map(extract).join('\n');
eval(code);

// ---- build synthetic STATE ----
// 60 prior-session closes rising 700->759, then 5 today closes ~760-764.
let cont = [];
for(let i=0;i<60;i++) cont.push({ c:700+i, day:'2026-08-11', so:36000+i*180 });
const today = [];
for(let i=0;i<5;i++){ const c=760+i; cont.push({ c, day:'2026-08-12', so:30600+i*180 }); today.push({ c, so:30600+i*180 }); }
global.STATE = { SPY:{ contCloses:cont, contPriorCount:60, candles:today, price:764 } };

let pass=0, fail=0;
function ok(name, cond, got){ if(cond){pass++; console.log('PASS '+name+(got!==undefined?' -> '+got:''));} else {fail++; console.log('FAIL '+name+(got!==undefined?' -> got '+got:''));} }

// 1) continuous SMA-50 exists even though only 5 TODAY bars (would be 'na' before the fix)
const s = smaVal('SPY',50);
ok('continuous SMA available with few today bars', s!=null, s);

// 2) equals hand-computed average of the LAST 50 continuous closes
const last50 = cont.slice(cont.length-50).reduce((a,x)=>a+x.c,0)/50;
ok('continuous SMA equals hand-computed avg', Math.abs(s-last50)<1e-6, s+' vs '+last50);

// 3) it is NOT the today-only average (proves it uses prior sessions)
const todayAvg = today.reduce((a,x)=>a+x.c,0)/today.length;
ok('continuous SMA != today-only avg', Math.abs(s-todayAvg)>1, 'cont='+s+' todayOnly='+todayAvg);

// 4) trendVerdict is no longer "na" with only 5 today bars (continuous history present)
const tv = trendVerdict('SPY');
ok('trendVerdict not "na" early (was the "need more bars" bug)', tv.state!=='na', tv.state);

// 5) sanity fallback: corrupt continuous series (absurdly low) with a FULL today
//    series available -> smaVal must fall back to the today-only SMA + set flag.
const today50 = [];
for(let i=0;i<55;i++) today50.push({ c:760+(i%5), so:30600+i*180 });  // 55 today bars ~760-764
global.STATE.SPY.candles = today50;
global.STATE.SPY.contCloses = cont.map(x=>({...x, c:x.c-500})); // ~200s, spot 764 -> >5% off
global.SMA_CONT_FLAG.SPY = false;
const todayOnly50 = today50.slice(today50.length-50).reduce((a,x)=>a+x.c,0)/50;
const s2 = smaVal('SPY',50);
ok('sanity fallback returns today-only when continuous is absurd', Math.abs(s2-todayOnly50)<1e-6, 's2='+s2+' todayOnly='+todayOnly50);
ok('sanity fallback sets SMA_CONT_FLAG', global.SMA_CONT_FLAG.SPY===true, global.SMA_CONT_FLAG.SPY);

console.log(fail===0 ? '\nALL PASS' : '\n'+fail+' FAILED');
process.exit(fail===0?0:1);
