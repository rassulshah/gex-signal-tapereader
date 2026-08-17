// (v10.49 C) DECISION MATRIX — Direction grade × in-play Node grade. This is the ONLY
// place an action word appears in the whole tool, and the labels characterise SETUP
// QUALITY, never an order: no entry, no stop, no size, no buy/sell/long/short.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function exVar(n){ var i=src.indexOf('var '+n+'='); if(i<0) i=src.indexOf('var '+n+' ='); var j=src.indexOf('\n};', i); return src.slice(i, j+3); }
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

var DIR={grade:'B', dir:'UP'}, NODE={grade:'A'}, INPLAY={k:773};
var RULE=null;
global.directionGrade=function(){ return DIR; };
global.nodeGrade=function(){ return NODE; };
global.inPlayZone=function(){ return INPLAY; };
global.ruleGet=function(){ return RULE; };
global.ruleTier=function(){ return '⚖'; };

eval(exVar('DECISION_MATRIX')+'\n'+ex('decisionCell'));

// ================= 1. the 3×3 labels are exactly the mockup's =================
ok(DECISION_MATRIX.A.A==='take · follow-thru', '1a A×A');
ok(DECISION_MATRIX.A.B==='take · tight tgt',   '1b A×B');
ok(DECISION_MATRIX.A.C==='wait fresher node',  '1c A×C');
ok(DECISION_MATRIX.B.A==='bounce play',        '1d B×A');
ok(DECISION_MATRIX.B.B==='scalp',              '1e B×B');
ok(DECISION_MATRIX.B.C==='skip',               '1f B×C');
ok(DECISION_MATRIX.C.A==='scalp only',         '1g C×A');
ok(DECISION_MATRIX.C.B==='skip',               '1h C×B');
ok(DECISION_MATRIX.C.C==='stand aside',        '1i C×C');

// ================= 2. all nine cells resolve =================
var seen={};
['A','B','C'].forEach(function(dg){ ['A','B','C'].forEach(function(ng){
  DIR={grade:dg,dir:'UP'}; NODE={grade:ng};
  var c=decisionCell('SPY');
  seen[c.cell]=c.text;
  ok(c.cell===dg+'×'+ng, '2·'+dg+ng+' cell id', c.cell);
  ok(c.text===DECISION_MATRIX[dg][ng], '2·'+dg+ng+' text matches the matrix', c.text);
  ok(c.dirGrade===dg && c.nodeGrade===ng, '2·'+dg+ng+' both grades carried');
}); });
ok(Object.keys(seen).length===9, '2z all nine cells distinct', Object.keys(seen).length);

// ================= 3. the in-play strike rides on the cell =================
DIR={grade:'B',dir:'UP'}; NODE={grade:'A'}; INPLAY={k:773.5};
ok(decisionCell('SPY').k===773.5, '3a in-play strike recorded on the decision', decisionCell('SPY').k);
INPLAY=null;
var noNode=decisionCell('SPY');
ok(noNode.k===null && noNode.cell==='B×A', '3b no node in contact still yields a cell (nodeGrade mock)', noNode.cell);
INPLAY={k:773};

// ================= 4. LEARNING: a measured-bad cell re-words itself =================
RULE={ id:'decision.B×A', promoted:true, rate:38, n:44, tier:'measured' };
var reworded=decisionCell('SPY');
ok(/skip/.test(reworded.text) && /38%/.test(reworded.text), '4a a promoted cell measured under 45% is re-worded to skip', reworded.text);
RULE={ id:'decision.B×A', promoted:true, rate:64, n:44, tier:'measured' };
ok(decisionCell('SPY').text==='bounce play', '4b a healthy measured rate leaves the label alone', decisionCell('SPY').text);
RULE={ id:'decision.B×A', promoted:false, rate:20, n:3 };
ok(decisionCell('SPY').text==='bounce play', '4c an UNPROMOTED bad rate may NOT re-word the cell (n too small)');
RULE=null;

// ================= 5. defensive =================
global.directionGrade=function(){ throw new Error('boom'); };
var safe=decisionCell('SPY');
ok(safe.cell==='C×C' && safe.text==='stand aside', '5a a throwing spine degrades to stand aside, never throws', safe.cell);
global.directionGrade=function(){ return DIR; };

// ================= 6. VOCABULARY RED LINE =================
// No matrix label may contain an execution word. The tool stays descriptive.
var banned=/\b(entry|stop|size|buy|sell|long|short)\b/i;
['A','B','C'].forEach(function(dg){ ['A','B','C'].forEach(function(ng){
  ok(!banned.test(DECISION_MATRIX[dg][ng]), '6·'+dg+ng+' label carries no execution word', DECISION_MATRIX[dg][ng]);
}); });
var matrixSrc=exVar('DECISION_MATRIX');
ok(!banned.test(matrixSrc.replace(/\/\/[^\n]*/g,'')), '6z the matrix declaration itself is clean');

// ================= 7. source guards =================
ok(/never entry\/stop\/size|no entry, no stop, no size|not instructions/i.test(src), '7a the red line is documented in source');
ok(/DECISION = the Direction grade|Direction grade × the in-play Node grade/.test(src), '7b the decision hover explains the fusion');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
