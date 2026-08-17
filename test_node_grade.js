// (v10.49 C) NODE GRADE — one grade per meaningful zone, from four inputs:
// polarity · tap freshness · ROC (now + since open) · confluence (QQQ, VEX drift).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

global.STATE={SPY:{price:774.0}, QQQ:{price:null}};
var ACM={ now:{pct:0,label:'Steady'}, day:{pct:0,label:'Steady'} };
var DRIFT={verdict:'NONE', dir:0};
var QAGREE=false;
global.accumCanon=function(){ return ACM; };
global.driftRead=function(){ return DRIFT; };
global.qqqAgrees=function(){ return QAGREE; };
global.nodeTapCount=function(){ return 0; };
global.ruleTier=function(){ return '⚖'; };
global.ACM_BAND=8;

eval(['gradeOfScore','gradeDisp','acmLabel','nodeHoldDir','nodeGrade'].map(ex).join('\n'));

function N(o){ return Object.assign({k:773, pos:true, taps:0, state:'Steady', chg:0}, o); }

// ================= 1. nodeHoldDir =================
ok(nodeHoldDir({k:773}, 774)===1,  '1a a node BELOW price holds -> up');
ok(nodeHoldDir({k:776}, 774)===-1, '1b a node ABOVE price holds -> down');
ok(nodeHoldDir(null, 774)===0 && nodeHoldDir({k:773}, null)===0, '1c missing inputs -> 0');

// ================= 2. polarity =================
var p1=nodeGrade('SPY', N({pos:true}));
var p2=nodeGrade('SPY', N({pos:false}));
ok(p1.score-p2.score===1, '2a +γ scores 1 more than −γ', p1.score+' vs '+p2.score);
ok(p1.inputs.pol==='+' && p2.inputs.pol==='-', '2b polarity recorded on the inputs');
ok(nodeGrade('SPY', N({pos:null})).inputs.pol===null, '2c unknown polarity -> null, no points');

// ================= 3. tap freshness: 0 -> +2, 1 -> +1, 2+ -> −1 =================
var t0=nodeGrade('SPY', N({taps:0})), t1=nodeGrade('SPY', N({taps:1})), t2=nodeGrade('SPY', N({taps:2})), t3=nodeGrade('SPY', N({taps:3}));
ok(t0.score-t1.score===1, '3a a fresh node beats a once-tested one by 1', t0.score+' vs '+t1.score);
ok(t1.score-t2.score===2, '3b 2nd tap -> 3rd tap is a 2-point drop (the graveyard)', t1.score+' vs '+t2.score);
ok(t2.score===t3.score,   '3c 3rd and 4th tap score the same (both spent)', t2.score+'/'+t3.score);
ok(t0.inputs.tap===0 && t3.inputs.tap===3, '3d tap count recorded');

// ================= 4. ROC now =================
var bNow=nodeGrade('SPY', N({state:'Building'})), fNow=nodeGrade('SPY', N({state:'Fading'})), sNow=nodeGrade('SPY', N({state:'Steady'}));
ok(bNow.score-sNow.score===1, '4a Building now is +1');
ok(sNow.score-fNow.score===1, '4b Fading now is −1');
ok(bNow.inputs.rocNow==='Building' && fNow.inputs.rocNow==='Fading', '4c ROC state recorded');
// accumCanon can supply the state when the level carries none
ACM={ now:{pct:20,label:'Acm'}, day:{pct:0,label:'Steady'} };
ok(nodeGrade('SPY', N({state:null})).inputs.rocNow==='Building', '4d accumCanon.now drives ROC when the level is silent');
ACM={ now:{pct:0,label:'Steady'}, day:{pct:0,label:'Steady'} };

// ================= 5. ROC since open: >=+15 -> +1, <=-15 -> −1 =================
ACM={ now:{pct:0,label:'Steady'}, day:{pct:15,label:'Acm'} };
var up=nodeGrade('SPY', N({}));
ACM={ now:{pct:0,label:'Steady'}, day:{pct:14,label:'Acm'} };
var flat=nodeGrade('SPY', N({}));
ACM={ now:{pct:0,label:'Steady'}, day:{pct:-15,label:'Dec'} };
var dn=nodeGrade('SPY', N({}));
ok(up.score-flat.score===1,   '5a +15% since open is the threshold (+1)', up.score+' vs '+flat.score);
ok(flat.score-dn.score===1,   '5b −15% since open is −1', flat.score+' vs '+dn.score);
ok(up.inputs.rocDay.pct===15 && up.inputs.rocDay.label==='Acm', '5c since-open growth recorded with its label');
ACM={ now:{pct:0,label:'Steady'}, day:{pct:0,label:'Steady'} };

// ================= 6. confluence: Q +1, V +1 =================
var base=nodeGrade('SPY', N({}));
QAGREE=true;  var q=nodeGrade('SPY', N({}));
ok(q.score-base.score===1, '6a QQQ agreeing is +1', q.score+' vs '+base.score);
ok(q.inputs.conf.q===true, '6b Q recorded');
QAGREE=false;
DRIFT={verdict:'AGREE-UP', dir:1};                        // node 773 is BELOW price 774 -> holdDir +1
var v=nodeGrade('SPY', N({k:773}));
ok(v.score-base.score===1, '6c VEX drift agreeing with the hold direction is +1', v.score+' vs '+base.score);
ok(v.inputs.conf.v===true && v.inputs.conf.holdDir===1, '6d V + holdDir recorded');
var vOpp=nodeGrade('SPY', N({k:776}));                    // ceiling above: holding means DOWN, drift says UP
ok(vOpp.inputs.conf.v===false, '6e drift agreeing with the WRONG side scores nothing', vOpp.inputs.conf.v);
DRIFT={verdict:'NONE', dir:0};

// ================= 7. the full A / B / C ladder =================
QAGREE=true; DRIFT={verdict:'AGREE-UP', dir:1};
ACM={ now:{pct:20,label:'Acm'}, day:{pct:30,label:'Acm'} };
var A=nodeGrade('SPY', N({k:773, pos:true, taps:0, state:'Building'}));
ok(A.score===7, '7a everything right = 7 (pol 1 + fresh 2 + building 1 + grown 1 + Q 1 + V 1)', A.score);
ok(A.grade==='A', '7b -> grade A', A.grade);
QAGREE=false; DRIFT={verdict:'NONE',dir:0};
ACM={ now:{pct:0,label:'Steady'}, day:{pct:0,label:'Steady'} };
var B=nodeGrade('SPY', N({k:773, pos:true, taps:0, state:'Steady'}));
ok(B.score===3 && B.grade==='B', '7c clean fresh node with no confluence = B', B.score+'/'+B.grade);
ok(B.disp==='B−', '7d ...displayed B− at the threshold', B.disp);
ACM={ now:{pct:0,label:'Steady'}, day:{pct:-40,label:'Dec'} };
var C=nodeGrade('SPY', N({k:773, pos:false, taps:3, state:'Fading'}));
ok(C.score===-3 && C.grade==='C', '7e spent bleeding −γ node = C', C.score+'/'+C.grade);

// ================= 8. defensive =================
ok(nodeGrade('SPY', null).grade==='C', '8a a null level grades C, never throws');
global.accumCanon=function(){ throw new Error('boom'); };
ok(nodeGrade('SPY', N({})).grade!=null, '8b a throwing input never breaks the grade');
global.accumCanon=function(){ return ACM; };
ok(nodeGrade('SPY', N({})).tier==='⚖', '8c tier comes from RULES, defaulting to hand-set ⚖');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
