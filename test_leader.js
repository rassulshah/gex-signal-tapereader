// (16.52) ONE ACTIVE TAB PER ORIGIN — the leader election in localStorage. Two "tabs" share one fake localStorage; each
// runs its own copy of the election code with its own TAB_ID / PANEL_MUTED / FUTMODE. Pins: the first unmuted tab leads;
// a second equal-rank tab yields (stable tie on the id); a live ES-futures chart outranks an ES-family chart outranks the
// rest; a muted tab never leads and drops the key it held; a dead leader is replaced after the TTL; the gates read it.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m) throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
const store={}; global.localStorage={ getItem:(k)=>(k in store?store[k]:null), setItem:(k,v)=>{ store[k]=String(v); }, removeItem:(k)=>{ delete store[k]; } };
global.document={ getElementById:()=>null, createElement:()=>({ style:{} }), body:{ appendChild(){} } }; global.css=()=>{};
function tab(id, opts){
  opts=opts||{};
  const code=[
    'var TAB_ID='+JSON.stringify(id)+', LEADER_KEY="gpts_leader_v1", LEADER_TTL=6000, LEADER_ME=false, LEADER_OTHER=null;',
    'var PANEL_MUTED='+(opts.muted?'true':'false')+';',
    'var FUTMODE='+JSON.stringify(opts.fut||{ chart:'SPY', fam:null, live:false })+';',
    'function dispMarket(){ return '+JSON.stringify(opts.market||'ES')+'; }',
    ex('leaderRead'), ex('leaderRank'), ex('leaderChart'), ex('leaderTick'), ex('tabActive'),
    'return { tick:function(now){ return leaderTick(now); }, active:function(){ return tabActive(); }, mute:function(m){ PANEL_MUTED=m; }, rank:function(){ return leaderRank(); } };'
  ].join('\n');
  return (new Function('localStorage', code))(global.localStorage);
}
const T0=1000000;
// 1. two ES-family tabs, equal rank: the first to tick leads, the second yields, and it stays that way
const A=tab('aaaa0001'), B=tab('bbbb0002');
ok(A.tick(T0)===true && A.active(), '1a the first unmuted tab claims');
ok(B.tick(T0+100)===false && !B.active(), '1b a second equal-rank tab yields to a live leader');
ok(A.tick(T0+2000)===true && B.tick(T0+2100)===false, '1c ...and the roles hold on the next heartbeats');
// 2. the tie is stable: if BOTH claimed in the same instant, the smaller id wins on the next tick
store['gpts_leader_v1']=JSON.stringify({ id:'bbbb0002', t:T0+3000, rank:2, chart:'SPY', since:T0+3000 });
ok(A.tick(T0+3001)===true, '2a the smaller id claims over an equal-rank record it did not write');
ok(B.tick(T0+3002)===false, '2b ...and the larger id yields to it');
// 3. rank: a live ES-futures chart beats an ES-family chart beats anything else
const E=tab('zzzz0009', { fut:{ chart:'EPZ26', fam:'ES', live:true } });
ok(E.rank()===3 && A.rank()===2 && tab('gggg', { market:'NQ', fut:{ chart:'GC1', fam:'GC', live:false } }).rank()===1, '3a ranks: live ES futures 3, ES family 2, other 1');
ok(E.tick(T0+4000)===true && A.tick(T0+4001)===false && B.tick(T0+4002)===false, '3b the ES1 tab takes the lead from the SPY-scale tab whatever their ids');
// 4. a muted tab never leads and gives up the key it held
E.mute(true); ok(E.tick(T0+5000)===false && !('gpts_leader_v1' in store), '4a the leader closes its panel (✕): it drops the key at once');
ok(A.tick(T0+5001)===true, '4b ...and the next unmuted tab takes over immediately');
E.mute(false);
// 5. a dead leader (tab closed, no heartbeat) is replaced after the TTL, not before
ok(B.tick(T0+5001+5000)===false, '5a 5 s after the leader\'s last beat: still its');
ok(B.tick(T0+5001+6001)===true, '5b 6 s after: taken over');
// 6. the gates in the live source read tabActive()
const tickSrc=ex('tick'), feedSrc=ex('onFeed'), ktSrc=ex('ktrkSample'), exSrc=ex('gammaProfileExportNow'), irSrc=ex('irtExportNow');
ok(/leaderTick\(\)/.test(tickSrc) && /if\(!tabActive\(\)\)\{ try\{ render\(\); \}catch\(eRp\)\{\} return; \}/.test(tickSrc), '6a tick(): a passive tab renders what it has and returns before the recorders');
ok(/!tabActive\(\)\) return;/.test(feedSrc), '6b onFeed(): a passive tab records nothing from the feed');
ok(/!tabActive\(\)\) return;/.test(ktSrc), '6c ktrkSample(): a passive tab does not sample the Kings');
ok(/!tabActive\(\)\) return;/.test(exSrc) && /!tabActive\(\)\) return;/.test(irSrc), '6d the two exports refuse in a passive tab');
console.log('test_leader: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
