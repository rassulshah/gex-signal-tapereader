// (v11.19) THE EXPIRY SETS ARE KEYED BY SYMBOL.
// v11.17-11.18 kept ONE global set, hardcoded `symbol=SPY` into the self-fetch request and
// `STATE.SPY.price` into the read. Selecting QQQ or NQ therefore produced SPY levels priced against QQQ —
// confidently wrong numbers on the face of the card, which is worse than showing nothing. Every entry
// point now carries the symbol, and these tests exist so that can never silently come back.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.window={__gptsDebug:{}};
global.SIDE_MIN_SHARE=0.05;
eval(['gexLevels','cpFromPayload','expSetLevels','expSetFetch','expSetTick'].map(ex).join('\n'));

const mk=(k,call,put)=>({k:k, v:call+put, net:put-call, d:1});
function book(base){ return { levels:[{l:[mk(base-5,10,400), mk(base,20,900), mk(base+3,600,30), mk(base+8,300,20)]}],
                              expirations:['2026-08-21','2026-08-24'] }; }
global.SIDE_MIN_SHARE=0.05; global.HVL_MAX_DIST=0.03;
global.EXPSET={ SPY:{ dte0:null, week:{ j:book(760), ts:Date.now(), exps:['2026-08-21','2026-08-24'], n:4 } },
                QQQ:{ dte0:null, week:{ j:book(705), ts:Date.now(), exps:['2026-08-21'], n:4 } } };
global.STATE={ SPY:{price:762.5}, QQQ:{price:707.5} };

// ---------- the read must use the RIGHT book against the RIGHT price ----------
{
  const S=expSetLevels('SPY','week'), Q=expSetLevels('QQQ','week');
  // book(base): base-5 has 400 put, base has 900 put -> the heaviest PUT below spot is `base` itself
  ok(S.ps===760,'SPY reads the SPY book',S.ps);
  ok(Q.ps===705,'QQQ reads the QQQ book — NOT the SPY one',Q.ps);
  ok(S.cr===763 && Q.cr===708,'and each call wall comes from its own book',[S.cr,Q.cr]);
  ok(S.sym==='SPY' && Q.sym==='QQQ','each set reports which symbol it belongs to',[S.sym,Q.sym]);
  ok(S.nExps===2 && Q.nExps===1,'and its own expiration count',[S.nExps,Q.nExps]);
  // THE BUG THIS REPLACES: QQQ levels priced against the SPY spot would put every QQQ level ~55 below
  // price and the card would read as one enormous gap. Both walls must straddle their own spot.
  ok(Q.ps<707.5 && Q.cr>707.5,'the QQQ walls straddle the QQQ spot',[Q.ps,707.5,Q.cr]);
  ok(S.ps<762.5 && S.cr>762.5,'and the SPY walls straddle the SPY spot',[S.ps,762.5,S.cr]);
}
// ---------- a symbol with no set yet returns nothing, not another symbol's ----------
{
  ok(expSetLevels('QQQ','dte0')===null,'an unfetched set is null, never a fallback to a different symbol');
  ok(expSetLevels('IWM','week')===null,'an unknown symbol is null rather than defaulting to SPY');
  global.STATE={ SPY:{price:762.5}, QQQ:{price:null} };
  ok(expSetLevels('QQQ','week')===null,'no price for that symbol means no levels for that symbol');
  global.STATE={ SPY:{price:762.5}, QQQ:{price:707.5} };
}
// ---------- the request must ask for the symbol it was given ----------
{
  const calls=[];
  global.LASTFEEDURL='https://x/api/gex/levels?symbol=SPY&data_type=vanna&nodes=60&exp_mode=next_n&exp_count=4&v=1';
  global.LASTAUTH='Bearer t';
  global.EXPSET_MIN_MS=0; global.EXPSET_TRY={}; global.EXPSET_FAIL={};
  global.EXPSET_SLOW={wk7:1}; global.EXPSET_SLOW_MS=300000;
  global.EXPSET_SPEC={ dte0:{exp_mode:'current', exp_count:'1', nodes:'250'},
                       week:{exp_mode:'week',    exp_count:'1', nodes:'250'},
                       wk7: {exp_mode:'next_n',  exp_count:'6', nodes:'250'} };
  global.fetch=(u)=>{ calls.push(u); return { then(){ return { catch(){} }; } }; };
  expSetFetch('QQQ','dte0');
  ok(calls.length===1,'a request went out',calls.length);
  ok(/symbol=QQQ/.test(calls[0]),'for QQQ — not the hardcoded SPY of v11.17',calls[0]);
  ok(/data_type=gamma/.test(calls[0]),'always gamma, whatever the template carried');
  ok(/exp_mode=current/.test(calls[0]) && /exp_count=1/.test(calls[0]),'with the 0DTE parameters');
  ok(/nodes=250/.test(calls[0]),'and a widened node count');
  expSetFetch('SPY','week');
  ok(/symbol=SPY/.test(calls[1]) && /exp_mode=week/.test(calls[1]),'and the SPY weekly request is separate',calls[1]);
  // throttling and back-off must be per symbol+set, or one symbol starves the other
  global.EXPSET_MIN_MS=90000;
  const before=calls.length;
  expSetFetch('QQQ','dte0');
  ok(calls.length===before,'the same symbol+set is throttled');
  expSetFetch('SPY','dte0');
  ok(calls.length===before+1,'but a DIFFERENT symbol is not blocked by it',calls.length);
  global.EXPSET_FAIL['QQQ|week']=3; global.EXPSET_TRY['SPY|week']=0; const b2=calls.length;
  expSetFetch('QQQ','week');
  ok(calls.length===b2,'a set that has failed three times stops asking');
  expSetFetch('SPY','week');
  ok(calls.length===b2+1,'and that back-off does not silence the other symbol',calls.length-b2);
}
// ---------- the tick follows the chart ----------
// NOTE the eval'd expSetFetch is a module-local binding, so it cannot be stubbed via `global`. The tick is
// therefore observed through the fetch stub — which is the more honest test anyway: it proves a real
// request for the right symbol goes out, not merely that a function was called.
{
  const urls=[];
  global.fetch=(u)=>{ urls.push(u); return { then(){ return { catch(){} }; } }; };
  global.EXPSET_TRY={}; global.EXPSET_FAIL={};
  global.document={visibilityState:'visible'};
  global.activeSym=()=>'QQQ';
  expSetTick();
  ok(urls.length===3,'the tick asks for both live sets plus the slow control',urls.length);
  ok(urls.every(u=>/symbol=QQQ/.test(u)),'for the symbol on the chart, not a fixed one',urls.map(u=>(u.match(/symbol=\w+/)||[])[0]));
  ok(urls.some(u=>/exp_mode=current/.test(u)) && urls.some(u=>/exp_mode=week/.test(u)),'one 0DTE and one through-Friday');
  ok(urls.some(u=>/exp_mode=next_n/.test(u)),'plus the rolling-7 control that is never displayed');
  urls.length=0; global.EXPSET_TRY={}; global.activeSym=()=>'SPY'; expSetTick();
  ok(urls.length===3 && urls.every(u=>/symbol=SPY/.test(u)),'switching charts switches which book is kept fresh',urls.length);
  urls.length=0; global.EXPSET_TRY={}; global.document={visibilityState:'hidden'}; expSetTick();
  ok(urls.length===0,'a hidden tab asks for nothing');
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
