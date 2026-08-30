// v13.0 LAYOUT — four steps, TREND first, FRAME dissolved into LOCATION, NODES gutter moved left.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function noc(s){ return String(s).replace(/\/\*[\s\S]*?\*\//g,'').replace(/^\s*\/\/.*$/gm,''); }
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0,fail=0;
function ok(c,m,x){ if(c){pass++;} else {fail++; 
// ---- (v14.46) EVERY POST GETS ITS NUMBER ----------------------------------------------------
// Operator-caught: "there are more flags than strikes". Measured live 2026-08-26: 15 posts on the
// rail, 9 labels. v14.5's 4% neighbour-thinning blanked six of them, because adjacent 5-point
// strikes sit ~3.7% apart — just inside the rule. A post you cannot name is a post you cannot trade.
{
  const src=require('fs').readFileSync('./v10.js','utf8');
  ok(/EVERY POST GETS ITS\s*\n?\s*\/\/ NUMBER/.test(src) || /more flags than strikes/.test(src),
     'v1446a the operator\'s report is recorded where the fix lives');
  ok(/var LBLOK=\{\}, LBLTIER=\{\}, ROLEOK=\{\}/.test(src),
     'v1446b labels and roles are gated SEPARATELY — the number identifies the post, the role is a bonus');
  ok(/for\(var t=0;t<2;t\+\+\)/.test(src), 'v1446c labels are allocated across TWO tiers');
  ok(/g3plab\.g3t2\{bottom:-21px/.test(src), 'v1446d ...and tier 2 hangs a full label lower');
  ok(/g3emt\{margin-bottom:21px\}/.test(src),
     'v1446e the row BUYS the space, so the gamma profile is pushed down rather than written over');
  ok(/LBLTIER\[P\.k\]\?' g3t2':''/.test(src), 'v1446f the tier reaches the rendered class');

  // the allocator, executed: a ladder of evenly-spaced strikes must lose NO labels
  const alloc=(xs)=>{
    const tierX=[[],[]], out=[];
    xs.forEach((x)=>{
      for(let t=0;t<2;t++){
        let clash=false;
        for(let q=0;q<tierX[t].length;q++) if(Math.abs(tierX[t][q]-x)<4){ clash=true; break; }
        if(!clash){ tierX[t].push(x); out.push(t); return; }
      }
      out.push(null);
    });
    return out;
  };
  // the operator's actual rail: 15 posts, 3.7% apart
  const XS=[2.3,6,9.7,20.7,28,31.7,35.3,39,50,79.3,83,86.7,90.3,94,97.7];
  const T=alloc(XS);
  ok(T.filter(t=>t===null).length===0, 'v1446g on the live 15-post rail, NO label is dropped', T);
  ok(T.filter(t=>t===0).length>0 && T.filter(t=>t===1).length>0, 'v1446h ...they share both tiers');
  // and same-tier neighbours are never closer than the clearance
  let bad=0;
  for(let i=0;i<XS.length;i++) for(let j=i+1;j<XS.length;j++)
    if(T[i]===T[j] && Math.abs(XS[i]-XS[j])<4) bad++;
  ok(bad===0, 'v1446i no two labels on the same tier are within 4% — nothing can mash', bad);
  // a pathological cluster still refuses rather than overlapping
  const C=alloc([50,50.5,51,51.5]);
  ok(C[3]===null, 'v1446j four labels inside 2% still drops the last — refusal beats overlap', C);
}

console.log('FAIL '+m+(x!==undefined?'  got: '+x:''));} }

// ---------- the step bar ----------
{
  const names=/var STEP_NAMES=\[([^\]]+)\]/.exec(src)[1];
  const short=/var STEP_SHORT=\[([^\]]+)\]/.exec(src)[1];
  // (v14.32, operator-directed) REACTION and EXECUTE retired from the face — two steps remain.
  ok(names.split(',').length===2, 'two steps (v14.32: REACTION/EXECUTE retired from display)', names.split(',').length);
  ok(/① TREND/.test(names) && /① TREND/.test(short), 'TREND is step ①');
  ok(!/FRAME/.test(names), 'FRAME is not a step any more');
  ok(!/BIAS/.test(names), 'and BIAS is renamed, not merely reordered');
  ok(/② LOCATION/.test(names) && !/REACTION/.test(names) && !/EXECUTE/.test(names),
     'LOCATION is step ② and the retired steps are gone from the bar (v14.32)');
  const tips=/var STEP_TIPS=\[([\s\S]*?)\];/.exec(src)[1];
  ok(tips.split(/',\s*\n/).length>=2, 'tips cover the rendered steps (extras tolerated for the retired sections)');
}
// ⚠ the loops must be LENGTH-DRIVEN, or a future step count silently truncates the bar
{
  const pv=ex('panelV3');
  // ⚠⚠ (v14.84) THE STEP BAR IS GONE — operator: "why is trend and location at the top headers.
  // lets remove that too." This pinned that the chip loop was LENGTH-driven so a changed step count
  // could not silently truncate the bar. There is no bar. The assertion now pins the removal, so
  // reinstating a step bar turns it red and the length-driven rule has to be re-made deliberately.
  ok(!/i<STEP_SHORT\.length/.test(pv) && !/g3steps/.test(pv),
     'the step bar is gone from panelV3 — no chip loop, no g3steps container');
  ok(!/g3sh '\+c\+'/.test(pv), '...and no per-section header row either');
  // ⚠ the VOCABULARY survives even though nothing renders it: STEP_TIPS carried the only written
  // statement of what each section is FOR. That is doctrine, and deleting it with the header would
  // have lost it silently.
  // ⚠⚠ FAKE ON FIRST WRITING, caught by mutation: it grepped the WHOLE FILE for the doctrine line,
  // and the comment I had just written above panelV3 QUOTES that line — so gutting STEP_TIPS left
  // the assertion green, satisfied by the comment describing the thing it was meant to protect.
  // Bind to the array's own text.
  const TIPS=(/var STEP_TIPS=\[[\s\S]*?\];/.exec(src)||[''])[0];
  ok(TIPS.length>0 && /you trade AT levels, never between them/i.test(TIPS),
     '...but STEP_TIPS is KEPT — the doctrine it carries is not the header it was attached to',
     TIPS.length);
  ok(/j<secs\.length/.test(pv), 'and so is the section loop');
  // (v15.01) ONE section renders. TREND came off the face on his instruction — "even the dntend
  // Brk area was also removed" — and the evidence backed it: DRIFT is a 50.0% coin flip on n=68,
  // test_trendbadge is permanently red, and the confirm tally was never scored against TREND's 34%.
  // secBias() is KEPT so bias.confirm keeps feeding the recorder.
  // ⚠ `pv` is the panelV3 BODY — `function secBias` is declared outside it, so requiring both in
  // pv checked a scope that could never hold them. Mount site in pv, survival in the whole source.
  ok(/var secs=\[secLoc\]/.test(pv), 'one section renders (v15.01: TREND off the face)');
  ok(/function secBias/.test(src), '...and secBias survives, so bias.confirm keeps feeding the recorder');
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
