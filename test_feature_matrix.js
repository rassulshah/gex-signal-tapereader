// (v10.55 PART F) ENGINE-READY DATA. The panel already recorded everything; it recorded
// it in a shape no model can read — one queue per feature, outcomes hanging off each
// record. buildFeatureMatrix flattens that into ONE ROW PER BAR PER SYMBOL: every
// recorded input and vote as `feature.field` columns, the regime / session / model stamp
// beside them, and the four labels at the end. Plus the new context predictors, which
// are recorded and vote on nothing.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

var LS={};
global.localStorage={ getItem:function(k){ return (k in LS)?LS[k]:null; }, setItem:function(k,v){ LS[k]=String(v); } };
global.TODAY='2026-08-17';
global.CANDLE_MS=180000; global.FEAT_FWD=10; global.DIR_PTS=0.5;
global.ctTodayStr=function(){ return '2026-08-17'; };
global.ctMinutesSinceMidnight=function(){ return 13*60+45; };   // 13:45 CT
global.mul=function(a,b){ return a/(1/b); };
global.STATE={ SPY:{ price:772.6, candles:[], contCloses:[{c:775.2, day:'2026-08-14'},{c:773.1, day:'2026-08-17'}] }, QQQ:{ price:null, candles:[] } };
global.closedCandles=function(){ return [ {o:775,h:775.4,l:774.6,c:774.8}, {o:774.8,h:775,l:772.2,c:772.6} ]; };
global.nodeMapModel=function(){ return { ok:true, px:772.6, kingK:773, levels:[] }; };
global.legEngine=function(){ return { dir:'dn', phase:'PB', magnet:{k:773,pct:100,isKing:true}, lastPB:{k:775},
   pbDetected:{k:775, rolledFrom:775.5, step:3}, predictedPB:false,
   roll:{ side:'ceil', steps:[776,775.5,775], count:3, signal:true, confirmed:true, weakening:false },
   pbZone:{lo:772.6,hi:775}, invalidations:{trendBreak:false,pbBreak:false} }; };
global.recorderLoad=function(){ return { days:{} }; };
global.recorderDay=function(){ return {}; };

eval(['buildFeatureMatrix','eventTagLoad','eventTagSet','eventTagNow','eventTagLabel','registerFeature','featureByKey'].map(ex).join('\n'));
global.FEATURES=[]; global.EVENT_TAG=''; global.EVENT_KEY='gpts_event_v1';

// ---------------- a two-bar recorded day, exactly as featEnqueue writes it ---------
var DAY={ feat:{ SPY:[
  { key:'dir', t:1000, bar:1, n:20, px:773.4, session:'afternoon', hit:1, mfe:0.9, mae:-0.2,
    frame:{ tgtHit:true, invalHit:false, first:'tgt', rr:2.4 }, partial:false, resolved:true,
    rec:{ grade:'B', verdict:'DN', score:4, relation:'confirmed', trendState:'dn', tgt:773, inval:775.4,
          regime:{tag:'trend',opex:false,event:true}, model:{rulesAsOf:'2026-08-17', weightsHash:'abc123'} } },
  { key:'node', t:1000, bar:1, n:20, px:773.4, session:'afternoon', hit:0, mfe:0.9, mae:-0.2, resolved:true,
    rec:{ k:775, grade:'A', score:5, pol:'+', tap:1, holdDir:-1, regime:{tag:'trend',opex:false,event:true} } },
  { key:'leg.roll', t:1000, bar:1, n:20, px:773.4, session:'afternoon', hit:1, mfe:0.9, mae:-0.2, resolved:true,
    rec:{ count:3, side:'ceil', confirmed:true, signal:true, weakening:false, dir:'dn', vote:-1, steps:[776,775.5,775],
          session:{ sessions:4, ready:true, vote:-1, flr:0, ceil:3 }, regime:{tag:'trend',opex:false,event:true} } },
  { key:'predictors', t:1000, bar:1, n:20, px:773.4, session:'afternoon', hit:null, mfe:0.9, mae:-0.2, resolved:true,
    rec:{ timeToClose:75, barOfDay:20, distToKing:0.4, distToMagnet:0.4, pbActive:true, rollCount:3,
          sessionRangePos:0.12, dayNet:-2.4, pdc:775.2, pdcRel:'below', event:'FOMC',
          regime:{tag:'trend',opex:false,event:true} } },
  { key:'dir', t:2000, bar:2, n:21, px:772.9, session:'afternoon', hit:0, mfe:0.2, mae:-0.8, resolved:true,
    rec:{ grade:'C', verdict:'DN', score:2, relation:'divergence', trendState:'dn',
          regime:{tag:'chop',opex:false,event:false}, model:{rulesAsOf:'2026-08-17', weightsHash:'abc123'} } }
] } };

var M=buildFeatureMatrix(DAY);

