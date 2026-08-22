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
global.STATE={ SPY:{price:766.28}, QQQ:{price:500} };
global.IF_STALE_MIN=25;
global.FUTMODE={ ok:true, fam:'ES', chart:'ES', underlying:'SPY', r:10.05, futPx:7697, approx:false };
global.dispIsFut=()=>!!(FUTMODE&&FUTMODE.fam&&FUTMODE.ok);
let CHAIN=null, ASKED=null;
global.ifChain=(s)=>{ ASKED=s; return CHAIN; };
eval(ex('ifLadder'));

// SPX-scale levels now, because that is the book ES is a future on
const lv=(o)=>Object.assign({cr:7700,ps:7650,mag:7650,maxPain:7620,crSuppressed:null,psSuppressed:null,strikes:209},o||{});
const chain=(o)=>Object.assign({ spot:7676.56, ageMin:3, rolled:true, err:null,
  pub:{zeroGamma:7679.88, callWall:7700, putWall:7665},
  toFri:{exps:[1,2,3,4,5,6], lv:lv()}, dte0:{exps:[1], lv:lv({cr:7680,ps:7600,maxPain:7610})} }, o||{});
const row=(L,id)=>L.rows.find(r=>r.id.split('·').includes(id));
const at=(L,id)=>{ const r=row(L,id); return r?r.k:null; };

