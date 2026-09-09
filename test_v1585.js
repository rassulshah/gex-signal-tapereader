// test_v1585.js — (v15.85) XG2–XG5 · SG2–SG5 — his labels for the IRT node lines. Operator, 2026-09-08 (evening):
// "in the irt export, the spy gama level cand be SG2 SG3 etc., and the spx levels can be XG2 and XG3 etc.. make this
// change. also note the qqq king should only be on the nq not the es." The second sentence has been true since v15.80
// (R-18) and is pinned again here; the NQ symbol's QQQ lines keep G2–G5 until he says otherwise (asked).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }

ok(/@version\s+15\.(8[5-9]|9\d)/.test(src) && /var GPTS_VERSION='15\.(8[5-9]|9\d)';/.test(src), '0a v15.85 or later in both spots');

// ---------- 1. the label sites ----------
{
  global.window={__gptsDebug:{}};
  eval(['irtSpyTop','irtQqqTop'].map(ex).join('\n'));
  const J={ levels:[{ s:768, l:[{k:770,v:-127712000,d:-1},{k:767,v:125000000,d:1},{k:766,v:102000000,d:1},{k:765,v:85600000,d:1},{k:769,v:51000000,d:1},{k:768,v:23000000,d:-1}] }] };
  const T=irtSpyTop(J, 770);
  ok(T.rows.map(r=>r.lbl).join(' ')==='SG2 SG3 SG4 SG5', '1a the SPY book\'s next four are SG2..SG5', T.rows.map(r=>r.lbl));
  ok(/lbl:'XG'\+\(i\+2\)/.test(ex('irtBuildCsv')) && !/lbl:'G'\+\(i\+2\)/.test(ex('irtBuildCsv')), '1b the SPX book\'s next four are XG2..XG5 (the G-only label is gone from the ES rows)');
  global.ladderFor=()=>({ pct:{ '722.00':100, '721.00':44, '720.00':-9, '723.00':30, '719.00':12, '724.00':8 }, count:20, src:'ladder' }); global.tapeMap=()=>null;
  const Q=irtQqqTop(722);
  ok(Q.rows.map(r=>r.lbl).join(' ')==='G2 G3 G4 G5', '1c the NQ symbol\'s QQQ lines stay G2..G5 (his call pending)', Q.rows.map(r=>r.lbl));
  ok(/\/\^\(XG\|SG\|G\)\[2-5\]\$\|\^D-\[A-Z\]\+ KING\$\|\^D-\[A-Z\]\+\[2-9\]\$\|\^D\[1-9\]\$\/\.test/.test(ex('irtGHeld')) && !/\[GS\]\[2-5\]/.test(ex('irtGHeld')), '1d the hold accepts XG / SG (ES), G (NQ) and (v15.91) the D rows by book, nothing else');
  ok(/SG rows '\+g3esc\(IRT_LAST\.sWhy\)/.test(src), '1e the gear\'s IRT line says SG rows');
}

// ---------- 2. the hold filters by the new grammar ----------
{
  eval(v('IRT_KINGS_KEY'));
  var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
  global.ctTodayStr=()=>'2026-09-08';
  eval(['irtGLatch','irtGHeld'].map(ex).join('\n'));
  LS[IRT_KINGS_KEY]=JSON.stringify({ day:'2026-09-08', G:{ rows:[{k:7647.5, lbl:'G2', spx:7630, pct:85},{k:7667.5, lbl:'XG3', spx:7650, pct:41},{k:7690, lbl:'XG1', spx:7675, pct:30},{k:7700, lbl:'S2', spx:7680, pct:20}], t:Date.now() }, GS:{ rows:[{k:767, lbl:'S2', pct:98},{k:766, lbl:'SG3', pct:80}], t:Date.now() }, GQ:{ rows:[{k:721, lbl:'G2', pct:44},{k:720, lbl:'QG3', pct:-9}], t:Date.now() } });
  const HG=irtGHeld(), HS=irtGHeld('GS'), HQ=irtGHeld('GQ');
  ok(HG && HG.rows.map(r=>r.lbl).join(' ')==='G2 XG3', '2a an ES hold keeps XG rows (and a pre-v15.85 G row for one tick); XG1 and an S row are dropped', HG&&HG.rows.map(r=>r.lbl));
  ok(HS && HS.rows.map(r=>r.lbl).join(' ')==='SG3', '2b a SPY hold keeps SG rows only — the pre-v15.85 S2 is dropped, the next fresh book re-latches', HS&&HS.rows.map(r=>r.lbl));
  ok(HQ && HQ.rows.map(r=>r.lbl).join(' ')==='G2', '2c the NQ hold keeps G rows; a QG label is not (yet) a label', HQ&&HQ.rows.map(r=>r.lbl));
  irtGLatch([{k:7647.5, lbl:'XG2', spx:7630, pct:85}]);
  ok(irtGHeld().rows[0].lbl==='XG2', '2d a fresh latch carries the XG label into the hold');
}

// ---------- 3. the QQQ King is on NQ only (v15.80, pinned again on his word) ----------
ok(!/QQQ KING.*EPU26|EPU26.*QQQ KING/.test(ex('irtBuildCsv').replace(/\/\/[^\n]*/g,'')) && /xqWhy/.test(ex('irtBuildCsv')), '3a irtBuildCsv writes no QQQ KING row on the ES symbol (xqWhy says why)');

// ---------- 4. the record ----------
{
  const cl=fs.readFileSync('changelog/CHANGELOG.md','utf8'); ok(/^## v15\.85 /m.test(cl) && /XG2/.test(cl) && /SG2/.test(cl), '4a CHANGELOG v15.85 names XG / SG');
  const dc=fs.readFileSync('session-state/DECISIONS.md','utf8'); ok(/SG2 SG3/.test(dc) && /XG2 and XG3/.test(dc), '4b DECISIONS carries his words');
  let rec=null; try{ rec=JSON.parse(fs.readFileSync('learning/recommendations.json','utf8')); }catch(e){}
  const r26=rec && rec.rows.find(r=>r.id==='R-26'), r25=rec && rec.rows.find(r=>r.id==='R-25');
  ok(r26 && r26.status==='implemented' && r26.version==='15.85' && /XG2/.test(r26.text), '4c R-26 implemented in 15.85', r26);
  ok(r25 && r25.status==='proposed' && /tap record|TAP RECORD/i.test(r25.text), '4d R-25 (the tap record / level study) is proposed, not built', r25&&r25.status);
  ok(fs.existsSync('design/TAP-RECORD.md') && /nothing built/.test(fs.readFileSync('design/TAP-RECORD.md','utf8')), '4e the tap-record design exists and says nothing is built');
}

console.log('test_v1585: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
