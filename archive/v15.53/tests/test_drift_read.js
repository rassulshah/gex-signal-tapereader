// (v10.49 D) GEX/VEX DRIFT READ — the v10.48-vetted math, pinned to the REAL 2026-08-17
// tape. GVWAP/VVWAP are exposure-weighted mean strikes with a ±1σ band, each normalised
// INSIDE ITS OWN FEED so vanna's ~10× magnitudes cannot tilt the verdict.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

global.mul=function(a,b){ return a/(1/b); };
global.STATE={SPY:{price:773.66}, QQQ:{price:null}};
global.LASTFEED={SPY:null,QQQ:null};
global.LASTVEX ={SPY:null,QQQ:null};

eval(['driftNodesOf','driftStat','driftRead'].map(ex).join('\n'));

// ---- the real live book, 2026-08-17 (SPY 773.66) ----
const GAMMA=[[773,600099584],[776,275462048],[775,205436832],[774,186258000],[772,127631712]];
const VANNA=[[776,5451520000],[773,3569443072],[775,2608675584],[777,1720259712],[772,1698502912],[778,1238969344]];
function payload(rows, px){ return { levels:[ { s:px, l:rows.map(function(r){ return {k:r[0], v:r[1], d:1, net:0}; }) } ] }; }
global.LASTFEED.SPY={ j:payload(GAMMA,773.66), feed:'gamma', ts:Date.now() };
global.LASTVEX.SPY ={ j:payload(VANNA,773.66), ts:Date.now() };

// ========== 1. driftStat: weighted mean + population sigma ==========
var G=driftStat(GAMMA), V=driftStat(VANNA);
ok(Math.abs(G.vwap-773.929)<0.005, '1a GVWAP = 773.93 on the real gamma book', G.vwap);
ok(Math.abs(V.vwap-775.023)<0.005, '1b VVWAP = 775.02 on the real vanna book', V.vwap);
ok(Math.abs(G.sd-1.3147)<0.005,    '1c gamma sigma 1.31', G.sd);
ok(Math.abs(V.sd-1.8046)<0.005,    '1d vanna sigma 1.80', V.sd);
ok(Math.abs(G.lo-772.61)<0.01 && Math.abs(G.hi-775.24)<0.01, '1e gamma band 772.61–775.24', G.lo+'–'+G.hi);
ok(Math.abs(V.lo-773.22)<0.01 && Math.abs(V.hi-776.83)<0.01, '1f vanna band 773.22–776.83', V.lo+'–'+V.hi);
ok(G.n===5 && V.n===6, '1g node counts carried through', G.n+'/'+V.n);

// ========== 2. NORMALISATION: 10× magnitudes must not change anything ==========
var V10=driftStat(VANNA.map(function(r){ return [r[0], r[1]*10]; }));
ok(Math.abs(V10.vwap-V.vwap)<1e-6, '2a scaling every weight ×10 leaves VWAP identical', V10.vwap);
ok(Math.abs(V10.sd-V.sd)<1e-6,     '2b ...and leaves sigma identical', V10.sd);
var Vtiny=driftStat(VANNA.map(function(r){ return [r[0], r[1]/1000]; }));
ok(Math.abs(Vtiny.vwap-V.vwap)<1e-6, '2c scaling down ×1/1000 also leaves VWAP identical', Vtiny.vwap);
ok(driftStat([])===null && driftStat(null)===null, '2d empty/null input -> null');
ok(driftStat([[773,0],[774,0]])===null, '2e all-zero weights -> null (no divide by zero)');

// ========== 3. driftNodesOf: |v| weights, junk skipped ==========
var nodes=driftNodesOf(payload(GAMMA,773.66));
ok(nodes.length===5, '3a extracts one row per level', nodes.length);
ok(nodes[0][0]===773 && nodes[0][1]===600099584, '3b strike + |exposure| pair');
var neg=driftNodesOf({levels:[{s:773,l:[{k:772,v:-500},{k:774,v:300},{k:775,v:0},{k:null,v:99}]}]});
ok(neg.length===2 && neg[0][1]===500, '3c negative gamma contributes its MAGNITUDE, zero/nullish dropped', JSON.stringify(neg));
ok(driftNodesOf({levels:[]})===null && driftNodesOf(null)===null, '3d empty payload -> null');

// ========== 4. driftRead: the live verdict ==========
var r=driftRead('SPY');
ok(r.verdict==='AGREE-UP', '4a verdict AGREE-UP (both centres above 773.66, bands overlap)', r.verdict);
ok(r.label==='UP·conf',    '4b label UP·conf', r.label);
ok(r.dir===1,              '4c dir +1', r.dir);
ok(r.gvwap===773.93,       '4d gvwap rounded to 773.93', r.gvwap);
ok(r.vvwap===775.02,       '4e vvwap rounded to 775.02', r.vvwap);
ok(r.overlap===true,       '4f bands overlap');
ok(r.px===773.66,          '4g px carried', r.px);
ok(r.gLo===772.61 && r.gHi===775.24, '4h gamma band on the record', r.gLo+'–'+r.gHi);
ok(r.vLo===773.22 && r.vHi===776.83, '4i vanna band on the record', r.vLo+'–'+r.vHi);

