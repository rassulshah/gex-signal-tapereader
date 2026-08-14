// v10.24 Issue I: Node Map model — two-sided, strongest markers, King, verdicts, emphasis.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function mul(a,b){return a/(1/b);} global.mul=mul;
global.NODEMAP_NEAR_PAD=2;
global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',blue:'#4a90d9',sub:'#8b98a9',gold:'#e3c341',amber:'#f2b45a',card:'#12161f',line:'#1e2530',ink:'#e6edf3'};

// stubs for the model's dependencies
let FS, KING, TV, WIN=4, CANDLES=[];
global.futureStructureSummary=function(){return FS;};
global.tapeMap=function(){return {king:KING};};
global.trendVerdict=function(){return TV;};
global.adaptiveProxStrikes=function(){return WIN;};
global.closedCandles=function(){return CANDLES;};
global.gexRegime=function(){return {label:'Trend ↓',dir:-1,conf:'med',skew:2.1,why:'stub',target:769};};
global.gatekeeper=function(){return {ok:false,k:null};};
global.rugDetect=function(){return {ok:false,type:null,targets:[]};};
global.nodeAttraction=function(){return {stage:null,atNode:false};};
global.nodeOutcome=function(){return null;};
global.clusterDetect=function(){return {ok:false,regions:[],memberK:{}};};
global.doubleStackDetect=function(){return {ok:false,stacks:[],memberK:{}};};
global.barneyDetect=function(){return {ok:false,regions:[],memberK:{}};}; // (v10.31)
global.airPocketDetect=function(){return {ok:false,pockets:[],adjacent:null};}; // (v10.32)
global.nodeTapCount=function(){return 0;}; // (v10.33)
global.nodeLifecycle=function(){return {stage:'Fresh',taps:0,prob:80};}; // (v10.33)
global.deflectionAt=function(){return null;}; // (v10.34)

function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(['nmStrength','nmVerdict','nodeMapModel'].map(ex).join('\n'));

let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n+(g!==undefined?' -> '+g:''));}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+g:''));}};

function mkRow(k,side,pct,label,net,hist){ return {k,side,pct,pos:true,role:'Node',state:{label,net,arrow:'',seq:hist},hist:hist||[70,74,80],peak:{touches:1}}; }

// --- Scenario: price 771.9, King 773 above. Res 772(building)/773(king,building);
//     Sup 769(fading)/767(building). Down-trend emphasis (moving away? King is ABOVE) ---
global.STATE={SPY:{price:771.9, king:773}};
KING=773; WIN=4; CANDLES=[{c:773},{c:772.5},{c:771.9}]; // momentum down
TV={state:'dn',up:4,dn:16,win:20,slope:-0.3};
FS={ above:[ mkRow(772,'above',88,'Building',10), mkRow(773,'above',96,'Building',6) ],
     below:[ mkRow(769,'below',54,'Fading',-15), mkRow(767,'below',40,'Building',5) ] };

const m=nodeMapModel('SPY');
ok('model ok', m.ok===true, m.ok);
ok('two-sided: has both above and below levels', m.levels.some(L=>L.side==='above')&&m.levels.some(L=>L.side==='below'), true);
ok('King (773) is marked isKing', m.levels.some(L=>L.k===773&&L.isKing), true);
ok('strongest resistance chosen', !!m.strongRes, m.strongRes&&m.strongRes.k);
ok('strongest support chosen', !!m.strongSup, m.strongSup&&m.strongSup.k);
ok('emphasis side = below (downtrend)', m.emphasis==='below', m.emphasis);
// King is ABOVE, emphasis is BELOW -> moving AWAY from King
ok('againstKing true (price moving away from King above)', m.againstKing===true, m.againstKing);
// verdicts: building res -> Bounce; fading sup -> Break-through
const res772=m.levels.find(L=>L.k===772), sup769=m.levels.find(L=>L.k===769);
ok('building resistance 772 verdict = Bounce', res772.verdict==='Bounce', res772.verdict);
ok('fading support 769 verdict = Break-through', sup769.verdict==='Break-through', sup769.verdict);
// display order: highest strike first
ok('levels ordered highest strike first', m.levels[0].k>=m.levels[m.levels.length-1].k, m.levels.map(L=>L.k).join(','));

// --- forming: node with <3 history samples -> Forming verdict ---
FS={ above:[ {k:772,side:'above',pct:80,pos:true,role:'Node',state:{label:'Building',net:5},hist:[80]} ], below:[] };
const m2=nodeMapModel('SPY');
const r=m2.levels.find(L=>L.k===772);
ok('thin-history node tagged Forming', r&&r.verdict==='Forming'&&r.forming===true, r&&r.verdict);

console.log(f===0?'\nALL PASS':'\n'+f+' FAILED'); process.exit(f===0?0:1);
