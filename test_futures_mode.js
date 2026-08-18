// (v10.55 PART E) FUTURES MODE — the chart may be a future, and then every level the
// panel draws must be spoken in that future's points.
//
// The rules being pinned here:
//   · the chart symbol is detected from the page each render (title, then header ticker)
//   · ES/MES -> the SPY tape · NQ/MNQ -> the QQQ tape · SPY/QQQ -> themselves
//   · ANYTHING ELSE -> "No options tape for <SYM> — levels unavailable", never a level
//   · r = futPrice / underlyingPrice, EMA-smoothed, with an honest fallback chain
//     (live -> last good -> constant) and an ≈ on EVERY converted number once it is
//     no longer live
//   · only the DISPLAY converts: recording stays in the underlying's strikes
//   · the futures chart's candles are converted back by 1/r so trend / in-play / drift
//     / R:R keep working
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

global.ES_RATIO=10.0676;
global.PAL={ card:'#12161f', line:'#1e2530', amber:'#f2b45a', sub:'#8b98a9', ink:'#e6edf3', blue:'#4a90d9', bg:'#0b0e14' };
global.LASTFEED={ SPY:null, QQQ:null };
global.STATE={ SPY:{ price:771.5, candles:[] }, QQQ:{ price:565.0, candles:[] } };
var TITLE='';
global.document={ title:'', querySelectorAll:function(){ return []; } };
var HEADERS=null;
global.readTrinityHeaders=function(){ return HEADERS; };
var LS={};
global.localStorage={ getItem:function(k){ return (k in LS)?LS[k]:null; }, setItem:function(k,v){ LS[k]=String(v); } };
global.readFiberCandles=function(sym){ return (FIBER[sym]||null); };
var FIBER={};
global.FUT_KNOWN=['MES','MNQ','ES','NQ','SPY','QQQ'];
global.FUTR={ ES:{ema:null,last:null,t:0}, NQ:{ema:null,last:null,t:0} };
global.FUT_FAMILY={ ES:'ES', MES:'ES', NQ:'NQ', MNQ:'NQ' };
global.FUT_UNDERLYING={ ES:'SPY', MES:'SPY', SPY:'SPY', NQ:'QQQ', MNQ:'QQQ', QQQ:'QQQ' };
global.NQ_RATIO=41.36;
global.FUT_CONST={ ES:ES_RATIO, NQ:41.36 };
global.FUT_EMA_A=0.25;
global.FUTCFG={ mode:'auto' };
global.FUTMODE={ chart:'SPY', fam:null, underlying:'SPY', r:1, live:true, approx:false, ok:true };

global.FUT_TICK={ES:0.25,NQ:0.25};
eval(['mul','fmtNum','futTick','fmtFut','futCfgLoad','futCfgSave','chartSymFromText','chartPxFromText',
      'futRatioStep','feedUnderlyingPx','futDetect','futModeCompute','futModeRefresh','futMode',
      'activeSym','dispIsFut','dispR','dispVal','futMark','fmtLvl','fmtSpan',
      'futCandlesToUnderlying','futRawCandles','futUnderlyingPx','futRatioText','futUnavailableHtml'].map(ex).join('\n'));

// ================= 1. SYMBOL DETECTION ============================================
ok(chartSymFromText('ES1 $7768.75')==='ES', '1a the spec\'s own title form', chartSymFromText('ES1 $7768.75'));
ok(chartPxFromText('ES1 $7768.75')===7768.75, '1b ...and its price', chartPxFromText('ES1 $7768.75'));
ok(chartSymFromText('MNQ1! $24,120.25')==='MNQ', '1c micro contracts', chartSymFromText('MNQ1! $24,120.25'));
ok(chartPxFromText('MNQ1! $24,120.25')===24120.25, '1d ...with a thousands separator');
ok(chartSymFromText('SPY $771.50')==='SPY' && chartSymFromText('QQQ $565.10')==='QQQ', '1e the cash symbols');
ok(chartSymFromText('GC1 $2465.10')==='GC', '1f an instrument we cannot map is still IDENTIFIED (so it can be named)', chartSymFromText('GC1 $2465.10'));
ok(chartSymFromText('')===null && chartSymFromText(null)===null, '1g nothing in, nothing out');

// ================= 2. MAPPING ====================================================
function mode(title, headers, forced){
  global.document.title=title; HEADERS=headers||null;
  global.FUTCFG={ mode:forced||'auto' };
  return futModeCompute();
}
var m=mode('ES1 $7768.75');
ok(m.chart==='ES' && m.underlying==='SPY' && m.ok===true, '2a ES maps to the SPY tape', m.chart+'→'+m.underlying);
ok(mode('MES1 $7768.75').underlying==='SPY', '2b MES too');
ok(mode('NQ1 $23,370.00').underlying==='QQQ', '2c NQ maps to the QQQ tape');
ok(mode('MNQ1 $23,370.00').underlying==='QQQ', '2d MNQ too');
ok(mode('SPY $771.50').underlying==='SPY' && mode('SPY $771.50').fam===null, '2e SPY is itself, with no conversion at all');
ok(mode('QQQ $565.10').underlying==='QQQ' && mode('QQQ $565.10').fam===null, '2f QQQ likewise');
var gc=mode('GC1 $2465.10');
ok(gc.ok===false && /No options tape for GC — levels unavailable/.test(gc.msg),
   '2g an unmapped instrument is NAMED and refused — never given invented levels', gc.msg);
