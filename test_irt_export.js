// (v11.4) IRT FLEXLEVELS EXPORT — CSV format pinned to the user's own FlexLevelsExport.csv sample.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'=[\\s\\S]*?;\\n'))[0]; }
global.window={__gptsDebug:{}};
eval(v('IRT_HEADER')); eval(v('IRT_LAST'));
eval(['irtColor'].map(ex).join('\n')); eval(v('IRT_COLORS'));
eval(v('IRT_RATIO_KEY'));
eval(['irtRound','irtCsvRow','irtRatio','irtBuildCsv'].map(ex).join('\n'));
global.ES_RATIO=10.05; global.LASTFEED={SPY:null};
var LS={}; global.localStorage={ getItem:k=>(k in LS?LS[k]:null), setItem:(k,val)=>{LS[k]=String(val);} };
global.CFG={ nodeThresh:20, irt:{ on:true, secs:180, futSym:'EPU26', etfSym:'SPY', file:'FlexLevelsExport.csv' } };
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
global.nodeMapModel=()=>({ ok:true, levels:[
  {k:774, pct:100, isKing:true, pos:true},
  {k:771.5, pct:44, isGatekeeper:true, pos:true},
  {k:775, pct:59, isCeil:true, pos:true},
  {k:770.5, pct:71, isFlr:true, pos:true},
  {k:777, pct:31, isStrongMag:true, pos:false},
  {k:768.5, pct:100, isCeil:true, pos:true, derived:true, src:'SPXW'},   // a derived lane at ITS OWN 100%
  {k:780, pct:12, pos:true},                       // below thresh, no role → dropped
]});
global.nextStopPick=()=>({ok:true, level:776, grade:'C'});
global.pbEntryPick=()=>({ok:true, level:771.5, grade:'B', state:'acm'});

const b=irtBuildCsv();
ok(!!b, '0 builds');
const lines=b.csv.trim().split('\r\n');
ok(lines[0]==='SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH,PENSTYLE,bDRAWTEXT,bDRAWPRICE,LABELPOS,bCUSTPOS,CUSTPOSALLMARGIN,CUSTPOSLEFTRIGHT,CUSTPOSUNITS,CUSTPOSWIDTH,bBANDS,BANDPENCOLOR,BANDPENWIDTH,BANDPENSTYLE,BANDABOVEBEL,BANDUNITS,BANDPRICE,bBANDS2,BAND2PENCOLOR,BAND2ABOVEBEL,BAND2UNITS,BAND2PRICE,bBANDLABELS,bTRANSLUCENT', '1a header verbatim from the sample');
ok(b.n===8 && lines.length===1+8*2, '1b 8 levels (King, GK, Ceil, Flr, −γ Mag, SPXW lane, NextStop, PBentry; 12% node dropped) × 2 symbols', [b.n,lines.length]);
const eRows=lines.filter(l=>l.startsWith('EPU26,')); const sRows=lines.filter(l=>l.startsWith('SPY,'));
ok(eRows.length===8 && sRows.length===8, '1c both symbols written', [eRows.length,sRows.length]);
const king=eRows.find(l=>/K 100%/.test(l));
ok(king && /^EPU26,7781\.750000,/.test(king), '1d King 774 × 10.0538 = 7781.66 → rounded to the 0.25 tick = 7781.75, six decimals', king);
const kingSpy=sRows.find(l=>/K 100%/.test(l));
ok(kingSpy && /^SPY,774\.000000,/.test(kingSpy), '1e SPY rows at raw strike');
ok(king.split(',').length===28, '1f exactly 28 columns like the sample', king.split(',').length);
const neg=eRows.find(l=>/Mag 31% -g/.test(l));
ok(neg && neg.split(',')[3]===String((247<<16)+(113<<8)+163), '1g −γ magnet carries the purple COLORREF (BGR)', neg&&neg.split(',')[3]);
ok(/NextStop C/.test(b.csv) && /PBentry B acm/.test(b.csv), '1h the two forward calls ride along, dashed (style 2)');
const nsRow=eRows.find(l=>/NextStop/.test(l)); ok(nsRow.split(',')[5]==='2', '1i dashed pen style on NextStop');
// live=false → approx tag
global.FUTMODE={ fam:'ES', r:10.0538, live:false };
const b2=irtBuildCsv();
ok(/K 100% ~/.test(b2.csv.split('\r\n').filter(l=>l.startsWith('EPU26'))[0]||'') || /~/.test(b2.csv), '2a last-known ratio marks futures labels with ~');
// no symbols configured → null
global.CFG.irt.futSym=''; global.CFG.irt.etfSym='';
ok(irtBuildCsv()==null, '2b no symbols set → nothing written (never a wrong-symbol file)');
// label sanitation: commas can never break the CSV
global.CFG.irt.etfSym='SPY';
global.nextStopPick=()=>({ok:true, level:776, grade:'C,evil'});
const b3=irtBuildCsv();
ok(b3.csv.split('\r\n').every(l=>l==='' || l.split(',').length===28 || l===lines[0]), '2c commas in labels are stripped — every row keeps 28 columns');
// ---- 3. (v11.4.1) the ratio must not depend on what is charted
global.CFG.irt.futSym='EPU26'; global.CFG.irt.etfSym='';
global.FUTMODE={ fam:'ES', r:10.0538, live:true };
irtBuildCsv();                                              // charting ES: persists the live ratio
global.FUTMODE={ chart:'SPY', fam:null, r:1, live:true };   // user switches back to the SPY cash chart
const b4=irtBuildCsv();
ok(!!b4 && b4.ratio.src==='last-good' && Math.abs(b4.ratio.r-10.0538)<0.001, '3a on a CASH chart the export still writes, using the persisted ES ratio', b4&&b4.ratio);
ok(/^EPU26,7781\.750000,/.test(b4.csv.split('\r\n')[1]), '3b ...same converted price as when ES was charted');
ok(/ ~/.test(b4.csv), '3c ...marked ~ because the ratio is not live');
LS={};                                                       // nothing persisted
global.LASTFEED={ SPY:{ j:{ derived:[{source:'SPXW', ratio:0.09974500868055555}] } } };
const b5=irtBuildCsv();
ok(b5 && b5.ratio.src==='spxw-derived' && Math.abs(b5.ratio.r-10.0256)<0.01, '3d no persisted ratio → the feed’s own SPXW→SPY ratio is used', b5&&b5.ratio);
global.LASTFEED={SPY:null};
const b6=irtBuildCsv();
ok(b6 && b6.ratio.src==='const' && b6.ratio.r===10.05, '3e last resort: the ES constant', b6&&b6.ratio);
// ---- 4. (v11.4.3) an SPXW-derived lane never wears a SPY role word or the King's weight
global.CFG.irt.futSym='EPU26'; global.CFG.irt.etfSym='SPY'; global.FUTMODE={ fam:'ES', r:10.0538, live:true };
const b7=irtBuildCsv();
const dRow=b7.csv.split('\r\n').filter(l=>l.startsWith('SPY,')&&/768\.500000/.test(l))[0];
ok(!!dRow && /SPXW 100%/.test(dRow), '4a the derived lane is labelled "SPXW 100%" — not "Ceil 100%" beside the real King', dRow&&dRow.split(',')[2]);
ok(dRow.split(',')[4]==='1' && dRow.split(',')[5]==='1', '4b ...thin and dotted, so it cannot be mistaken for a SPY wall', dRow&&[dRow.split(',')[4],dRow.split(',')[5]]);
ok(dRow.split(',')[3]===String((130<<16)+(110<<8)+90), '4c ...in its own slate colour');
ok(/K 100%/.test(b7.csv) && b7.csv.split('\r\n').filter(l=>/,K 100%/.test(l)).length===2, '4d exactly one real King per symbol');
// (v11.25) the gamma level set and their chain lane both reach the chart now
global.lvlUnified=()=>({ px:762.5, rows:[{id:'CR',k:765},{id:'HVL',k:766},{id:'PS·Mag',k:760},{id:'PS0',k:762,tag:'0DTE'}] });
global.ifChainRows=()=>({ rows:[{id:'CR',k:775},{id:'Mag',k:760},{id:'PS',k:760},{id:'MP',k:758}] });