// ---- the levels on the face are THEIRS ----
{
  CHAIN=chain(); const L=ifLadder('SPY');
  ok(!L.err,'their chain produces a ladder',L.err);
  eq(L.src,'IF','tagged as their book');
  eq(at(L,'CR'),7700,'CR is THEIR call wall, on THEIR strike grid');
  eq(at(L,'PS'),7650,'PS is their put wall');
  eq(at(L,'Mag'),7650,'Mag is their magnet');
  eq(at(L,'MP'),7620,'max pain comes across — it needs open interest, which only they have');
  eq(at(L,'CR0'),7680,'CR0 is their 0DTE call wall');
  eq(at(L,'PS0'),7600,'PS0 is their 0DTE put wall');
  eq(L.n,209,'the strike count is theirs');
  eq(L.srcSym,'SPX','and SPX is the book we asked for, because ES is a future on SPX');
  eq(ASKED,'SPX','the companion is asked for SPX even though the panel symbol is SPY');
  ok(L.rolled===true,'the Friday roll rides along so the panel discloses it');
}
// ---- FLIP is THEIR published Zero Gamma, not a number of ours ----
// Their page prints it in the header (SPY 766.48, SPX 7679.88). v11.29 computed a cumulative-net-GEX
// crossing instead and was about to label it IF — the same mislabeling one layer down.
{
  CHAIN=chain(); const L=ifLadder('SPY');
  eq(at(L,'FLIP'),7679.88,'FLIP is their PUBLISHED zero gamma, taken as-is');
  const ids=L.rows.map(r=>r.id).join('·').split('·');
  ok(ids.every(i=>['CR','CR0','PS','PS0','Mag','MP','FLIP'].includes(i)),'every row is a level they actually give us',ids);
}
{
  CHAIN=chain({pub:null}); const L=ifLadder('SPY');
  ok(!L.rows.some(r=>/FLIP/.test(r.id)),'when they publish no zero gamma there is NO FLIP row — nothing is substituted');
}
{
  CHAIN=chain({pub:{zeroGamma:null,callWall:800,putWall:760}}); const L=ifLadder('SPY');
  ok(!L.rows.some(r=>/FLIP/.test(r.id)),'a null zero gamma is absent, not zero');
}
{
  CHAIN=chain({ dte0:{exps:[1], lv:lv({cr:7700,ps:7650})} });
  const L=ifLadder('SPY');
  const r=L.rows.find(x=>x.k===7700);
  ok(r.id.includes('CR')&&r.id.includes('CR0'),'labels landing on one strike MERGE onto one row',r.id);
  ok(r.id.indexOf('CR')<r.id.indexOf('CR0'),'wall first, then the 0DTE qualifier');
}
// ---- THE LIVE BASIS: SPX -> ES, not a maintained constant ----
// SPY reached ES through a ~10.05 multiplier, so half a point of disagreement with their page landed
// on the chart as five ES points. ES is a future on SPX; the conversion is a basis near 1.003, taken
// live from THEIR spot against the futures print.
{
  CHAIN=chain(); const L=ifLadder('SPY');
  const basis=7697/7676.56;
  ok(Math.abs(L.dispScale-basis)<1e-5,'the basis is the live futures print over THEIR spot',L.dispScale);
  ok(L.dispScale>1.0 && L.dispScale<1.01,'which is near 1, not near 10 — no error amplification',L.dispScale);
  eq(L.px,7697,'the price row shows the chart instrument, not the underlying');
  const cr=row(L,'CR');
  ok(Math.abs(cr.disp-(7700*basis))<0.01,'their strike is converted to the chart by that basis',cr.disp);
  ok(Math.abs(cr.und-(7700*(766.28/7676.56)))<0.001,'and separately to the UNDERLYING scale, which is what the candle reads use',cr.und);
  ok(cr.k===7700,'the original strike is preserved — we never restate it as a SPY strike, because SPY OI sits on its own 1-point grid');
}
{
  // a cash chart: no futures print, so the scale falls back to our underlying over theirs
  const keep=global.FUTMODE;
  global.FUTMODE={ ok:true, fam:null, chart:'SPY', underlying:'SPY', r:1, futPx:null };
  CHAIN=chain(); const L=ifLadder('SPY');
  ok(Math.abs(L.dispScale-(766.28/7676.56))<1e-6,'on a cash chart the scale is SPY over SPX',L.dispScale);
  eq(L.px,766.28,'and the price row is the cash price');
  global.FUTMODE=keep;
}
{
  CHAIN=chain({spot:null}); const L=ifLadder('SPY');
  ok(!!L.err && /spot/.test(L.err),'without THEIR spot there is no basis, so there is no ladder',L.err);
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

// ---- (v11.41) THEIR ZERO GAMMA WINS; OURS IS A LABELLED FALLBACK -----------------------------
// The shallow payload scan reported their zero gamma absent, and that conclusion drove a decision to
// compute our own. v1.7 walks nested objects instead. Ours is used ONLY when the payload genuinely
// carries nothing, and it is asterisked and tagged `calc` so it can never read as their number.
{
  CHAIN=chain();                                  // pub.zeroGamma = 7679.88
  const L=ifLadder('SPY');
  eq(at(L,'FLIP'),7679.88,'their published zero gamma is used when the payload has one');
  eq(L.hvlSrc,'pub','and the source is recorded as theirs');
  ok(!L.rows.some(r=>/FLIP\*/.test(r.id)),'no derived row is drawn alongside it');
}
{
  CHAIN=chain({ pub:{zeroGamma:null,callWall:null,putWall:null},
                toFri:{exps:[1,2], lv:lv(), gf:{flip:7666.4}} });
  const L=ifLadder('SPY');
  ok(L.rows.some(r=>/FLIP\*/.test(r.id)),'with nothing published, the derived flip appears');
  eq(L.hvlSrc,'calc','tagged as computed, not as theirs');
  const r=L.rows.find(x=>/FLIP/.test(x.id));
  ok(/\*/.test(r.id),'and it is asterisked on the face');
}
{
  CHAIN=chain({ pub:{zeroGamma:null}, toFri:{exps:[1], lv:lv()} });
  const L=ifLadder('SPY');
  ok(!L.rows.some(r=>/FLIP/.test(r.id)),'nothing published and nothing computed means NO row — never a guess');
  eq(L.hvlSrc,null,'and no source is claimed');
}
{
  // preference is absolute: a published value wins even when a derived one is also available
  CHAIN=chain({ pub:{zeroGamma:7679.88}, toFri:{exps:[1,2], lv:lv(), gf:{flip:7600}} });
  const L=ifLadder('SPY');
  eq(at(L,'FLIP'),7679.88,'theirs wins over ours whenever both exist');
}
console.log('\n'+pass+' pass / '+fail+' fail');
