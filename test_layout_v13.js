// v13.0 LAYOUT — four steps, TREND first, FRAME dissolved into LOCATION, NODES gutter moved left.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function noc(s){ return String(s).replace(/\/\*[\s\S]*?\*\//g,'').replace(/^\s*\/\/.*$/gm,''); }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0,fail=0;
function ok(c,m,x){ if(c){pass++;} else {fail++; console.log('FAIL '+m+(x!==undefined?'  got: '+x:''));} }

// ---------- the step bar ----------
{
  const names=/var STEP_NAMES=\[([^\]]+)\]/.exec(src)[1];
  const short=/var STEP_SHORT=\[([^\]]+)\]/.exec(src)[1];
  ok(names.split(',').length===4, 'four steps, not five', names.split(',').length);
  ok(/① TREND/.test(names) && /① TREND/.test(short), 'TREND is step ①');
  ok(!/FRAME/.test(names), 'FRAME is not a step any more');
  ok(!/BIAS/.test(names), 'and BIAS is renamed, not merely reordered');
  ok(/② LOCATION/.test(names) && /③ REACTION/.test(names) && /④ EXECUTE/.test(names),
     'the remaining three renumber to 2,3,4');
  const tips=/var STEP_TIPS=\[([\s\S]*?)\];/.exec(src)[1];
  ok(tips.split("',\n").length===4 || tips.split(/',\s*\n/).length===4, 'one tip per step');
}
// ⚠ the loops must be LENGTH-DRIVEN, or a future step count silently truncates the bar
{
  const pv=ex('panelV3');
  ok(/i<STEP_SHORT\.length/.test(pv), 'the chip loop is driven by the array length, not a literal 5');
  ok(/j<secs\.length/.test(pv), 'and so is the section loop');
  ok(/var secs=\[secBias, secLoc, secReact, secExec\]/.test(pv), 'four sections render');
  ok(!/secFrame/.test(noc(pv).replace(/var secs=.*/,'')), 'panelV3 does not render secFrame as a section');
}
// ---------- FRAME moved INSIDE location ----------
{
  const sl=ex('secLoc');
  ok(/var h=secFrame\(sym\)\+'<div class="g3b">'/.test(sl),
     'secLoc OPENS with secFrame — badges, target, rail and read line lead the section');
  ok(typeof ex('secFrame')==='string' && ex('secFrame').length>200,
     'and secFrame still exists as a renderer, so its ~15 assertions elsewhere still bind');
}
// ---------- the levels block is HIDDEN, and hiding it did not disarm the panel ----------
{
  const sl=ex('secLoc');
  ok(/var LOC_SHOW_LEVELS=false;/.test(src), 'the level rows are switched off');
  ok(/if\(LOC_SHOW_LEVELS\) h\+='<div class="g3r'/.test(sl), 'the level row HTML is guarded');
  ok(/if\(LOC_SHOW_LEVELS\) h\+='<div class="g3prow"/.test(sl), 'and so is the price row');
  // ⚠⚠ THE POINT. G3_AT_LEVEL feeds steps ③ and ④. If the loop had been deleted to hide the rows,
  // REACTION and EXECUTE would never light again — a display change silently disarming the trade.
  ok(/if\(near && !atLevel\) atLevel=r;/.test(noc(sl)),
     'the loop STILL RUNS and still finds atLevel, which is what arms REACTION and EXECUTE');
  ok(/G3_AT_LEVEL/.test(sl), 'and G3_AT_LEVEL is still set from it');
  ok(!/if\(LOC_SHOW_LEVELS\)[\s\S]{0,40}atLevel=r/.test(sl),
     'atLevel is NOT inside the display guard');
}
// ---------- stepState ----------
{
  const ss=ex('stepState');
  ok(/var st=\[false,false,false,false\], wait=''/.test(ss), 'four step flags');
  ok(/st\[0\]=\(B\.live>=3 && B\.dir!==0\)/.test(ss), 'step 0 is the trend vote');
  ok(/if\(R\.g==null\) wait='waiting on the <b>gamma book<\/b>'/.test(ss),
     'the gamma book still gates the WAIT message even though it is no longer a step');
  ok(/all four steps satisfied/.test(ss), 'and the armed message counts four');
  ok(/i<st\.length/.test(ss), 'the cursor loop follows the array, not a literal');
}
// ---------- the chart: NODES gutter moved left, IF profiles removed ----------
{
  const nc=ex('nodeChartHtml');
  ok(/var SX=3, SW=56;/.test(nc), 'the flow gutter is on the LEFT');
  ok(/var PL=64, PR=410;/.test(nc), 'and the price plot takes back the width it vacated');
  ok(!/var GX=|var DX=/.test(noc(nc)), 'the GEX and DEX columns are gone');
  ok(!/drawProf/.test(noc(nc)), 'and so is the profile drawer that fed them');
  // ⚠ direction: on the LEFT, growing rightward is what points the bars at price.
  ok(/g\+='<rect x="'\+\(SXA\+from\)/.test(nc), 'bars anchor at the left edge and grow toward price');
  ok(!/SXR/.test(noc(nc)), 'the old right-edge anchor is gone, not merely unused');
  ok(/>NODES<\/text>/.test(nc), 'the gutter is captioned NODES');
  ok(!/>GEX<\/text>/.test(nc) && !/>DEX<\/text>/.test(nc), 'and GEX/DEX captions are gone');
  ok(!/IF · structure/.test(nc), 'as is the IF structure caption');
}
console.log('\n'+pass+' pass / '+fail+' fail');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
