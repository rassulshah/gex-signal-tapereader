// (v11.26) THE FIVE-STEP FACE — session phase, the 2D regime, price action standing in for
// market internals, and the Friday roll-forward.
//
// The Friday bug was live on 2026-08-21: exp_mode=week means "through Friday of the current
// week", which ON a Friday is today — so the week window collapsed onto the 0DTE window and
// CR/CR0 printed the same number from the same 284 strikes. Confirmed on the live panel.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b), m, {got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

global.window={};
global.SNAP_MAX_STEPBACK=8;
global.STATE={ SPY:{candles:[]}, QQQ:{candles:[]} };
global.EXPSET={}; global.EXPSET_ROLL={}; global.EXPSET_TRY={}; global.EXPSET_FAIL={};
global.EXPSET_SPEC={ dte0:{exp_mode:'current',exp_count:'1',nodes:'500'},
                     week:{exp_mode:'week',exp_count:'1',nodes:'500'},
                     wk7:{exp_mode:'next_n',exp_count:'6',nodes:'500'} };
global.LASTFEED={SPY:null,QQQ:null}; global.LASTVEX={SPY:null,QQQ:null};
global.fmtLvl=(x)=>String(x);
eval(ex('closedCandles')); eval(ex('pickSnapshot'));
eval(ex('sessionPhase')); eval(ex('clv')); eval(ex('paRead')); eval(ex('paReject'));
eval(ex('isoDow')); eval(ex('isoAddDays')); eval(ex('nextWeekFriday'));
eval(ex('expSetRollCheck')); eval(ex('expSetSpecFor')); eval(ex('rollNote'));
eval(ex('bookNet')); eval(ex('regime2D'));

// ---------- session phase: the panel used to be entirely time-blind ----------
// 2026-08-17 is a Monday. CT is UTC-5 in August.
const at=(h,m,dow)=>new Date(Date.UTC(2026,7,17+(dow-1),h+5,m,0));
{
  const o=sessionPhase(at(8,45,5));
  ok(o.rth===true,'08:45 CT is inside RTH');
  ok(/CHARM/.test(o.label),'the first hour is the CHARM phase — dealers could not hedge overnight',o.label);
  ok(o.leftMin===(15*60)-(8*60+45),'minutes left counts to the 15:00 CT close',o.leftMin);
  ok(o.pct>=0&&o.pct<=100,'session progress is a percentage',o.pct);
}
eq(sessionPhase(at(12,0,3)).label,'MIDDAY','midday on a Wednesday is named as the low-energy stretch');
eq(sessionPhase(at(14,15,3)).label,'POWER HOUR','the same clock midweek is the power hour, not expiry');
// (v11.27) an expiry day is an expiry day from the OPEN, not from 13:30
ok(/^EXPIRY/.test(sessionPhase(at(9,0,5)).label),'a Friday is flagged EXPIRY in the MORNING, not only in the last 90 minutes',sessionPhase(at(9,0,5)).label);
ok(/^EXPIRY/.test(sessionPhase(at(12,0,5)).label),'and at midday too',sessionPhase(at(12,0,5)).label);
ok(/charm is accelerating/.test(sessionPhase(at(14,15,5)).sub),'the final stretch of an expiry day says charm is accelerating');
ok(!/EXPIRY/.test(sessionPhase(at(12,0,3)).label),'a Wednesday is never labelled EXPIRY');
// (v11.27) the phase describes CONDITIONS; tactics belong to the regime line alone
[[8,45,5],[10,0,3],[12,0,3],[14,15,3],[14,15,5]].forEach(function(t){
  var o=sessionPhase(at(t[0],t[1],t[2]));
  ok(!/\bfade|\bbreaks? (?:work|usually)/i.test(o.sub||''),'phase text at '+t[0]+':'+t[1]+' gives no fade-vs-break advice — that contradicted the regime playbook',o.sub);
});
{
  const o=sessionPhase(at(17,0,3));
  ok(o.rth===false&&o.label==='CLOSED','after 15:00 CT it says CLOSED rather than reading structure as live');
}
ok(/weekend/.test(sessionPhase(at(12,0,7)).sub||''),'a Sunday says the structure is last Friday’s');

