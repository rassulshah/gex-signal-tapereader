// (v11.29) THE LADDER SHOWS INSIDER FINANCE'S LEVELS.
//
// That was the requirement from the first session. The companion already fetched and computed their
// levels; v11.26 then wired the ladder to the SKYLIT sets, so the panel displayed our numbers while
// their chain sat unused in storage. Live on 2026-08-21 the face read CR 766 while their book said 770.
//
// No fallback (user-directed): if their chain is missing or stale the ladder says so and shows nothing.
// Skylit measures flow, IF measures open-interest stock — substituting one for the other would change
// what the numbers mean without changing how they look.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

global.window={};
global.STATE={ SPY:{price:765.28} };
global.IF_STALE_MIN=25;
let CHAIN=null; global.ifChain=()=>CHAIN;
eval(ex('ifLadder'));

const lv=(o)=>Object.assign({cr:770,ps:765,mag:765,maxPain:765,crSuppressed:null,psSuppressed:null,strikes:209},o||{});
const chain=(o)=>Object.assign({ spot:765.215, ageMin:3, rolled:true, err:null,
  pub:{zeroGamma:766.48, callWall:800, putWall:760},
  toFri:{exps:[1,2,3,4,5,6], lv:lv()}, dte0:{exps:[1], lv:lv({cr:768,ps:760,maxPain:762})} }, o||{});
const at=(L,id)=>{ const r=L.rows.find(r=>r.id.split('·').includes(id)); return r?r.k:null; };

// ---- the levels on the face are THEIRS ----
{
  CHAIN=chain(); const L=ifLadder('SPY');
  ok(!L.err,'their chain produces a ladder',L.err);
  eq(L.src,'IF','tagged as their book');
  eq(at(L,'CR'),770,'CR is THEIR call wall — 770, not the 766 Skylit computed');
  eq(at(L,'PS'),765,'PS is their put wall');
  eq(at(L,'Mag'),765,'Mag is their magnet');
  eq(at(L,'MP'),765,'max pain comes across — it needs open interest, which only they have');
  eq(at(L,'CR0'),768,'CR0 is their 0DTE call wall');
  eq(at(L,'PS0'),760,'PS0 is their 0DTE put wall');
  eq(L.n,209,'the strike count is theirs');
  ok(L.rolled===true,'the Friday roll rides along so the panel discloses it');
}
// ---- HVL is THEIR published Zero Gamma, not a number of ours ----
// Their page prints it in the header (SPY 766.48, SPX 7679.88). v11.29 computed a cumulative-net-GEX
// crossing instead and was about to label it IF — the same mislabeling one layer down.
{
  CHAIN=chain(); const L=ifLadder('SPY');
  eq(at(L,'HVL'),766.48,'HVL is their PUBLISHED zero gamma, taken as-is');
  const ids=L.rows.map(r=>r.id).join('·').split('·');
  ok(ids.every(i=>['CR','CR0','PS','PS0','Mag','MP','HVL'].includes(i)),'every row is a level they actually give us',ids);
}
{
  CHAIN=chain({pub:null}); const L=ifLadder('SPY');
  ok(!L.rows.some(r=>/HVL/.test(r.id)),'when they publish no zero gamma there is NO HVL row — nothing is substituted');
}
{
  CHAIN=chain({pub:{zeroGamma:null,callWall:800,putWall:760}}); const L=ifLadder('SPY');
  ok(!L.rows.some(r=>/HVL/.test(r.id)),'a null zero gamma is absent, not zero');
}
{
  CHAIN=chain({ dte0:{exps:[1], lv:lv({cr:770,ps:765})} });
  const L=ifLadder('SPY');
  const row=L.rows.find(r=>r.k===770);
  ok(row.id.includes('CR')&&row.id.includes('CR0'),'labels landing on one strike MERGE onto one row',row.id);
  ok(row.id.indexOf('CR')<row.id.indexOf('CR0'),'wall first, then the 0DTE qualifier');
}
// ---- no fallback ----
{
  CHAIN=null; const L=ifLadder('SPY');
  ok(!!L.err,'no companion means no ladder — Skylit is NOT substituted',L);
  ok(!L.rows,'and no rows come back at all');
}
{
  CHAIN=chain({ ageMin:40 }); const L=ifLadder('SPY');
  ok(!!L.err && /40m/.test(L.err),'a stale chain is refused, and the age is named',L.err);
  ok(L.stale===true,'flagged stale rather than absent');
}
ok((CHAIN=chain({ageMin:24}), !ifLadder('SPY').err),'just inside the staleness bound it still reads');
eq((CHAIN=chain({err:'fetch blocked'}), ifLadder('SPY').err),'fetch blocked','their own error passes through verbatim');
ok((CHAIN=chain({toFri:null,dte0:null}), !!ifLadder('SPY').err),'a chain with no level sets is an error, not an empty ladder');
{
  global.STATE.SPY.price=null;
  ok((CHAIN=chain(), !!ifLadder('SPY').err),'with no price there is no ladder — the distances would be meaningless');
  global.STATE.SPY.price=765.28;
}
{
  CHAIN=chain({ toFri:{exps:[1,2], lv:lv({cr:null,crSuppressed:{k:790,share:2.1}})} });
  const L=ifLadder('SPY');
  ok(L.suppressed.length===1 && /790/.test(L.suppressed[0]),'a wall THEIR rule withholds is surfaced, not silently dropped',L.suppressed[0]);
}
console.log('\n'+pass+' pass / '+fail+' fail');
