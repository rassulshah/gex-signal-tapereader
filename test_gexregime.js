// v10.24: GEX-structure regime classifier — Trend / Whipsaw / Rainbow, from ground-truth shapes.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function mul(a,b){return a/(1/b);} global.mul=mul;
global.REGIME_TREND_SKEW=1.8; global.REGIME_WHIP_EDGEMID=2.0; global.REGIME_RAINBOW_MIN=4; global.REGIME_SIG_PCT=20;
global.fmtNum=function(n){return (Math.round(n*100)/100).toString();};
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(['sum3','gexRegime'].map(ex).join('\n'));
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n+(g!==undefined?' -> '+g:''));}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+g:''));}};
function W(k,pct,pos){return {k,pct,pos:pos!==false,net:0};}

// --- TREND DOWN (mirrors Ex1: price 6435, King 6400 below, ~2.1:1 down-skew) ---
global.STATE={SPY:{price:6435, walls:[ W(6400,100),W(6395,60),W(6390,40), W(6470,30),W(6480,22) ]}};
let r=gexRegime('SPY');
ok('Trend detected (down-skew)', /Trend/.test(r.label), r.label);
ok('Trend direction DOWN', r.dir===-1, r.dir);
ok('skew >= 1.8', r.skew>=1.8, r.skew);
ok('trend target is the heavy down node (6400)', r.target===6400, r.target);

// --- WHIPSAW (two edges + hollow middle: strong 6650 & 6600, weak middle) ---
global.STATE={SPY:{price:6625, walls:[ W(6650,90),W(6600,85), W(6625,22),W(6630,24) ]}};
r=gexRegime('SPY');
ok('Whipsaw detected (bimodal edges, hollow middle)', r.label==='Whipsaw', r.label);
ok('Whipsaw edge/mid ratio >= 2', r.edgeMid>=2, r.edgeMid);
ok('Whipsaw dir neutral', r.dir===0, r.dir);

// --- RAINBOW ROAD (many nodes, both polarities interleaved, full middle, wide) ---
global.STATE={SPY:{price:6600, walls:[ W(6620,60,true),W(6615,55,false),W(6605,50,true),W(6595,52,false),W(6585,48,true),W(6575,45,false) ]}};
r=gexRegime('SPY');
ok('Rainbow Road detected (scattered both-sign)', r.label==='Rainbow Road', r.label);
ok('Rainbow dir = stand aside (0)', r.dir===0, r.dir);
ok('Rainbow counts prominent nodes >= 4', r.prominent>=4, r.prominent);

// --- FORMING (too few prominent nodes) ---
global.STATE={SPY:{price:772, walls:[ W(773,80) ]}};
r=gexRegime('SPY');
ok('single prominent node -> Forming', r.label==='Forming', r.label);

console.log(f===0?'\nALL PASS':'\n'+f+' FAILED'); process.exit(f===0?0:1);
