// (v11.42) LEVEL DEPTH, EXPECTED MOVE AND THE CHART AXES.
//
// Every ladder row looked equally important. A strike carrying heavy dealer GAMMA and heavy dealer
// DELTA is a materially harder place for price to pass than one carrying gamma alone — gamma decides how
// price behaves there, delta decides how much hedging must happen to get through.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
// the SECOND declaration is ours
function ex2(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');let m,last=null;while((m=re.exec(src))!==null) last=m;let i=src.indexOf('{',last.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(last.index,e+1);}

global.window={}; global.LASTFEED={SPY:null}; global.STATE={SPY:{price:766}};
let CHAIN=null; global.ifChain=()=>CHAIN;
global.ifLadder=()=>({err:null, undScale:0.0998, dispScale:1.0023});
global.pickSnapshot=(s)=>({snap:s[s.length-1]});
eval(ex('levelDepth')); eval(ex('confTier'));

// (v11.43) BOTH BOOKS NOW COME FROM THEIR CHAIN. Gamma used to be read from LASTFEED — Skylit flow —
// while the LEVELS being scored came from InsiderFinance. Two different books: Skylit gamma peaks at
// spot, IF's walls sit away from it, and every level measured gamma-thin (0.02-0.20 live). A level has
// to be compared to the book it came from.
const chain=(gexProf,dexProf)=>({err:null, toFri:{ds:{gexProf:gexProf, dexProf:dexProf}}});

// ---- both books, and the tiers ----
{
  CHAIN=chain([[7676,-800],[7716,-40]], [[7676,-2000],[7716,-100]]);   // 7676*0.0998 = 766.1
  const C=levelDepth('SPY');
  ok(C.ok,'depth reads both of THEIR books');
  const t=confTier(C,766);
  ok(!!t,'a level heavy in gamma AND delta scores');
  eq(t.tier,2,'and it is the top tier — both books loaded');
}
{
  CHAIN=chain([[7676,-800],[7716,-40]], [[7676,-10],[7716,-2000]]);
  const C=levelDepth('SPY');
  const t=confTier(C,766);
  eq(t.tier,1,'heavy in ONE book only is the lower tier — real, but easier to pass');
}
{
  CHAIN=chain([[7676,-800],[7716,-20]], [[7676,-2000],[7716,-20]]);
  const C=levelDepth('SPY');
  eq(confTier(C,770).tier,0,'a level thin in both books scores nothing and gets no marker');
}
{
  CHAIN=chain([[7676,-800]], null);
  const C=levelDepth('SPY');
  ok(C.ok,'gamma alone still produces a reading');
  ok(confTier(C,766).d===0,'with the delta side simply zero rather than assumed');
}
{
  CHAIN=chain(null, [[7676,-2000]]);
  const C=levelDepth('SPY');
  ok(C.ok,'and delta alone does too');
  ok(confTier(C,766).g===0,'with gamma zero rather than borrowed from another book');
}
{
  // gamma and delta can still land on adjacent keys after scaling; the zone must catch both
  CHAIN=chain([[7675,-800]], [[7676,-2000]]);
  const C=levelDepth('SPY');
  const t=confTier(C,766);
  ok(t.g>0 && t.d>0,'adjacent keys inside the zone are aggregated, not resolved to one',t);
}
{
  CHAIN=chain([[7676,-800]], [[7800,-2000]]);
  const C=levelDepth('SPY');
  const t=confTier(C,766);
  ok(t.d===0,'but a strike far away is NOT pulled in — tolerance, not nearest-at-any-distance');
}
eq(confTier(null,766),null,'no depth data means no tier rather than a default');
eq(confTier({ok:false},766),null,'and an empty read is the same');
{
  CHAIN=null;
  eq(levelDepth('SPY').ok,false,'with no chain there is nothing to score');
}
{
  ok(/labelled "IF structure"; it has to BE IF/.test(src)||/BOTH BOOKS NOW COME FROM|BOTH SIDES FROM THE SAME BOOK/.test(src),
     'and the source records why both sides must come from one book');
}
// ---- the axes ----
{
  ok(/A chart without scales is a picture/.test(src),'the axes are deliberate, and the source says why');
  ok(/steps=\[1,2,2\.5,5,10,20,25,50,100\]/.test(src),
     'price ticks land on ROUND numbers chosen from the range, not on the range divided by four');
  ok(/Math\.ceil\(lo\/stepv\)\*stepv/.test(src),'the first tick is rounded UP into the range');
  ok(/stepM=\(mins>75\)\?30:15/.test(src),'the time step widens on a longer window so labels cannot collide');
  ok(/roll=\(stepM-\(m0%stepM\)\)%stepM/.test(src),
     'and time marks land on the half hour rather than on whenever the window happens to start');
}
// ---- expected move ----
{
  // v11.45 routed FRAME cells through cell(label,value,tip). v11.66-v11.86 rewrote the section into
  // three rows and the cells became g3tip'd <span class="g3emk"> — label AND value inside ONE tipped
  // element, which is the property that mattered. The old greps kept passing on the NAME of the old
  // helper, so when the helper went they failed while the property was intact. Assert the PROPERTY.
  const FR=ex('secFrame');
  const emk=FR.split('<span class="g3emk"').slice(1);
  ok(emk.length>=2, 'the FRAME band cells are on the line', emk.length);
  ok(emk.every(x=>x.startsWith("'+g3tip(")),
     'and every one of them opens with its tip, so the VALUE is hoverable and not just the label',
     emk.map(x=>x.slice(0,24)));
  ok(/EXP LOW/.test(FR) && /EXP HIGH/.test(FR),
     'the rails are named EXP LOW / EXP HIGH (v11.75, the user\'s wording)');
  ok(/0\.80 sigma/.test(FR) && /58%/.test(FR) && /1\.25/.test(FR),
     'and the hover carries the caveat the label does not: 0.80 sigma, ~58% containment, x1.25 to a true one-sigma');
  ok(/a one-sided straddle is not a straddle/i.test(src),'the blank case is explained rather than left mysterious');
}
// ---- depth reaches REACTION ----
{
  ok(/>DEPTH<\/em>/.test(src),'REACTION carries a DEPTH row');
  ok(/What is standing behind it\?/.test(src),'asking its question first');
  ok(/both loaded/.test(src),'and it calls out when both books are heavy');
}
console.log('\n'+pass+' pass / '+fail+' fail');
