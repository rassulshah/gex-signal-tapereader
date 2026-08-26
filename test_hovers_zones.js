// (v11.38) LEADING-QUESTION HOVERS + THE THREE-ZONE CHART.
// The hover convention on this panel is question first, answer second — a trader should be able to
// point at anything and be told what it is FOR before being told what it is.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){
  const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) throw new Error('function not found: '+n);
  let i=src.indexOf('{',m.index), d=0, e=-1;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0){ e=k; break; } } }
  return src.slice(m.index,e+1);
}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

// ---- every step tip opens with a question ----
{
  const m=src.match(/var STEP_TIPS=\[([\s\S]*?)\];/);
  ok(!!m,'the step tips exist');
  const tips=(m[1].match(/'(?:[^'\\]|\\.)*'/g)||[]).map(t=>t.slice(1,-1));
  // (v13.0) FOUR steps: ① FRAME dissolved into ② LOCATION, ② BIAS renamed ① TREND and moved first.
  ok(tips.length===4,'one per step',tips.length);
  tips.forEach((t,i)=>ok(/\?/.test(t.slice(0,120)),'step '+(i+1)+' opens with a question',t.slice(0,60)));
  ok(/HOW MUCH SHOULD YOU TRUST/i.test(tips[0]),'TREND pairs direction with confidence, and leads');
  ok(/AT levels, never between/i.test(tips[1]),'LOCATION says you trade at levels, not between them');
  ok(/step most people skip/i.test(tips[2]),'REACTION names itself as the step that gets skipped');
  ok(/blank here is a result/i.test(tips[3]),'EXECUTE says a refusal is a result');
  // ⚠ the regime lesson must SURVIVE the loss of its own step, or dissolving FRAME quietly deletes it
  ok(/negative gamma means breaks work/i.test(tips[0]),
     'and the regime playbook it inherited from FRAME is still taught somewhere');
}
// ---- the substantive hovers ask before they tell ----
{
  const musts=[
    [/Which way, and on whose authority\?/, 'the BIAS verdict'],
    [/How much conviction is behind this call\?/, 'the confirmation count'],
    [/Is anything structurally confirming the call\?/, 'the DRIFT gate'],
    [/Where is the day trying to go\?/, 'the TGT cell'],
    // ⚠ TERM and ATR are NOT missing hovers — the CELLS were removed at v11.49, when FRAME line 2
    // stopped being four naked measurements (DEX, TERM, EM, ATR) and became the anchored band.
    // They are struck out here rather than deleted so nobody re-adds them thinking they were lost.
    [/Is it being defended or abandoned\?/, 'NODE'],
    [/What is being watched, and why that\?/, 'the WATCH row'],
    [/Where can a trade actually happen\?/, 'the NODES block'],
    [/Did it push price back\?/, 'PRICE'],
    [/Who is winning the bars right now\?/, 'PRESSURE'],
    [/Why is this refused\?/, 'a blocked EXECUTE'],
    [/What is armed, and what proves it wrong\?/, 'an armed EXECUTE'],
    [/Which node, and is it building or bleeding\?/, 'the node line'],
    [/What is this level and how far away\?/, 'a ladder row'],
    [/Where is price sitting in the level set\?/, 'the price row'],
    [/Which book, which window, and how old\?/, 'the SET line'],
    [/How far through a level can price go/, 'the zone width'],
    [/What is this picture telling you\?/, 'the chart itself'],
    [/Is mass moving between strikes\?/, 'a roll'],
  ];
  musts.forEach(function(m){ ok(m[0].test(src), m[1]+' asks a question before it explains'); });
}
// ---- DEX: the cell went with the same v11.49 rewrite ----
// The claim it guarded (a map of hedging pressure, never a direction) now lives on the read sentence,
// which test_em_band.js S30 enforces by EXECUTING the composer and grepping the emitted text for
// forecast vocabulary -- a stronger check than this one ever was.
// ⚠ Check the RENDERED cells, not the source text. DEX/TERM/ATR each still appear inside the v11.49
// comment that explains their removal — the fourth time in this project a removal comment has made a
// test think the removed thing was still there. Strip comments first, then look.
{
  const FR=ex('secFrame')
    .replace(/\/\*[\s\S]*?\*\//g,'')
    .split('\n').map(l=>l.replace(/^\s*\/\/.*$/,'')).join('\n');
  ['DEX','TERM','ATR'].forEach(w=>
    ok(!new RegExp("'"+w+"'|>"+w+"<").test(FR),
       w+' is gone from the FRAME face, so there is no un-caveated '+w+' cell to guard'));
}
// ---- the three zones ----
{
  // --- INVERTED (v13.0): the IF structure profiles are gone; the left gutter is Skylit NODE flow ---
  ok(!/IF · structure/.test(src),'the IF structure caption is gone with its columns');
  ok(/Skylit · flow/.test(src),'the right zone is captioned as Skylit flow');
  ok(!/>GEX<\/text>/.test(src) && !/>DEX<\/text>/.test(src),
     'and neither GEX nor DEX is drawn any more (INVERTED v13.0)');
  ok(/>NODES<\/text>/.test(src),'the left gutter is captioned NODES');
  ok(/>NODES</.test(src),'and the flow column is labelled');
  ok(/60m and 15m/.test(src)||/60 and 15 minutes/.test(src),'the growth ticks are explained as 60m and 15m');
}
// ---- centred labels free BOTH gutters ----
{
  const fn=src.slice(src.indexOf('function centreLvl'), src.indexOf('function centreLvl')+900);
  ok(/text-anchor="middle"/.test(fn),'level labels are centre-anchored');
  const lines=(fn.match(/<line x1=/g)||[]).length;
  ok(lines>=2,'the level line is drawn in TWO segments so it breaks around the text instead of running under it',lines);
}
// ---- the SMA is drawn, and from the same source the verdict uses ----
{
  ok(/contSMAAtTodayIdx\(sym, period, off\+ix\)/.test(src),
     'the SMA line walks the SAME continuous series trendVerdict reads, so the line and the call cannot disagree');
  ok(/off=allC\.length-bars\.length/.test(src),'and the index offset is carried through, since bars is a tail slice');
}
console.log('\n'+pass+' pass / '+fail+' fail');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