ok(gc.r===1, '2h ...and no ratio is fabricated for it', gc.r);
ok(/No options tape/.test(futUnavailableHtml()), '2i the panel renders exactly that one line');
ok(/if\(!futMode\(\)\.ok\)/.test(ex('render')) && /futUnavailableHtml\(\)/.test(ex('render')),
   '2j ...and render() stops there instead of drawing a map it does not have');

// the ⚙ override
global.FUTMODE=mode('SPY $771.50','',null);
ok(mode('SPY $771.50', null, 'ES').chart==='ES', '2k ⚙ force ES overrides the detected chart');
ok(mode('ES1 $7768.75', null, 'SPY').chart==='SPY', '2l ⚙ force SPY overrides a futures chart');
ok(mode('ES1 $7768.75', null, 'NQ').underlying==='QQQ', '2m ⚙ force NQ switches the underlying too');
ok(/class="gpts-fut" data-fut/.test(src) && /futBtn\('auto'/.test(src) && /futBtn\('ES'/.test(src) &&
   /futBtn\('NQ'/.test(src) && /futBtn\('SPY'/.test(src),
   '2n the four-way override (auto | SPY | ES | NQ) is in the ⚙ drawer');
ok(/gpts_futcfg_v1/.test(src), '2o ...persisted under a NEW key, renaming nothing');

// the header ticker is the second source
var hdr=mode('Atlas', { ES:{px:7768.75, chg:0.4} });
ok(hdr.chart==='ES' && hdr.futPx===7768.75, '2p when the title says nothing, the header ticker does', hdr.chart+' '+hdr.futPx);

// ================= 3. THE RATIO: live, smoothed, and honest when it is not ========
var st={ema:null,last:null,t:0};
var r1=futRatioStep(st, 7768.75, 771.5, 'ES');
ok(r1.live===true && r1.approx===false && Math.abs(r1.r-10.0697)<0.01, '3a first live reading is the raw ratio', r1.r.toFixed(4));
var r2=futRatioStep(st, 7800, 771.5, 'ES');
ok(r2.r>r1.r && r2.r<7800/771.5, '3b ...and later ones are EMA-SMOOTHED, not jumped to', r2.r.toFixed(4));
var r3=futRatioStep(st, null, 771.5, 'ES');
ok(r3.live===false && r3.approx===true && r3.src==='last-good' && r3.r===r2.r,
   '3c an unreadable futures price falls back to the LAST GOOD ratio, marked approximate', r3.src);
var r4=futRatioStep({ema:null,last:null,t:0}, null, null, 'ES');
ok(r4.src==='const' && r4.r===ES_RATIO && r4.approx===true,
   '3d with no history at all it falls back to the CONSTANT, still marked approximate', r4.r);
ok(futRatioStep({ema:null,last:null,t:0}, null, null, 'NQ').r===41.36, '3e NQ has its own last-known constant');

// ================= 4. EVERY DISPLAYED LEVEL CONVERTS ==============================
global.FUTMODE={ chart:'ES', fam:'ES', underlying:'SPY', r:10.0676, live:true, approx:false, ok:true, ratioSrc:'live' };
ok(dispIsFut()===true && dispR()===10.0676, '4a futures display mode is active');
ok(fmtLvl(772)==='7772.25', '4b a STRIKE is shown in ES points, on the ES 0.25 tick (v11.0.1: 7772.19 → 7772.25)', fmtLvl(772));
ok(fmtLvl(772).indexOf('772')!==0, '4c ...and the SPY strike is NOT shown alongside it (user decision)', fmtLvl(772));
ok(fmtSpan(0.5)==='5', '4d a zone WIDTH scales by r and rounds to the tick (0.50 SPY -> 5 ES points)', fmtSpan(0.5));
ok(fmtLvl(null)==='–', '4e a missing level is a dash, never a converted zero');
global.FUTMODE.approx=true; global.FUTMODE.live=false;
ok(fmtLvl(772).charAt(0)==='≈', '4f once the ratio is assumed, EVERY converted level carries the ≈', fmtLvl(772));
ok(fmtSpan(0.5).charAt(0)==='≈', '4g ...distances included');
ok(/\(≈ /.test(futRatioText()) && /ES\/SPY/.test(futRatioText()), '4h the footer says so too', futRatioText());
global.FUTMODE.approx=false; global.FUTMODE.live=true;
ok(futRatioText()==='ES/SPY 10.068 (live)', '4i and reads "(live)" when it is measured', futRatioText());
// the cash panel is byte-identical to before
global.FUTMODE={ chart:'SPY', fam:null, underlying:'SPY', r:1, live:true, approx:false, ok:true };
ok(dispIsFut()===false && fmtLvl(772.25)===fmtNum(772.25) && fmtSpan(0.5)===fmtNum(0.5),
   '4j in SPY mode fmtLvl IS fmtNum — nothing about the cash view changes', fmtLvl(772.25));
// the renderers use it
var DZ=ex('deflZonesBlock');
ok(/fmtLvl\(L\.k\)/.test(DZ) && /fmtLvl\(fr\.tgt\)/.test(DZ) && /fmtLvl\(fr\.inval\)/.test(DZ),
   '4k the zone rows, the target and the invalidation all go through fmtLvl');
ok(/fmtSpan\(Math\.abs\(px-L\.k\)\)/.test(DZ), '4l ...and so does the distance');
ok(/fmtLvl\(leg\.magnet\.k\)/.test(ex('legVoice')) && /fmtLvl\(leg\.pbDetected\.k\)/.test(ex('legVoice')),
   '4m the READ speaks the magnet and the pullback node in the displayed instrument too');
ok(/fmtLvl\(leg\.pbDetected\.k\)/.test(ex('legDecisionLine')), '4n ...and the decision line');
ok(/fmtLvl\(kingK\)/.test(ex('kingHeaderBlock')) && /fmtLvl\(gkK\)/.test(ex('kingHeaderBlock')),
   '4o the King strike and the gatekeeper strike convert');
ok(/fmtLvl\(node\.k\)/.test(ex('kingHeaderBlock')), '4p ...and the SUP / RES magnets');
ok(/fmtLvl\(d\.gvwap\)/.test(ex('driftLineHtml')) && /fmtLvl\(d\.vvwap\)/.test(ex('driftLineHtml')),
   '4q the GEX / VEX drift centres convert too');
ok(/'R:R '\+rr\.toFixed\(1\)/.test(ex('rrText')), '4r R:R is a RATIO — it is unitless and must NOT be scaled');

// ================= 5. THE UNDERLYING IS RECONSTRUCTED FROM THE FUTURES CHART ======
var futBars=[{time:1,open:7768,high:7772,low:7760,close:7770},{time:2,open:7770,high:7780,low:7768,close:7776}];
var conv=futCandlesToUnderlying(futBars, 10.0676);
ok(conv.length===2 && Math.abs(conv[0].close-771.79)<0.01, '5a futures candles convert back by 1/r', conv[0].close.toFixed(2));
ok(conv[0].high>conv[0].close && conv[0].low<conv[0].close, '5b ...OHLC stays coherent');
ok(futCandlesToUnderlying(futBars, 0)===null && futCandlesToUnderlying(null, 10)===null, '5c a bad ratio converts nothing');
global.FUTMODE={ chart:'ES', fam:'ES', underlying:'SPY', r:10.0676, live:true, approx:false, ok:true, futPx:7768.75 };
FIBER={ 'ES1':futBars };
var raw=futRawCandles('SPY');
ok(raw && raw.length===2 && raw[0].close<800, '5d refreshSym gets SPY-scaled candles even though the chart is ES', raw&&raw[0].close.toFixed(2));
ok(Math.abs(futUnderlyingPx()-771.7)<0.5, '5e ...and the underlying PRICE is futPx / r', futUnderlyingPx().toFixed(2));
FIBER={ 'SPY':futBars };
ok(futRawCandles('SPY')===futBars, '5f when the cash chart IS on the page, its own candles win untouched');
ok(/var raw=futRawCandles\(sym\)/.test(ex('refreshSym')), '5g refreshSym reads through that path');

// ================= 6. RECORDING STAYS IN THE UNDERLYING'S STRIKES =================
// This is the whole safety property: the conversion is a DISPLAY concern. If any record
// path converted, four days of learning would be in two different unit systems.
['featRecordAll','featEnqueue','fcHistSample','nodeHistSample','tradeFrame','legStep'].forEach(function(fn){
  ok(!/fmtLvl|dispVal\(/.test(ex(fn)), '6·'+fn+' does not convert — it records in '+'underlying strikes');
});
ok(/recording stays in|Recording stays in/i.test(src), '6z the invariant is written down in the source');

// ================= 7. THE ACTIVE UNDERLYING DRIVES THE PANEL ======================
global.FUTMODE={ chart:'NQ', fam:'NQ', underlying:'QQQ', r:41.36, live:true, approx:false, ok:true };
ok(activeSym()==='QQQ', '7a an NQ chart makes QQQ the active underlying', activeSym());
global.FUTMODE={ chart:'ES', fam:'ES', underlying:'SPY', r:10.0676, live:true, approx:false, ok:true };
ok(activeSym()==='SPY', '7b an ES chart keeps SPY', activeSym());

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