// ---------- close location: the honest OHLC stand-in for TICK ----------
eq(clv({h:10,l:0,c:9}),0.9,'a bar closing near its high has a high close location');
eq(clv({h:10,l:0,c:1}),0.1,'a bar closing near its low has a low close location');
eq(clv({h:5,l:5,c:5}),0.5,'a zero-range bar is neutral, not a divide-by-zero');
{
  STATE.SPY.candles=[]; for(let i=0;i<10;i++) STATE.SPY.candles.push({h:100+i,l:99+i,c:99.9+i});
  const o=paRead('SPY');
  ok(o.ok===true,'price action reads once enough bars have closed');
  ok(o.dir===1,'sustained upper-third closes vote UP — the stand-in for a sustained positive TICK',o.clv);
  eq(o.struct,'HH/HL','rising highs and lows are tagged HH/HL');
  ok(o.upBars===10&&o.dnBars===0,'and it counts the bars rather than only averaging them',[o.upBars,o.dnBars]);
}
{
  STATE.SPY.candles=[]; for(let i=0;i<10;i++) STATE.SPY.candles.push({h:100-i,l:99-i,c:99.1-i});
  const o=paRead('SPY');
  ok(o.dir===-1,'sustained lower-third closes vote DOWN',o.clv);
  eq(o.struct,'LH/LL','falling highs and lows are tagged LH/LL');
}
{
  STATE.SPY.candles=[{h:10,l:0,c:5},{h:10,l:0,c:5},{h:10,l:0,c:5},{h:10,l:0,c:5}];
  const o=paRead('SPY');
  eq(o.dir,0,'mid-range closes with flat structure stay balanced — no vote either way');
  eq(o.struct,'inside','and the structure is inside');
}
// (v11.27) STRUCTURE BREAKS THE TIE. Live on 2026-08-21 the tape was LH/LL with a 0.42 mean close
// location and PA voted NEUTRAL — stepping lows are a direction even when no bar closes at an extreme.
{
  STATE.SPY.candles=[]; for(let i=0;i<10;i++) STATE.SPY.candles.push({h:101-i,l:99-i,c:100-i});
  const o=paRead('SPY');
  eq(o.struct,'LH/LL','falling highs and lows with mid-range closes');
  ok(o.clv>0.40&&o.clv<0.60,'close location alone is in the neutral band',o.clv);
  eq(o.dir,-1,'so STRUCTURE breaks the tie and PA votes down');
  ok(o.tie===true,'and the read is marked as decided on the tiebreak, not on close location');
}
{
  STATE.SPY.candles=[]; for(let i=0;i<10;i++) STATE.SPY.candles.push({h:101+i,l:99+i,c:100+i});
  const o=paRead('SPY');
  eq(o.struct,'HH/HL','rising highs and lows with mid-range closes');
  eq(o.dir,1,'structure breaks that tie upward');
}
ok((STATE.SPY.candles=[{h:1,l:0,c:1}],paRead('SPY').ok===false),'one bar is not a read, and it says so instead of guessing');
{
  STATE.SPY.candles=[]; for(let i=0;i<10;i++) STATE.SPY.candles.push({high:100+i,low:99+i,close:99.9+i});
  ok(paRead('SPY').dir===1,'it reads long-form OHLC keys too, not just the short ones');
}

// ---------- rejection: the only confirmation available without order flow ----------
STATE.SPY.candles=[{h:100,l:98,c:99},{h:103,l:99,c:99.5}];
ok((paReject('SPY',102)||{}).side==='above','a wick through a level that closes back UNDER is a rejection from above');
STATE.SPY.candles=[{h:100,l:98,c:99},{h:100,l:95,c:99.5}];
ok((paReject('SPY',97)||{}).side==='below','a wick under that closes back OVER is a rejection from below');
STATE.SPY.candles=[{h:100,l:98,c:99},{h:103,l:99,c:102.8}];
ok(paReject('SPY',102)===null,'closing BEYOND the level is not a rejection — that is a break');
ok(paReject('SPY',null)===null,'no level means no rejection claim');

