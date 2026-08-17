// v10.47 tests — READ paragraph + Node Map sentence + header block + sync banner (Phase A).
var fs=require('fs');
var src=fs.readFileSync('v10.js','utf8');
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0){ console.error('MISSING '+name); process.exit(1); }
  var depth=0,started=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){depth++;started=true;} else if(c==='}'){depth--; if(started&&depth===0){j++;break;}}} return src.slice(i,j); }
var f=0,p=0; function ok(c,n){ if(c){p++;} else {f++; console.log('  FAIL: '+n);} }

var PAL={bg:'#0b0e14',card:'#12161f',line:'#1e2530',longAccent:'#2ec27e',shortAccent:'#f0616d',ink:'#e6edf3',sub:'#8b98a9',amber:'#f2b45a',gold:'#e3c341',blue:'#5aa9ff'};
var DEFLECT_CONFIRM=2, EP_DEFL_HANDOFF=3, BO_HL_LOOKBACK=14;
function fmtNum(x){ return (Math.round(x*100)/100).toString(); }
function stepIcon(n){ return '<i>'+n+'</i>'; }
function ctNow(){ return new Date(2026,7,17,11,5); }
function closedCandles(){ return []; }
var REGIME='trend'; function regimeTag(){ return {tag:REGIME, er:0.68}; }
var SRB={dom:'support',forming:false}; function srBattle(){ return SRB; }
var STUDY={src:'local repo',bars:402,kingPull:{all:{h:120,n:201},byDist:{'1':{h:36,n:60},'2':{h:41,n:68}},byHour:{'11':{h:23,n:31}}}}; function studyLoad(){ return STUDY; }
function studyPct(o){ return (o&&o.n)?Math.round(100*o.h/o.n):null; }
function studyTag(o){ return (o&&o.n>=20)?'📊':'⚖'; }
var STATE={SPY:{price:774.30}};
var MODEL=null; function nodeMapModel(){ return MODEL; }
function L(o){ return Object.assign({state:'Steady',chg:0,taps:0,pos:true,ep:{},dist:0,side:'above'},o); }
function mk(px, levels, range){ levels.forEach(function(x){ x.dist=Math.abs(x.k-px); x.side=x.k>px?'above':'below'; });
  var flr=levels.filter(function(x){return x.isFlr;})[0]||null, ceil=levels.filter(function(x){return x.isCeil;})[0]||null;
  return {ok:true,px:px,levels:levels.slice().sort(function(a,b){return b.k-a.k;}),kingK:(levels.filter(function(x){return x.isKing;})[0]||{}).k,flr:flr,ceil:ceil,range:(flr&&ceil)?{lo:flr.k,hi:ceil.k,inside:px>=flr.k&&px<=ceil.k}:null,regime:{label:'Trend'}}; }

eval(['_nmRole','_nmAcc','_nmIsAcc','_nmIsDec','_nmB','nodeMapSentence','nodeMapSentenceHtml','readBlock44','syncBannerHtml'].map(grab).join('\n'));
function strip(h){ return h.replace(/<[^>]+>/g,'').replace(/\s+/g,' ').trim(); }

// ---- Scenario A: bullish, gate BOw + decreasing, King accumulating ----
MODEL=mk(774.30,[
  L({k:777.75,pct:22,isStrongMag:true,isNext:true,state:'Building',chg:6}),
  L({k:776.50,pct:33,isCeil:true,state:'Steady',chg:-3,taps:2}),
  L({k:775.38,pct:100,isKing:true,state:'Building',chg:12,taps:0,ep:{state:'Pull',tw:71}}),
  L({k:774.50,pct:28,isGatekeeper:true,state:'Fading',chg:-8,taps:2,ep:{state:'BOw'}}),
  L({k:773.25,pct:41,isFlr:true,state:'Building',chg:19,taps:1}),
  L({k:771.50,pct:18,isStrongMag:true,isNext:true,pos:false})
]);
var r=strip(readBlock44('SPY'));
console.log('READ A:', r.slice(0,420));
ok(/^BULLISH\./.test(r),'A verdict BULLISH (no READ label)');
ok(/Up to King 775\.38\./.test(r),'A destination King');
ok(/Gate 774\.5 held 2×/.test(r),'A gate + taps');
ok(/Sup 773\.25 building, Res 776\.5 steady/.test(r),'A S/R states');
ok(/King heavier — pulling up/.test(r),'A King heavier');
ok(/60% at this distance, 74% this hour/.test(r),'A odds sentence');
ok(/Watch 773\.25\./.test(r),'A watch floor');
ok(!/mass/i.test(r),'A never says mass');
var s1=nodeMapSentence(MODEL,'SPY',function(){return '';});
console.log('NM A:', s1.verdict, strip(s1.text));
ok(s1.verdict==='CONT','A NM CONT');
ok(/^thru Gate 774\.5 → King 775\.38: Dec ▼8%, King 775\.38 Acm ▲12% pulling\. Sup 773\.25 Acm ▲19%\.$/.test(strip(s1.text)),'A NM bare-bones template');

// ---- Scenario B: chop -> SIDEWAYS, odds dropped ----
REGIME='chop'; STATE.SPY.price=774.90; MODEL=mk(774.90, MODEL.levels.map(function(x){ x=Object.assign({},x); x.ep={}; if(x.isKing){x.state='Steady';x.chg=2;} if(x.isFlr){x.state='Steady';x.chg=1;} return x; }));
r=strip(readBlock44('SPY')); console.log('READ B:', r.slice(0,300));
ok(/^SIDEWAYS\./.test(r),'B chop -> SIDEWAYS');
ok(/Inside 773\.25–776\.5, mid-range/.test(r),'B range + midpoint wording');
ok(/King 775\.38 steady\./.test(r),'B King steady');
ok(!/% at this distance/.test(r),'B odds sentence dropped in chop');
ok(/Watch 776\.5 \/ 773\.25\./.test(r),'B watch both edges');
REGIME='trend';

