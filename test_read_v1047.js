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
ok(/READ ▸ BULLISH\./.test(r),'A verdict BULLISH');
ok(/going up toward the King at 775\.38, about a strike away/.test(r),'A destination King + distance');
ok(/Gate at 774\.5 is in between and it has already held twice/.test(r),'A gate between + held twice');
ok(/Support at 773\.25 is building while Resistance at 776\.5 is steady/.test(r),'A S/R states');
ok(/King is getting heavier — dealers are pulling price up/.test(r),'A King heavier');
ok(/worked 60% of the time at this distance, 74% in this hour/.test(r),'A odds sentence');
ok(/Watch the floor at 773\.25: breaking it changes the read/.test(r),'A watch floor');
ok(!/mass/i.test(r),'A never says mass');
var s1=nodeMapSentence(MODEL,'SPY',function(){return '';});
console.log('NM A:', s1.verdict, strip(s1.text));
ok(s1.verdict==='CONTINUATION','A NM CONTINUATION');
ok(/through 774\.5 toward the King at 775\.38 because the gate is decreasing \(▼8%\), while the King above is accumulating \(▲12%\) and pulling harder\. Support at 773\.25 is accumulating too so the floor under the move is firm\./.test(strip(s1.text)),'A NM locked template');

// ---- Scenario B: chop -> SIDEWAYS, odds dropped ----
REGIME='chop'; STATE.SPY.price=774.90; MODEL=mk(774.90, MODEL.levels.map(function(x){ x=Object.assign({},x); x.ep={}; if(x.isKing){x.state='Steady';x.chg=2;} if(x.isFlr){x.state='Steady';x.chg=1;} return x; }));
r=strip(readBlock44('SPY')); console.log('READ B:', r.slice(0,300));
ok(/READ ▸ SIDEWAYS\./.test(r),'B chop -> SIDEWAYS');
ok(/inside 773\.25–776\.5, near the midpoint/.test(r),'B range + midpoint wording');
ok(/King at 775\.38 is steady, neither side is building/.test(r),'B King steady, neither building');
ok(!/worked \d+%/.test(r),'B odds sentence dropped in chop');
ok(/Watch 776\.5 and 773\.25: a break with follow-through sets direction/.test(r),'B watch both edges');
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
ok(/READ ▸ TBD\./.test(r),'C verdict TBD');
ok(/King at 775\.38 is above price and pulling up, but Resistance at 776\.5 and 777\.75 sit right above it and are building — they block the way/.test(r),'C resistance named + blocks the way');
ok(/Support at 773\.25 is fading, not helping/.test(r),'C support not helping');
ok(/Watch the gate at 774\.5: a hold means the King wins, a break means the resistance wins/.test(r),'C watch gate');
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
ok(s2.verdict==='REVERSAL','D REVERSAL');
ok(/likely at 776\.5 — the ceiling is accumulating \(▲14%\) and has held once, and 777\.75 behind it is accumulating as well, so resistance is stacking\. Nothing below price is decreasing, so a deflection here would have support to fall back on\./.test(strip(s2.text)),'D locked reversal template');
// 3rd tap flips
MODEL.levels.forEach(function(x){ if(x.isCeil) x.taps=2; });
var s3=nodeMapSentence(MODEL,'SPY',function(){return '';}); console.log('NM D3:', s3.verdict, strip(s3.text));
ok(s3.verdict==='CONTINUATION' && /held twice — a third tap usually fails \(~33%\)/.test(s3.text),'D 3rd-tap flips to continuation with warning');
// polarity: purple ceiling
MODEL.levels.forEach(function(x){ if(x.isCeil){ x.taps=1; x.pos=false; } });
var s4=nodeMapSentence(MODEL,'SPY',function(){return '';});
ok(/purple node/.test(s4.text),'D polarity mentioned for −γ node');

// ---- Scenario E: no node in play ----
STATE.SPY.price=775.00;
MODEL=mk(775.00,[ L({k:776.50,pct:45,isCeil:true}), L({k:775.38,pct:100,isKing:true,ep:{state:'Pull'}}), L({k:773.25,pct:41,isFlr:true}) ]);
MODEL.levels.forEach(function(x){ if(x.isKing) x.ep={}; });
var s5=nodeMapSentence(MODEL,'SPY',function(){return '';}); console.log('NM E:', s5.verdict, strip(s5.text));
ok(s5.verdict==='NO NODE IN PLAY' && /between 773\.25 and 775\.38/.test(strip(s5.text)),'E no node in play names neighbours');

// ---- sync banner ----
var bn=syncBannerHtml({reason:'no-consensus',votes:{tag:775.38,feed:null,tapemax:775.38}});
ok(/OUT OF SYNC WITH TAPE/.test(bn) && /three King sources disagree/.test(bn) && !/Diagnose[^"]*<\/div>\s*<div/.test(bn),'sync banner is one line with hover detail');
ok(/html\+=kingHeaderBlock\(\)/.test(src) && src.indexOf('html+=kingHeaderBlock()')<src.indexOf("html+=readBlock44('SPY')"),'render(): header block before READ');
ok(/function kingHeaderBlock\(\)/.test(src) && /stepIcon\(1\)\+stepIcon\(2\)\+stepIcon\(3\)/.test(grab('kingHeaderBlock')),'kingHeaderBlock has ★SUP/King/★RES + ①②③');
ok(!/uD83D\\uDEAA':''\)/.test(grab('nodeRolePill')) && /gateSvgSm/.test(grab('nodeRolePill')),'gate icon in row pill is the castle-gate svg, not a door');
ok(/FLRCEIL_EDGE_PCT/.test(grab('nodeMapModel')) && /pickEdge/.test(grab('nodeMapModel')),'Flr/Ceil = largest bounding node (pickEdge)');
ok(/isNext=true/.test(grab('nodeMapModel')) && /Mag \\u00b7 next/.test(grab('nodeRoleBadge')),'next-target class exists');
ok(/Defl \\u00b7 '\+roleAb/.test(grab('deflectionBlock')),'defl card abbreviated name');
ok(/@version\s+10\.47/.test(src) && /feed v10\.47/.test(src),'version 10.47');
console.log('test_read_v1047: '+p+' passed, '+f+' failed'); process.exit(f?1:0);
