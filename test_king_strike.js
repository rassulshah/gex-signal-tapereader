// Regression test for the v10.25 KING-SELECTION BUG.
// BUG: extractWalls returned the max exposure MAGNITUDE as the King (e.g. 6.6e8),
// which consumers treated as a strike -> nonsense King disconnected from the tape.
// FIX: King = the STRIKE with the largest ABSOLUTE exposure (|v|), so a dominant
// NEGATIVE-gamma node can be King. Confirmed against day_811.json where recorded
// `king` was ~4.4e8..9.9e8 while price was ~770 and the true tape king (tking) was 769-775.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
function ok(c,m){ if(c){pass++; console.log('PASS '+m);} else {fail++; console.log('FAIL '+m);} }

// globals extractWalls references
var MIN_STRENGTH=15;
function mul(a,b){ return a*b; }

function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(['extractWalls','synthDerived'].map(ex).join('\n'));

// ---- Case 1: dominant POSITIVE node at 780 ----
var j1={ levels:[ { s:772, l:[
  {k:769, v:5.0e8, d:1}, {k:772, v:6.0e8, d:-1}, {k:780, v:9.9e8, d:1}, {k:775, v:3.0e8, d:1}
] } ] };
var r1=extractWalls(j1);
ok(r1.king===780, 'King = STRIKE of largest |exposure| (780), got '+r1.king);
ok(r1.king<10000, 'King is a strike, NOT a magnitude (got '+r1.king+')');

// ---- Case 2: dominant NEGATIVE-gamma node is King (the tape -85% case) ----
var j2={ levels:[ { s:772, l:[
  {k:769, v:4.0e8, d:1}, {k:772, v:-9.5e8, d:-1}, {k:780, v:6.0e8, d:1}
] } ] };
var r2=extractWalls(j2);
ok(r2.king===772, 'negative-gamma node can be King (772), got '+r2.king);

// the King strike must appear in walls at ~100% (largest |v| normalizes to 100)
var kw=r2.walls.filter(function(w){return Math.abs(w.k-772)<0.001;})[0];
ok(kw && kw.pct===100, 'King node normalizes to 100% ('+(kw?kw.pct:'missing')+')');

// a lesser node normalizes to |v|/kingMag, sign-independent
var lw=r2.walls.filter(function(w){return Math.abs(w.k-780)<0.001;})[0];
ok(lw && lw.pct===Math.round(100*6.0e8/9.5e8), 'lesser node %-normalized to King magnitude ('+(lw?lw.pct:'missing')+')');

// ---- Case 3: empty / degenerate ----
ok(extractWalls({levels:[]}).king===null, 'no levels -> king null');
ok(extractWalls({levels:[{s:772,l:[]}]}).king===null, 'no nodes -> king null');

// ---- Case 4: real recorded day proves the OLD symptom would be caught ----
try{
  var day=JSON.parse(fs.readFileSync('./day_811.json','utf8'));
  var spy=day.snaps.SPY||[];
  var badKings=spy.filter(function(s){ return typeof s.king==='number' && s.king>10000; }).length;
  // (documents the pre-fix corruption; tking was the correct strike)
  ok(badKings>0, 'day_811 recorded '+badKings+' magnitude-as-king snaps (pre-fix corruption, now documented)');
  var tk=spy.map(function(s){return s.tking;}).filter(function(x){return typeof x==='number';});
  ok(tk.every(function(x){return x>700 && x<800;}), 'tking (correct source) were all real strikes 700-800');
}catch(e){ console.log('(day_811 not present, skipping real-data check)'); }

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
