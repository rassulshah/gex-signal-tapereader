// ============================================================================================
// test_v1561.js — (v15.61) THE LADDER NEVER SHOWS FEWER THAN LAD_MIN_ROWS STRIKES WHEN THE TAPE HAS THEM.
//   Live 2026-09-03 12:40 CT: 100 strikes on the SPXW tape, three at or above the 20% threshold, a three-bar ladder.
//   Sub-threshold CONTEXT rows are appended for DRAWING ONLY: the engine's node set is untouched.
// ============================================================================================
const fs=require('fs');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,300):''));} };
function ex(n){ const m=new RegExp('function\\s+'+n+'\\s*\\(','g').exec(src); if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index),d=0; for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(!d) return src.slice(m.index,k+1); } } }
const build=(g,fns,tail)=>new Function('__g', Object.keys(g).map(k=>'var '+k+'=__g.'+k+';').join('\n')+'\n'+fns.map(ex).join('\n')+'\n'+(tail||''));
// the live tape of 2026-09-03 12:40 CT, as probed
const PCT={ '7750.00':100, '7745.00':45, '7740.00':28, '7735.00':11, '7755.00':8, '7730.00':6, '7760.00':5, '7725.00':3, '7720.00':3, '7710.00':2, '7790.00':1, '7785.00':1, '7775.00':1, '7765.00':-1, '7715.00':-1, '7500.00':0, '7600.00':0 };
const g={ LAD_MIN_ROWS:8, ifLadder:()=>({ dispScale:1.0011 }), tapeMap:()=>({ pct:PCT, count:100, king:7750 }), emPos:()=>50 };
const subs=build(g,['ladderSubPiles'],'return ladderSubPiles;')(g);
const have=[{k:7750},{k:7745},{k:7740}];
let S=subs({ ok:true }, 'SPY', have);
ok(S.length===5 && S.map(x=>x.k).join(',')==='7735,7755,7730,7760,7725','1a three real nodes -> five context rows, the next strongest first, never one already drawn',S.map(x=>x.k));
ok(S.every(x=>x.sub===true && x.role===null) && S[0].pct===11 && S[0].brake===true && S[0].accel===false,'1b every context row is flagged sub, has no role, keeps its real %King and its polarity');
ok(subs({ ok:true },'SPY',[{k:1},{k:2},{k:3},{k:4},{k:5},{k:6},{k:7},{k:8}]).length===0,'1c eight real nodes -> no context rows');
ok(subs({ ok:true },'SPY',[{k:1},{k:2},{k:3},{k:4},{k:5},{k:6},{k:7},{k:8},{k:9}]).length===0,'1c2 …and nine (more than the minimum, tape strikes still unused) -> none either: the minimum is a floor, not a target');
ok(subs({ ok:false },'SPY',have).length===0,'1d no band -> nothing');
const g2=Object.assign({},g,{ tapeMap:()=>({ pct:{ '7750.00':100, '7745.00':45, '7740.00':28, '7735.00':0 }, count:4 }) });
ok(build(g2,['ladderSubPiles'],'return ladderSubPiles;')(g2)({ ok:true },'SPY',have).length===0,'1e a zero-%King strike is never a context row: the ladder shows what the tape has, not filler');
// the engine's node set is untouched: emPiles is not what draws the subs
ok(/var RAILPS_DRAW=RAILPS; try\{ var _subs=ladderSubPiles\(EB, sym, RAILPS\); if\(_subs\.length\) RAILPS_DRAW=RAILPS\.concat\(_subs\); \}catch\(eSub\)\{\}/.test(src) && /ladderHtml\(EB, RB, sym, RAILPS_DRAW,/.test(src),'1f the subs are appended to the DRAW list only; emPiles (the recorder, the READ, the rolls) is unchanged');
ok(!/emPiles\([^)]*\)\.concat\(ladderSubPiles/.test(src) && (src.match(/ladderSubPiles\(/g)||[]).length===2,'1g …and nothing else calls ladderSubPiles');
ok(/class="g3ldbar '\+k\+\(role==='KING'\?' king':''\)\+\(P\.sub\?' sub':''\)/.test(src) && /\.g3ldbar\.sub\{opacity:\.42/.test(src) && /CONTEXT ROW — below the/.test(src),'1h a sub row is drawn dimmed with a hover that says it is not a node');
console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
