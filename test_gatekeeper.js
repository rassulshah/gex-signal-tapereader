// v10.24: Gatekeeper — nearest high-|value| blocker between price and King + strength ratio.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function mul(a,b){return a/(1/b);} global.mul=mul;
global.REGIME_SIG_PCT=20; global.GK_RATIO_STRONG=1.8;
global.fmtNum=function(n){return (Math.round(n*100)/100).toString();};
let KING; global.tapeMap=function(){return {king:KING};};
let NOW=40000; global.ctNowSecOfDay=function(){return NOW;};
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(['isEarlySession','gatekeeper'].map(ex).join('\n'));
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n+(g!==undefined?' -> '+g:''));}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+g:''));}};
function W(k,pct){return {k,pct,pos:true,net:0};}

// --- Ex3-style: price 663, King 664 just above; strong gatekeeper ~ at King, next-beyond weaker (~3x) ---
// Model as: price 663, King 668 above, gatekeeper 664 (90%) between, node beyond 666 (30%) -> ratio 3x
KING=668; NOW=45000; // not early
global.STATE={SPY:{price:663, walls:[ W(664,90), W(666,30), W(668,85) ]}};
let g=gatekeeper('SPY');
ok('gatekeeper detected on path up to King', g.ok===true, g.ok);
ok('gatekeeper is the DOMINANT intervening node (664, 90%)', g.k===664, g.k);
ok('strength ratio computed vs next-beyond', g.ratio!=null, g.ratio);
ok('dominant gatekeeper -> reversal verdict', /Reversal/.test(g.verdict), g.verdict);
ok('King decoy discount applied (King less reachable)', g.decoyDiscount>0, g.decoyDiscount);

// --- weak gatekeeper: small blocker, huge King beyond -> ratio < 1 -> "may pass" ---
KING=668;
global.STATE={SPY:{price:663, walls:[ W(664,25), W(668,95) ]}};
g=gatekeeper('SPY');
ok('weak blocker -> watch/may-pass verdict', /may pass|Watch/.test(g.verdict), g.verdict);
ok('weak gatekeeper -> no decoy discount', g.decoyDiscount===0, g.decoyDiscount);

// --- clear path: no significant node between price and King ---
KING=668;
global.STATE={SPY:{price:663, walls:[ W(668,95), W(660,40) ]}};
g=gatekeeper('SPY');
ok('no intervening node -> no gatekeeper', g.ok===false, g.ok);

// --- (v10.27) MAGNITUDE-DRIVEN definition: a stronger-but-FARTHER node must beat a
// weaker-but-NEARER one. price 777.5, King 780. On-path nodes: 778 (nearer, 22%) and
// 779 (farther, 41%). Old nearest-rule picked 778; doctrine (2nd-highest node) picks 779. ---
KING=780; NOW=45000;
global.STATE={SPY:{price:777.5, walls:[ W(778,22), W(779,41), W(780,100) ]}};
g=gatekeeper('SPY');
ok('magnitude wins over nearness -> gatekeeper is 779 (stronger), not 778 (nearer)', g.k===779, g.k);

// --- exact-magnitude TIE -> tiebreak to the node NEARER price ---
KING=780; NOW=45000;
global.STATE={SPY:{price:777.5, walls:[ W(778,40), W(779,40), W(780,100) ]}};
g=gatekeeper('SPY');
ok('exact tie -> nearer-to-price wins (778)', g.k===778, g.k);

// --- early-session boost: a borderline ratio becomes strong in the first hour ---
KING=668; NOW=33000; // within first hour (open 30600s; 33000-30600=2400s < 3600)
global.STATE={SPY:{price:663, walls:[ W(664,80), W(666,52), W(668,70) ]}}; // ratio ~1.54, borderline
g=gatekeeper('SPY');
ok('early-session borderline gatekeeper flagged strong', g.strong===true, 'ratio='+g.ratio+' strong='+g.strong);

console.log(f===0?'\nALL PASS':'\n'+f+' FAILED'); process.exit(f===0?0:1);
