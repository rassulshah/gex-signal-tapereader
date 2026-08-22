// (v11.38) LEADING-QUESTION HOVERS + THE THREE-ZONE CHART.
// The hover convention on this panel is question first, answer second — a trader should be able to
// point at anything and be told what it is FOR before being told what it is.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

// ---- every step tip opens with a question ----
{
  const m=src.match(/var STEP_TIPS=\[([\s\S]*?)\];/);
  ok(!!m,'the step tips exist');
  const tips=(m[1].match(/'(?:[^'\\]|\\.)*'/g)||[]).map(t=>t.slice(1,-1));
  ok(tips.length===5,'one per step',tips.length);
  tips.forEach((t,i)=>ok(/\?/.test(t.slice(0,120)),'step '+(i+1)+' opens with a question',t.slice(0,60)));
  ok(/PLAYBOOK IS LEGAL/i.test(tips[0]),'FRAME says which playbook is legal today');
  ok(/HOW MUCH SHOULD YOU TRUST/i.test(tips[1]),'BIAS pairs direction with confidence');
  ok(/AT levels, never between/i.test(tips[2]),'LOCATION says you trade at levels, not between them');
  ok(/step most people skip/i.test(tips[3]),'REACTION names itself as the step that gets skipped');
  ok(/blank here is a result/i.test(tips[4]),'EXECUTE says a refusal is a result');
}
// ---- the substantive hovers ask before they tell ----
{
  const musts=[
    [/Which way, and on whose authority\?/, 'the BIAS verdict'],
    [/How much conviction is behind this call\?/, 'the confirmation count'],
    [/Is anything structurally confirming the call\?/, 'the DRIFT gate'],
    [/Where is the day trying to go\?/, 'the TGT cell'],
    [/Is the market pricing near-term stress\?/, 'the TERM cell'],
    [/How much room does this tape need\?/, 'ATR'],
    [/How much room is left before structure\?/, 'CAGE'],
    [/Is this level being defended or abandoned\?/, 'NODE'],
    [/Did the level push price back\?/, 'PRICE'],
    [/Who is winning the bars right now\?/, 'PRESSURE'],
    [/Why is this trade refused\?/, 'a blocked EXECUTE'],
    [/What is armed, and what proves it wrong\?/, 'an armed EXECUTE'],
    [/How far through a level can price go/, 'the zone width'],
    [/What is this picture telling you\?/, 'the chart itself'],
    [/Is mass moving between strikes\?/, 'a roll'],
  ];
  musts.forEach(function(m){ ok(m[0].test(src), m[1]+' asks a question before it explains'); });
}
// ---- DEX explains that it maps rather than points ----
ok(/does not point|not as a direction/i.test(src),'the DEX hover says it maps hedging pressure rather than pointing');
// ---- the three zones ----
{
  ok(/IF · structure/.test(src),'the left zone is captioned as InsiderFinance structure');
  ok(/Skylit · flow/.test(src),'the right zone is captioned as Skylit flow');
  ok(/>GEX</.test(src) && />DEX</.test(src),'both structure columns are labelled');
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
