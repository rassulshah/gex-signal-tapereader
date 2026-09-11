// test_v1596.js — (v15.96) THE KING CHART — price vs the SPX/SPY Kings as step lines, in the node-ladder section.
//
// Operator, 2026-09-11: "a 3 min candle chart that display spy and spx king lines as steps, tracking their movements and
// when and where they deflected or price broke through." Design + the factor backlog: design/KING-STUDY.md. This test pins
// the piece that carries the meaning — the deflect/break detection, done the doctrine way (the WICK tests the line, the
// CLOSE decides) — plus the honest scale (SPY exact, SPX approx ~), the n-gated factor shell, and the view toggle wiring.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

ok(/@version\s+15\.9[6-9]/.test(src) && /var GPTS_VERSION='15\.9[6-9]';/.test(src), '0a v15.96 or later in both spots');

// ---------- 1. the deflect/break detection — the WICK tests, the CLOSE decides ----------
{
  global.mul=(a,b)=>a*b;
  global.DEFL_NEAR=1;
  global.displayScale=()=>({scale:1});     // ES == SPY scale here, so bar prices are used as-is
  global.FUTMODE={r:1};
  eval(ex('kingChartRR'));
  eval(ex('kingChartEvents'));
  // a flat SPY King at 765; SPX line absent (all null → no SPX events)
  const KL=[765,765,765,765,765], NUL=[null,null,null,null,null];
  const bars=[
    {t:1,o:767,h:768,l:766,c:767},   // above, no touch
    {t:2,o:767,h:767,l:764.5,c:766}, // wick to 764.5 (tests 765), closes 766 back above → DEFLECT
    {t:3,o:766,h:766,l:763,c:763.5}, // closes 763.5 below 765 → crossed → BREAK
    {t:4,o:763,h:762,l:760,c:761},   // well below, high 762 never reaches 764 → NO event
    {t:5,o:761,h:761.5,l:760,c:761}  // still below, no touch
  ];
  const ev=kingChartEvents(bars, KL, NUL, 1);
  ok(ev.length===2, '1a two events found on the SPY line — the untested bars produce nothing', ev.map(e=>e.type));
  ok(ev[0] && ev[0].i===1 && ev[0].type==='deflect' && ev[0].book==='SPY', '1b bar 1: wick tested the line, close returned to the same side → DEFLECT (held)', ev[0]);
  ok(ev[1] && ev[1].i===2 && ev[1].type==='break', '1c bar 2: the close flipped to the other side of the line → BREAK', ev[1]);
  ok(ev.every(e=>e.book!=='SPX'), '1d the SPX line was all-null, so it contributes no events (no phantom crossings)', ev.map(e=>e.book));
  // a bar whose wick never reaches the line is never an event
  const ev2=kingChartEvents([{t:1,o:770,h:771,l:769,c:770},{t:2,o:770,h:771,l:769,c:770}], [765,765], [null,null], 1);
  ok(ev2.length===0, '1e price that never comes within ~1 ATR of the King is not a tap', ev2);
}

// ---------- 2. the chart is wired to REAL recorded data, at honest scale ----------
{
  const kh=ex('kingChartHtml'), kb=ex('kingChartBars');
  ok(/kingAt\('SPY',\s*bars\[i\]\.t\)/.test(kh) && /kingAt\('SPXW',\s*bars\[i\]\.t\)/.test(kh), '2a the step lines read kingAt(book, bar.t) for BOTH books — the recorded King journey');
  ok(/closedCandles\('SPY'\)/.test(kb), '2b the candles are the recorder’s own 3-minute bars (closedCandles)');
  ok(/x\*spxD/.test(kh) && /kingChartSpxDisp/.test(src) && /SPY K ~/.test(ex('kingChartSvg')), '2c SPY King exact (a\\*rr); SPX King approx via the current dispScale, and the line wears ~ (honest scale)');
  ok(/wick tests, close decides/.test(kh), '2d the ledger says how a tap is judged — the doctrine rule, not a guess');
}

// ---------- 3. the stats layer is honest — no rate without its n ----------
{
  const kh=ex('kingChartHtml');
  ok(/recording · n=0/.test(kh) && /n ≥ 15 with a Wilson low/.test(kh), '3a the factor table is n-gated — every factor reads "recording · n=0" until it fills');
  ok(/doctrine/.test(kh) && /prov · n=4/.test(kh), '3b the insights carry their source — doctrine (the Academy prior) and the one provisional finding with its n');
  ok(/design\/KING-STUDY\.md/.test(kh), '3c it points at the factor backlog (design/KING-STUDY.md) — his roll / growth / pika / polarity / confluence / trend / time-of-day list');
}

// ---------- 4. the view toggle (grid ⇄ King chart) is wired end to end ----------
{
  ok(/ladderView: 'grid'/.test(src), '4a CFG.ladderView defaults to the grid — the King chart is opt-in');
  ok(/o\.ladderView==='grid'\|\|o\.ladderView==='kingchart'/.test(src), '4b …and it persists (the load accepts both values)');
  ok(/class="gpts-ladview" data-view="/.test(src) && /_kv==='kingchart'/.test(src), '4c the section renders the toggle and branches to kingChartHtml when chosen');
  ok(/t\.getAttribute\('data-view'\)/.test(src) && /CFG\.ladderView=\(vw==='kingchart'\)/.test(src), '4d the body delegation handles the toggle click — saves and re-renders');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