// ========== 5. the other verdicts ==========
global.STATE.SPY.price=778.00;                       // price above BOTH centres now
var dn=driftRead('SPY');
ok(dn.verdict==='AGREE-DN' && dn.label==='DN·conf' && dn.dir===-1, '5a both centres below price -> AGREE-DN', dn.verdict);

global.STATE.SPY.price=774.50;                       // between the two centres
var sp=driftRead('SPY');
ok(sp.verdict==='SPLIT' && sp.label==='split' && sp.dir===0, '5b centres straddle price -> SPLIT', sp.verdict);

// (v10.49.1) non-overlapping bands but SAME side of price => LEAN (agree on direction,
// lower confidence). SPLIT is reserved for opposite sides. Bands only set conf vs lean.
global.STATE.SPY.price=700.00;
global.LASTVEX.SPY={ j:payload([[900,1],[900.01,1]],700), ts:Date.now() };
var noOv=driftRead('SPY');
ok(noOv.verdict==='LEAN-UP' && noOv.dir===1 && noOv.overlap===false, '5c same side disjoint bands -> LEAN-UP not SPLIT', noOv.verdict);

global.LASTVEX.SPY=null; global.STATE.SPY.price=773.66;
var none=driftRead('SPY');
ok(none.verdict==='NONE' && none.label==='—' && none.dir===0, '5d no VEX feed -> NONE', none.verdict);
ok(none.gvwap===773.93, '5e ...but GVWAP is still reported', none.gvwap);
global.LASTFEED.SPY=null;
ok(driftRead('SPY').verdict==='NONE', '5f no feeds at all -> NONE, no throw');
global.LASTFEED.SPY={ j:payload(GAMMA,773.66), feed:'gamma', ts:Date.now() };
global.LASTVEX.SPY ={ j:payload(VANNA,773.66), ts:Date.now() };
global.STATE.SPY.price=null;
ok(driftRead('SPY').verdict==='NONE', '5g no price -> NONE (never guesses a side)');


// ---- (v10.56 PART E) each centre says which side of price it sits on: ↓ below (red) / ↑ above (green)
(function(){
  try{
    global.PAL=global.PAL||{}; PAL.shortAccent='#f0616d'; PAL.longAccent='#2ec27e';
    eval(ex('driftSideArrow'));
    ok(/↓/.test(driftSideArrow(768.7,770)) && /#f0616d/.test(driftSideArrow(768.7,770)), 'centre BELOW price -> red ↓');
    ok(/↑/.test(driftSideArrow(771.7,770)) && /#2ec27e/.test(driftSideArrow(771.7,770)), 'centre ABOVE price -> green ↑');
    ok(driftSideArrow(null,770)==='' && driftSideArrow(770,null)==='', 'missing number -> no arrow (no invented lean)');
    ok(/G'\+\(d\.gvwap!=null\?fmtLvl\(d\.gvwap\):'–'\)\+driftSideArrow\(d\.gvwap,d\.px\)/.test(src) && /V'\+\(d\.vvwap!=null\?fmtLvl\(d\.vvwap\):'–'\)\+driftSideArrow\(d\.vvwap,d\.px\)/.test(src), 'the drift line renders G<n><arrow> · V<n><arrow>');
    ok(/var stepsLine=\(function\(\)\{[\s\S]{0,600}justify-content:center;text-align:center/.test(src), 'Steps 1-5 line is CENTRED (v10.56 PART E)');
  }catch(e){ ok(false,'v10.56 drift arrows / centred steps threw: '+e); }
})();
console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);

// ---- v10.49.1 regression: same side of price => NOT split (band overlap only sets conf) ----
(function(){
  try{
    // gamma centred 774 tight, vanna centred 776 tight — bands do NOT overlap, but BOTH above px 772
    global.LASTFEED={SPY:{j:{levels:[{s:772,l:[{k:774,v:900},{k:773,v:300},{k:775,v:250}]}]}}};
    global.LASTVEX ={SPY:{j:{levels:[{s:772,l:[{k:776,v:900},{k:777,v:300},{k:775,v:250}]}]}}};
    global.STATE={SPY:{price:772}};
    var d=driftRead('SPY');
    ok(d.dir===1, 'both centres above px -> dir UP (not split), got dir='+d.dir+' verdict='+d.verdict);
    ok(d.verdict!=='SPLIT', 'same side => not SPLIT, got '+d.verdict);
    // opposite sides => SPLIT
    global.LASTVEX={SPY:{j:{levels:[{s:772,l:[{k:769,v:900},{k:768,v:300},{k:770,v:250}]}]}}};
    var d2=driftRead('SPY');
    ok(d2.verdict==='SPLIT', 'opposite sides of px => SPLIT, got '+d2.verdict);
  }catch(e){ ok(false,'v10.49.1 regression threw: '+e); }
})();