// ---- Scenario C: TBD — King above but resistance stacking above it, floor fading ----
STATE.SPY.price=774.30;
MODEL=mk(774.30,[
  L({k:777.75,pct:30,isStrongMag:true,state:'Building',chg:11}),
  L({k:776.50,pct:45,isCeil:true,state:'Building',chg:14,taps:2}),
  L({k:775.38,pct:100,isKing:true,state:'Steady',chg:3}),
  L({k:774.50,pct:28,isGatekeeper:true,state:'Steady',chg:0,taps:1,ep:{state:'Pull'}}),
  L({k:773.25,pct:41,isFlr:true,state:'Fading',chg:-12})
]);
SRB={dom:'resistance',forming:false};
r=strip(readBlock44('SPY')); console.log('READ C:', r.slice(0,420));
ok(/^TBD\./.test(r),'C verdict TBD');
ok(/King 775\.38 above, pulling up — but Res 776\.5, 777\.75 building against it/.test(r),'C resistance named');
ok(/Sup 773\.25 fading\./.test(r),'C support state');
ok(/Watch Gate 774\.5\./.test(r),'C watch gate');
SRB={dom:'support',forming:false};

// ---- Scenario D: REVERSAL at an accumulating ceiling on its 2nd arrival, next node building ----
STATE.SPY.price=776.10;
MODEL=mk(776.10,[
  L({k:777.75,pct:30,isStrongMag:true,isNext:true,state:'Building',chg:9}),
  L({k:776.50,pct:45,isCeil:true,state:'Building',chg:14,taps:1,ep:{state:'Pull',tw:70}}),
  L({k:775.38,pct:100,isKing:true,state:'Steady',chg:2}),
  L({k:773.25,pct:41,isFlr:true,state:'Steady',chg:1})
]);
var s2=nodeMapSentence(MODEL,'SPY',function(){return '';}); console.log('NM D:', s2.verdict, strip(s2.text));
ok(s2.verdict==='REV','D REV');
ok(/^at Ceil 776\.5: Acm ▲14%, held 1×, 777\.75 Acm ▲9% behind\. Sup 773\.25 steady\.$/.test(strip(s2.text)),'D bare-bones reversal template');
// 3rd tap flips
MODEL.levels.forEach(function(x){ if(x.isCeil) x.taps=2; });
var s3=nodeMapSentence(MODEL,'SPY',function(){return '';}); console.log('NM D3:', s3.verdict, strip(s3.text));
ok(s3.verdict==='CONT' && /held 2× — 3rd tap usually fails/.test(s3.text),'D 3rd-tap flips to CONT with warning');
// polarity: purple ceiling
MODEL.levels.forEach(function(x){ if(x.isCeil){ x.taps=1; x.pos=false; } });
var s4=nodeMapSentence(MODEL,'SPY',function(){return '';});
ok(/−γ/.test(s4.text),'D polarity mentioned for −γ node');

// ---- Scenario E: no node in play ----
STATE.SPY.price=775.00;
MODEL=mk(775.00,[ L({k:776.50,pct:45,isCeil:true}), L({k:775.38,pct:100,isKing:true,ep:{state:'Pull'}}), L({k:773.25,pct:41,isFlr:true}) ]);
MODEL.levels.forEach(function(x){ if(x.isKing) x.ep={}; });
var s5=nodeMapSentence(MODEL,'SPY',function(){return '';}); console.log('NM E:', s5.verdict, strip(s5.text));
ok(s5.verdict==='NO NODE IN PLAY' && nodeMapSentenceHtml(MODEL,'SPY',function(){return '';},'')==='','E nothing engaged -> no sentence rendered');

// ---- sync banner ----
var bn=syncBannerHtml({reason:'no-consensus',votes:{tag:775.38,feed:null,tapemax:775.38}});
ok(/Out of sync/.test(bn) && /three King sources disagree/.test(bn) && !/Diagnose[^"]*<\/div>\s*<div/.test(bn),'sync banner is one line with hover detail');
ok(/html\+=kingHeaderBlock\(\)/.test(src) && src.indexOf('html+=kingHeaderBlock()')<src.indexOf("html+=readBlock44('SPY')"),'render(): header block before READ');
ok(/function kingHeaderBlock\(\)/.test(src) && /stepMini\(1\)/.test(grab('kingHeaderBlock')) && /stepMini\(2\)/.test(grab('kingHeaderBlock')) && /stepMini\(3\)/.test(grab('kingHeaderBlock')) && !/5-STEP/.test(grab('kingHeaderBlock')),'kingHeaderBlock: ①②③ inline in the pills, no 5-STEP row');
ok(!/uD83D\\uDEAA':''\)/.test(grab('nodeRolePill')) && /gateSvgSm/.test(grab('nodeRolePill')),'gate icon in row pill is the castle-gate svg, not a door');
ok(/FLRCEIL_EDGE_PCT/.test(grab('nodeMapModel')) && /pickEdge/.test(grab('nodeMapModel')),'Flr/Ceil = largest bounding node (pickEdge)');
ok(/isNext=true/.test(grab('nodeMapModel')) && /Mag \\u00b7 next/.test(grab('nodeRoleBadge')),'next-target class exists');
ok(/Defl \\u00b7 '\+roleAb/.test(grab('deflectionBlock')),'defl card abbreviated name');
ok(/@version\s+10\.49/.test(src) && /feed v10\.49/.test(src),'version 10.49');
console.log('test_read_v1047: '+p+' passed, '+f+' failed'); process.exit(f?1:0);