// ================= 1. ONE ROW PER BAR PER SYMBOL ==================================
ok(M.length===2, '1a four features across two bars collapse into TWO rows', M.length);
ok(M[0].sym==='SPY' && M[0].bar===1 && M[1].bar===2, '1b keyed by (sym, bar) and sorted', M[0].sym+'/'+M[0].bar);
ok(M[0].px===773.4 && M[0].n===20 && M[0].session==='afternoon', '1c the bar\'s own identity rides along');

// ================= 2. EVERY RECORDED FIELD IS A COLUMN ============================
ok(M[0]['dir.grade']==='B' && M[0]['dir.relation']==='confirmed' && M[0]['dir.score']===4,
   '2a the direction record is flattened to dir.* columns', M[0]['dir.grade']);
ok(M[0]['node.k']===775 && M[0]['node.pol']==='+' && M[0]['node.tap']===1, '2b the node record too');
ok(M[0]['leg.roll.count']===3 && M[0]['leg.roll.confirmed']===true && M[0]['leg.roll.vote']===-1,
   '2c ...and the leg engine, votes included', M[0]['leg.roll.count']);
ok(M[0]['leg.roll.session.sessions']===4 && M[0]['leg.roll.session.ready']===true,
   '2d nested objects flatten one level deeper rather than being dropped', M[0]['leg.roll.session.sessions']);
ok(M[0]['predictors.timeToClose']===75 && M[0]['predictors.pdcRel']==='below',
   '2e the context predictors are columns like everything else');
ok(!('leg.roll.steps' in M[0]) || Array.isArray(M[0]['leg.roll.steps'])===false,
   '2f array fields are not smeared across columns');

// ================= 3. REGIME / SESSION / MODEL STAMP ==============================
ok(M[0].regime==='trend' && M[1].regime==='chop', '3a the regime tag is its own column, per bar', M[0].regime);
ok(M[0].opex===false && M[0].event===1, '3b opex and the EVENT flag are separated out', M[0].event);
ok(M[0].weightsHash==='abc123' && M[0].rulesAsOf==='2026-08-17',
   '3c WHICH model scored the bar is stamped on the row', M[0].weightsHash);

// ================= 4. THE FOUR LABELS ============================================
ok(M[0].dirHit===1 && M[1].dirHit===0, '4a dirHit — did the direction travel', M[0].dirHit);
ok(M[0].deflHit===0, '4b deflHit — did the node hold', M[0].deflHit);
ok(M[0].contHit===1, '4c contHit — did the roll continue', M[0].contHit);
ok(M[0].tgtHit===1, '4d tgtHit — the frame\'s target before its invalidation', M[0].tgtHit);
ok(M[0].mfe===0.9 && M[0].mae===-0.2, '4e ...with MFE/MAE beside them', M[0].mfe);
ok(M[0].partial===0, '4f and whether the forward window was cut short');

// ================= 5. IT RIDES IN THE DAY EXPORT ==================================
var BDE=ex('buildDayExport');
ok(/matrix:\(function\(\)\{ try\{ return buildFeatureMatrix\(day\)/.test(BDE), '5a the export carries `matrix`');
ok(/matrixLegend/.test(BDE), '5b ...with a legend saying what the columns and labels are');
ok(/nodeHist:/.test(BDE) && /sessionRoll:/.test(BDE), '5c ...plus the node-cluster history and the multi-session roll');
ok(/futures:/.test(BDE) && /event:\(function/.test(BDE), '5d ...and which chart / event tag the day was recorded under');
ok(buildFeatureMatrix({})+''==='' , '5e an empty day yields an empty matrix, never a throw');
ok(buildFeatureMatrix(null).length===0, '5f ...and so does no day at all');

// ================= 6. THE EVENT TAG IS REAL NOW ==================================
ok(eventTagNow()===false, '6a with nothing set, the answer is still an honest false');
eventTagSet('FOMC');
ok(eventTagNow()===true && eventTagLabel()==='FOMC', '6b the ⚙ tag makes it true, and NAMES the event', eventTagLabel());
eventTagSet('');
ok(eventTagNow()===false, '6c clearing it clears the flag');
ok(/gpts_event_v1/.test(src), '6d stored under a NEW key, renaming nothing');
ok(/class="gpts-event"/.test(src), '6e and it is settable from the ⚙ drawer');
ok(/never a guess/.test(ex('eventTagNow')), '6f the honesty rule is written into the function');

// ================= 7. THE PREDICTORS VOTE ON NOTHING =============================
var PRED=src.slice(src.indexOf("key:'predictors'"), src.indexOf("key:'predictors'")+3000);
ok(/voting:false/.test(PRED), '7a the predictors record declares itself non-voting');
ok(/hit:null/.test(PRED), '7b ...and returns a null outcome: they are inputs, not claims');
['timeToClose','barOfDay','distToKing','distToMagnet','pbActive','rollCount','sessionRangePos','dayNet','pdc','event']
  .forEach(function(f){ ok(new RegExp(f).test(PRED), '7·'+f+' is recorded'); });

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