// ---------- the Friday roll-forward ----------
eq(isoDow('2026-08-21'),5,'2026-08-21 is a Friday');
eq(nextWeekFriday('2026-08-21'),'2026-08-28','from Friday the roll target is the NEXT Friday');
eq(nextWeekFriday('2026-08-19'),'2026-08-28','from Wednesday the target is still that following Friday');
eq(isoAddDays('2026-08-31',1),'2026-09-01','date arithmetic crosses a month boundary');
{
  EXPSET.SPY={ dte0:{exps:['2026-08-21']}, week:{exps:['2026-08-21']},
    wk7:{exps:['2026-08-21','2026-08-24','2026-08-25','2026-08-26','2026-08-27','2026-08-28','2026-08-31']} };
  EXPSET_ROLL={}; global.EXPSET_ROLL=EXPSET_ROLL;
  expSetRollCheck('SPY');
  ok(!!EXPSET_ROLL.SPY,'a week window that collapsed onto 0DTE is detected');
  eq(EXPSET_ROLL.SPY.to,'2026-08-28','and it rolls forward to next Friday');
  eq(EXPSET_ROLL.SPY.count,'5','exp_count is sized from the control set: 6 expirations through 8/28 means next_n=5');
  eq(expSetSpecFor('SPY','week').exp_mode,'next_n','the rolled week window switches to an explicit count');
  eq(expSetSpecFor('SPY','dte0').exp_mode,'current','the 0DTE window is never rolled');
  ok(/8\/28/.test(rollNote('SPY')||''),'the roll is disclosed on the face rather than applied silently',rollNote('SPY'));
  ok(EXPSET_TRY['SPY|week']===0,'and the throttle is cleared so the rolled window fetches immediately');
}
// (v11.28) THE OSCILLATION. v11.27 re-tested the HELD week set every tick, so the moment the rolled
// window came back spanning six expirations it no longer looked collapsed, the roll was dropped, the
// next fetch went out unrolled and collapsed again. Live on 2026-08-21 the panel flipped between a
// healthy 288-strike set and a degenerate 284-strike one every cycle. The roll is now a property of
// the DATE and stands until the trading day changes.
{
  const armed=JSON.parse(JSON.stringify(EXPSET_ROLL.SPY));
  EXPSET.SPY.week={exps:['2026-08-21','2026-08-24','2026-08-25','2026-08-26','2026-08-27','2026-08-28']};
  expSetRollCheck('SPY');
  ok(!!EXPSET_ROLL.SPY,'the rolled window coming back healthy does NOT cancel the roll');
  eq(EXPSET_ROLL.SPY.to,armed.to,'and the target is unchanged — no re-arming churn');
  expSetRollCheck('SPY'); expSetRollCheck('SPY');
  eq(EXPSET_ROLL.SPY.at,armed.at,'repeated checks on the same day are a no-op, so the fetch throttle is never reset in a loop');
}
{
  // a new trading day re-evaluates from scratch
  EXPSET.SPY.dte0={exps:['2026-08-24']};
  EXPSET.SPY.week={exps:['2026-08-24','2026-08-25','2026-08-26','2026-08-27','2026-08-28']};
  EXPSET.SPY.wk7={exps:['2026-08-24','2026-08-25','2026-08-26','2026-08-27','2026-08-28','2026-08-31','2026-09-01']};
  expSetRollCheck('SPY');
  ok(!EXPSET_ROLL.SPY,'on the Monday the roll is dropped — the week window is real again');
}
{
  // ...and Friday arms it from the DATE alone, without needing a collapsed set to prove it
  EXPSET.SPY.dte0={exps:['2026-08-28']};
  EXPSET.SPY.week={exps:['2026-08-28']};
  EXPSET.SPY.wk7={exps:['2026-08-28','2026-08-31','2026-09-01','2026-09-02','2026-09-03','2026-09-04','2026-09-08']};
  EXPSET_ROLL={}; global.EXPSET_ROLL=EXPSET_ROLL;
  expSetRollCheck('SPY');
  ok(!!EXPSET_ROLL.SPY,'the next Friday arms the roll again');
  eq(EXPSET_ROLL.SPY.to,'2026-09-04','targeting the Friday after that');
  eq(EXPSET_ROLL.SPY.count,'5','sized from the control set again');
}
{
  EXPSET.SPY={ dte0:{exps:['2026-08-21']}, week:{exps:['2026-08-21']}, wk7:{exps:['2026-08-21']} };
  EXPSET_ROLL={}; global.EXPSET_ROLL=EXPSET_ROLL;
  expSetRollCheck('SPY');
  ok(!EXPSET_ROLL.SPY,'with no control expirations reaching next Friday it refuses to guess a count');
}
{
  // (v11.28) Because the roll is decided from the DATE, a Friday arms it before the week set has
  // even arrived — which saves one guaranteed-collapsed fetch rather than waiting to be shown the
  // collapse. The CONTROL set is the one that is genuinely required, since it sizes the request.
  EXPSET.SPY={ dte0:{exps:['2026-08-21']}, week:null, wk7:{exps:['2026-08-21','2026-08-28']} };
  EXPSET_ROLL={}; global.EXPSET_ROLL=EXPSET_ROLL;
  expSetRollCheck('SPY');
  ok(!!EXPSET_ROLL.SPY,'a Friday arms the roll from the date, before the week set has landed');
}
{
  EXPSET.SPY={ dte0:{exps:['2026-08-21']}, week:{exps:['2026-08-21']}, wk7:null };
  EXPSET_ROLL={}; global.EXPSET_ROLL=EXPSET_ROLL;
  expSetRollCheck('SPY');
  ok(!EXPSET_ROLL.SPY,'but with no control set there is nothing to size the request from, so it waits');
}
{
  // a midweek collapse (a holiday-shortened week) is still caught by comparing the held sets
  EXPSET.SPY={ dte0:{exps:['2026-08-19']}, week:{exps:['2026-08-19']},
               wk7:{exps:['2026-08-19','2026-08-20','2026-08-21','2026-08-24','2026-08-25','2026-08-26','2026-08-27']} };
  EXPSET_ROLL={}; global.EXPSET_ROLL=EXPSET_ROLL;
  expSetRollCheck('SPY');
  ok(!!EXPSET_ROLL.SPY,'a midweek week-window that collapses is still detected from the held sets');
}

