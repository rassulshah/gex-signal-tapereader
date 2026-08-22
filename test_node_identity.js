// Test v10.26-prep Step 5 node identity: STATUS (Acm/Diss/Steady + Reshuffle) & TYPE (+gamma/-gamma).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }
var PAL={longAccent:'#2ec27e', shortAccent:'#f0616d', gold:'#e3c341', sub:'#8b98a9'};
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(['nodeStatusTag','nodeTypeTag'].map(ex).join('\n'));

// ---- STATUS vocabulary (doc: Accumulation->stronger, Dissipation->weakening) ----
var acm=nodeStatusTag({state:'Building', rapid:false});
ok(/>Acm</.test(acm), 'Building -> Acm label');
ok(acm.indexOf(PAL.longAccent)>=0, 'Acm colored green (strengthening)');

var diss=nodeStatusTag({state:'Fading', rapid:false});
// (v11.40) v10.44 renamed Diss -> Dec ("STATE = Acm / Dec / Steady", stated in the source). The test
// kept asserting the old word and then sat in the 'known stale' bucket for releases.
ok(/>Dec</.test(diss), 'Fading -> Dec label (Diss was renamed in v10.44)');
ok(diss.indexOf(PAL.shortAccent)>=0, 'Diss colored red (weakening)');

var steady=nodeStatusTag({state:'Steady', rapid:false});
// The label is 'Steady'; the 'Hold' rename referenced here never landed in the code.
ok(/>Steady</.test(steady), 'Steady state -> Steady label');
ok(steady.indexOf(PAL.sub)>=0, 'Steady colored grey');

// ---- RESHUFFLE (rapid) = fire (strengthening) / snow (weakening) ----
var rapidUp=nodeStatusTag({state:'Building', rapid:true, rapidDir:1});
ok(rapidUp.indexOf('\uD83D\uDD25')>=0, 'rapid strengthening -> fire icon');
ok(rapidUp.indexOf('Reshuffling')>=0, 'rapid icon tooltip ties to doc word Reshuffling');

var rapidDn=nodeStatusTag({state:'Fading', rapid:true, rapidDir:-1});
ok(rapidDn.indexOf('\u2744')>=0, 'rapid weakening -> snow icon');

var noReshuf=nodeStatusTag({state:'Building', rapid:false});
ok(noReshuf.indexOf('\uD83D\uDD25')<0 && noReshuf.indexOf('\u2744')<0, 'non-rapid -> no reshuffle icon');

// ---- TYPE (gamma polarity) ----
var pos=nodeTypeTag({pos:true});
ok(pos.indexOf('+\u03b3')>=0, 'positive-gamma -> +gamma');
ok(pos.indexOf(PAL.gold)>=0, 'positive-gamma colored yellow');
ok(pos.indexOf('pinning')>=0, 'positive-gamma tooltip = pinning/mean-reverting');

var neg=nodeTypeTag({pos:false});
ok(neg.indexOf('\u2212\u03b3')>=0, 'negative-gamma -> -gamma');
ok(neg.indexOf('#b58bff')>=0, 'negative-gamma colored purple');
ok(neg.indexOf('accelerant')>=0, 'negative-gamma tooltip = accelerant/breakout');

ok(nodeTypeTag({pos:null})==='', 'unknown polarity -> no type tag');

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
