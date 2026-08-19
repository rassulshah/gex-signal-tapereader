// (v11.1.2) THE FEATURE QUEUE MUST SURVIVE THE SESSION. `bar` is a millisecond timestamp;
// the v10.54 cutoff `maxBar-FEAT_KEEP_BARS` was 160 ms and deleted every prior bar on each
// enqueue, so nothing ever resolved. This test enqueues many bars and checks that the queue
// keeps FEAT_KEEP_BARS distinct bars, that resolution then works, and that the quota
// fallback never deletes today.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

var LS={}; var QUOTA=Infinity;
global.localStorage={ getItem:function(k){ return (k in LS)?LS[k]:null; },
  setItem:function(k,v){ v=String(v); if(v.length>QUOTA) throw new Error('QuotaExceededError'); LS[k]=v; } };
global.window={ __gptsDebug:{} };
global.TODAY='2026-08-19';
global.RECORDER_KEY='gpts_recorder_v7'; global.RECORDER_DAYS=10; global.RECORDER_SYMS=['SPY','QQQ'];
global.FEAT_KEEP_BARS=160; global.FEAT_FWD=10; global.DIR_PTS=0.5;
global.FEATURES=[]; global.FEAT_ARCHIVE={};
global.STATE={SPY:{price:770,candles:[]},QQQ:{}};
global.repoUpsertFeat=function(){}; global.featStatsInvalidate=function(){};
global.nodeMapModel=function(){ return {ok:true,kingK:771}; };
global.ctNowSecOfDay=function(){ return 11*3600; };
global.frameOutcome=function(){ return null; };
eval(['recorderLoad','recorderSave','recorderDay','featEnqueue','registerFeature','featureByKey','_fwdHitDir','_fwdHitNum','resolveFeatureOutcomes'].map(ex).join('\n'));
global.registerCoreFeatures=function(){ return FEATURES; };
// two tiny features
registerFeature({ key:'t.up', label:'t', phase:'x', fwd:10, record:function(){ return {verdict:'UP'}; },
  outcome:function(rec,fwd){ return {hit:_fwdHitDir(fwd,'UP')}; }, questions:[], rule:{id:'t.up'} });
registerFeature({ key:'t.dn', label:'t', phase:'x', fwd:10, record:function(){ return {verdict:'DN'}; },
  outcome:function(rec,fwd){ return {hit:_fwdHitDir(fwd,'DN')}; }, questions:[], rule:{id:'t.dn'} });

// ---- 1. enqueue 200 bars with REAL ms bar stamps, 3 minutes apart
var t0=1787060160000;
for(var i=0;i<200;i++){
  var bar=t0+i*180000;
  featEnqueue('SPY', {'t.up':{verdict:'UP'},'t.dn':{verdict:'DN'}}, {t:bar+2000, bar:bar, n:i+1, px:770, session:{bucket:'morning'}});
}
var arr=recorderDay(recorderLoad()).feat.SPY;
var bars={}; arr.forEach(function(r){ bars[r.bar]=1; });
ok(Object.keys(bars).length===160, '1a after 200 bars the queue holds exactly FEAT_KEEP_BARS=160 distinct bars (was 1 before the fix)', Object.keys(bars).length);
ok(arr.length===320, '1b ...= 160 bars x 2 features records', arr.length);
var minBar=Math.min.apply(null,Object.keys(bars).map(Number));
ok(minBar===t0+40*180000, '1c the 40 oldest bars were the ones dropped', (minBar-t0)/180000);

// ---- 2. a fresh day: 30 bars enqueued, then candles arrive → early bars resolve
LS={}; STATE.SPY.candles=[];
for(var j=0;j<30;j++){
  var b2=t0+j*180000;
  STATE.SPY.candles.push({b:b2,o:770,h:770.2,l:769.8,c:770});
  featEnqueue('SPY', {'t.up':{verdict:'UP'},'t.dn':{verdict:'DN'}}, {t:b2+2000, bar:b2, n:j+1, px:770});
}
// price runs up 0.8 over the last 10 candles → the bars recorded 10+ candles ago resolve
for(var q=0;q<10;q++){ STATE.SPY.candles.push({b:t0+(30+q)*180000,o:770,h:770.8,l:769.9,c:770.6}); }
var changed=resolveFeatureOutcomes('SPY');
var a2=recorderDay(recorderLoad()).feat.SPY;
var res=a2.filter(function(r){ return r.resolved; });
ok(changed>0 && res.length>0, '2a with the queue intact, records DO resolve once their 10-bar window closes', changed);
ok(a2.length===60, '2b nothing was pruned below the cap (30 bars x 2 = 60 records)', a2.length);
var upHits=res.filter(function(r){ return r.key==='t.up'; }).map(function(r){ return r.hit; });
var lateUp=res.filter(function(r){ return r.key==='t.up' && r.n>=21; }).map(function(r){ return r.hit; });
var earlyUp=res.filter(function(r){ return r.key==='t.up' && r.n<21; }).map(function(r){ return r.hit; });
ok(lateUp.length===10 && lateUp.every(function(h){ return h===1; }), '2c UP records whose window reaches the +0.8 run resolve hit=1', lateUp.join(''));
ok(earlyUp.length===20 && earlyUp.every(function(h){ return h===0; }), '2c2 UP records whose window was flat resolve hit=0 (not null, not pruned)', earlyUp.join(''));
var dnHits=res.filter(function(r){ return r.key==='t.dn'; }).map(function(r){ return r.hit; });
ok(dnHits.length && dnHits.every(function(h){ return h===0; }), '2d DN records over the same run resolve hit=0', dnHits.join(''));

// ---- 3. quota fallback never deletes TODAY
LS={}; QUOTA=Infinity;
var db=recorderLoad(); var day=recorderDay(db); day.feat={SPY:[]};
for(var k=0;k<40;k++) day.feat.SPY.push({key:'t.up',bar:t0+k*180000,rec:{pad:'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'}});
var bigPad=new Array(400).join('y');
db.days['2026-08-18']={date:'2026-08-18',snaps:{SPY:[{pad:bigPad},{pad:bigPad},{pad:bigPad},{pad:bigPad},{pad:bigPad}]},events:{}};
recorderSave(db);
var full=LS[RECORDER_KEY].length;
QUOTA=full;                                     // the next save (one record longer) will overflow; dropping 08-18 makes room
day.feat.SPY.push({key:'t.up',bar:t0+40*180000,rec:{pad:'zzzz'}});
recorderSave(db);
var db3=recorderLoad();
ok(!!db3.days[TODAY], '3a on quota, TODAY survives', Object.keys(db3.days).join(','));
ok(!db3.days['2026-08-18'], '3b ...the OLDER day was the one dropped');
// today alone, still over quota → the oldest half of the queue is shed, today stays
QUOTA=Math.floor(LS[RECORDER_KEY].length*0.8);   // today alone; shedding half the queue makes room
db3.days[TODAY].feat.SPY.push({key:'t.up',bar:t0+41*180000,rec:{pad:'w'}});
recorderSave(db3);
var db4=recorderLoad();
ok(!!db4.days[TODAY] && db4.days[TODAY].snaps, '3c today alone + quota: today (and its snaps) still there');
ok(db4.days[TODAY].feat.SPY.length<42 && db4.days[TODAY].feat.SPY.length>=21, '3d ...only the oldest half of the feature queue was shed', db4.days[TODAY].feat.SPY.length);
ok(db4.days[TODAY].feat.SPY[db4.days[TODAY].feat.SPY.length-1].bar===t0+41*180000, '3e ...newest record kept');

console.log('test_feat_keep: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