// ---------- the 2D regime ----------
// OUR sign convention is put-minus-call, so a POSITIVE sum is put-dominant = dealers short
// gamma = the conventional NEGATIVE gamma reading.
const book=(net)=>({ j:{ levels:[ {l:[{k:100,v:Math.abs(net),d:1,net:net}]} ] }, ts:Date.now() });
{
  LASTFEED.SPY=book(500); LASTVEX.SPY=book(500);
  const r=regime2D('SPY');
  ok(r.g===-1&&r.v===-1,'put-dominant in both books reads as negative gamma AND negative vanna',[r.g,r.v]);
  ok(r.danger===true,'that is the self-reinforcing cell and it is flagged');
  ok(/momentum/.test(r.play),'the playbook switches to momentum — fades are the losing side here');
  ok(/widen stops/.test(r.play),'and it says to widen stops, which is the whole point of the flag');
}
{
  LASTFEED.SPY=book(-500); LASTVEX.SPY=book(-500);
  const r=regime2D('SPY');
  ok(r.g===1&&r.v===1&&r.danger===false,'call-dominant in both books is the benign +g/+V cell');
  ok(/fade/.test(r.play),'and there the playbook is to fade the edges');
}
{
  LASTFEED.SPY=book(-500); LASTVEX.SPY=null;
  const r=regime2D('SPY');
  ok(r.v===null&&r.danger===false,'a missing vanna book leaves that axis null rather than assuming it');
  eq(r.label,'REG: +γ','and the label shows only the axis we actually have');
}
{
  LASTFEED.SPY=null; LASTVEX.SPY=null;
  const r=regime2D('SPY');
  ok(r.g===null&&r.label==='—','with no gamma book at all it reports nothing instead of a default regime');
}

console.log('\n'+pass+' pass / '+fail+' fail');
