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
  {k:780, pct:12, pos:true},                       // below thresh, no role → dropped
]});
global.nextStopPick=()=>({ok:true, level:776, grade:'C'});
global.pbEntryPick=()=>({ok:true, level:771.5, grade:'B', state:'acm'});

const b=irtBuildCsv();
ok(!!b, '0 builds');
const lines=b.csv.trim().split('\r\n');
ok(lines[0]==='SYMBOL,PRICE,LABEL,PENCOLOR,PENWIDTH,PENSTYLE,bDRAWTEXT,bDRAWPRICE,LABELPOS,bCUSTPOS,CUSTPOSALLMARGIN,CUSTPOSLEFTRIGHT,CUSTPOSUNITS,CUSTPOSWIDTH,bBANDS,BANDPENCOLOR,BANDPENWIDTH,BANDPENSTYLE,BANDABOVEBEL,BANDUNITS,BANDPRICE,bBANDS2,BAND2PENCOLOR,BAND2ABOVEBEL,BAND2UNITS,BAND2PRICE,bBANDLABELS,bTRANSLUCENT', '1a header verbatim from the sample');
ok(b.n===7 && lines.length===1+7*2, '1b 7 levels (King, GK, Ceil, Flr, −γ Mag, NextStop, PBentry; 12% node dropped) × 2 symbols', [b.n,lines.length]);
const eRows=lines.filter(l=>l.startsWith('EPU26,')); const sRows=lines.filter(l=>l.startsWith('SPY,'));
ok(eRows.length===7 && sRows.length===7, '1c both symbols written');
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
console.log('test_irt_export: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