// ---------- (v11.25) THE LEVELS REACH INVESTOR/RT ----------
// The user's brief: the chart must carry the same reads as the card. Both lanes export, and every one of
// THEIR lines is tagged IF so nothing on the chart is ambiguous about which measurement drew it.
{
  const B=irtBuildCsv();
  const csv=B&&B.csv||'';
  ok(/CR/.test(csv), '5a our CR reaches the file');
  ok(/HVL/.test(csv), '5b our HVL reaches the file');
  ok(/PS0/.test(csv), '5c and the 0DTE variant');
  ok(/PS.Mag/.test(csv), '5d including a merged label');
  ok(/IF CR/.test(csv), '5e THEIR call wall, tagged IF');
  ok(/IF MP/.test(csv), '5f and Max Pain — impossible from the Skylit feed, free from their chain');
  ok(/IF PS/.test(csv), '5g and their put support');
  const lines=csv.split('\r\n').filter(Boolean);
  ok(lines.filter(l=>/IF /.test(l)).length>=4, '5h every one of their levels is tagged, never bare');
}
{
  global.ifChainRows=()=>null;
  const B=irtBuildCsv();
  ok(B!==null, '5i no companion installed → the export still builds');
  ok(!/IF /.test(B.csv), '5j and carries none of their lines');
  ok(/HVL/.test(B.csv), '5k while ours are unaffected');
  global.ifChainRows=()=>({ rows:[{id:'CR',k:775},{id:'Mag',k:760},{id:'PS',k:760},{id:'MP',k:758}] });
}
{
  global.lvlUnified=()=>null;
  const B=irtBuildCsv();
  ok(B!==null, '5l no level set → the export still builds rather than throwing');
  ok(/IF CR/.test(B.csv), '5m their lane survives independently');
  global.lvlUnified=()=>({ px:762.5, rows:[{id:'CR',k:765},{id:'HVL',k:766},{id:'PS·Mag',k:760},{id:'PS0',k:762,tag:'0DTE'}] });
}
console.log('test_irt_export: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
