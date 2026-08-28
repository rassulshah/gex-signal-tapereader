// v10.47/v10.50 tests — Node Map sentence (retained fn) + header block + sync banner.
// (v10.50) readBlock44's legacy bare-bones sentence was SUPERSEDED by the 3-beat voice;
// its assertions moved to test_read_voice.js. nodeMapSentence is retained (not rendered
// live) so its templates are still pinned here.
var fs=require('fs');
var src=fs.readFileSync('v10.js','utf8');
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0){ console.error('MISSING '+name); process.exit(1); }
  var depth=0,started=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){depth++;started=true;} else if(c==='}'){depth--; if(started&&depth===0){j++;break;}}} return src.slice(i,j); }
var f=0,p=0; function ok(c,n){ if(c){p++;} else {f++; console.log('  FAIL: '+n);} }

var PAL={bg:'#0b0e14',card:'#12161f',line:'#1e2530',longAccent:'#2ec27e',shortAccent:'#f0616d',ink:'#e6edf3',sub:'#8b98a9',amber:'#f2b45a',gold:'#e3c341',blue:'#5aa9ff'};
var DEFLECT_CONFIRM=2, EP_DEFL_HANDOFF=3, BO_HL_LOOKBACK=14;
var SYNC_GRACE=2;                 // (v10.56 PART E) consecutive failed checks before the banner
function fmtNum(x){ return (Math.round(x*100)/100).toString(); }
function ctNow(){ return new Date(2026,7,17,11,5); }
function closedCandles(){ return []; }
function accumCanon(){ return {m15:{pct:null,label:'Steady'},session:{pct:null,label:'Steady'}}; }
var STATE={SPY:{price:774.30}};
var MODEL=null; function nodeMapModel(){ return MODEL; }
function L(o){ return Object.assign({state:'Steady',chg:0,taps:0,pos:true,ep:{},dist:0,side:'above'},o); }
function mk(px, levels, range){ levels.forEach(function(x){ x.dist=Math.abs(x.k-px); x.side=x.k>px?'above':'below'; });
  var flr=levels.filter(function(x){return x.isFlr;})[0]||null, ceil=levels.filter(function(x){return x.isCeil;})[0]||null;
  return {ok:true,px:px,levels:levels.slice().sort(function(a,b){return b.k-a.k;}),kingK:(levels.filter(function(x){return x.isKing;})[0]||{}).k,flr:flr,ceil:ceil,range:(flr&&ceil)?{lo:flr.k,hi:ceil.k,inside:px>=flr.k&&px<=ceil.k}:null,regime:{label:'Trend'}}; }

eval(['_nmRole','_nmAcc','_nmIsAcc','_nmIsDec','_nmB','nodeMapSentence','nodeMapSentenceHtml','syncBannerShow','syncBannerHtml'].map(grab).join('\n'));
function strip(h){ return h.replace(/<[^>]+>/g,'').replace(/\s+/g,' ').trim(); }

// ---- Scenario A: continuation through the gate toward the King ----
MODEL=mk(774.30,[
  L({k:777.75,pct:22,isStrongMag:true,isNext:true,state:'Building',chg:6}),
  L({k:776.50,pct:33,isCeil:true,state:'Steady',chg:-3,taps:2}),
  L({k:775.38,pct:100,isKing:true,state:'Building',chg:12,taps:0,ep:{state:'Pull',tw:71}}),
  L({k:774.50,pct:28,isGatekeeper:true,state:'Fading',chg:-8,taps:2,ep:{state:'BOw'}}),
  L({k:773.25,pct:41,isFlr:true,state:'Building',chg:19,taps:1}),
  L({k:771.50,pct:18,isStrongMag:true,isNext:true,pos:false})
]);
var s1=nodeMapSentence(MODEL,'SPY',function(){return '';});
console.log('NM A:', s1.verdict, strip(s1.text));
ok(s1.verdict==='CONT','A NM CONT');
ok(/^thru Gate 774\.5 → King 775\.38: Dec ▼8%, King 775\.38 Acm ▲12% pulling\. Sup 773\.25 Acm ▲19%\.$/.test(strip(s1.text)),'A NM bare-bones template');
ok(!/mass/i.test(strip(s1.text)),'A never says mass');

// ---- Scenario D: REVERSAL at an accumulating ceiling on its 2nd arrival ----
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
MODEL.levels.forEach(function(x){ if(x.isCeil) x.taps=2; });
var s3=nodeMapSentence(MODEL,'SPY',function(){return '';}); console.log('NM D3:', s3.verdict, strip(s3.text));
ok(s3.verdict==='CONT' && /held 2× — 3rd tap usually fails/.test(s3.text),'D 3rd-tap flips to CONT with warning');
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
// (v10.56 PART E) the banner needs SYNC_GRACE consecutive failed checks before it is
// shown at all — a single failed reconciliation is silent (and still logged).
var bn=syncBannerHtml({reason:'no-consensus',streak:2,votes:{tag:775.38,feed:null,tapemax:775.38}});
ok(/Out of sync/.test(bn) && /three King sources disagree/.test(bn) && !/Diagnose[^"]*<\/div>\s*<div/.test(bn),'sync banner is one line with hover detail');
ok(syncBannerHtml({reason:'no-consensus',streak:1,votes:{tag:775.38,feed:null,tapemax:775.38}})==='',
   '...and one failed check alone renders nothing');

// ---- render structure ----
ok(/html\+=kingHeaderBlock\(\)/.test(src) && src.indexOf('html+=kingHeaderBlock()')<src.indexOf("html+=readBlock44(__asym)"),'render(): header block before READ');
// (v10.50) step icons ①②③ RETIRED from the header pills.
var kh=grab('kingHeaderBlock');
ok(!/stepMini\(1\)/.test(kh) && !/stepMini\(2\)/.test(kh) && !/stepMini\(3\)/.test(kh) && !/5-STEP/.test(kh) && /SUP/.test(kh),'kingHeaderBlock: no ①②③ step icons, SUP/RES pills remain');
ok(!/uD83D\\uDEAA':''\)/.test(grab('nodeRolePill')) && /gateSvgSm/.test(grab('nodeRolePill')),'gate icon in row pill is the castle-gate svg, not a door');
ok(/FLRCEIL_EDGE_PCT/.test(grab('nodeMapModel')) && /pickEdge/.test(grab('nodeMapModel')),'Flr/Ceil = largest bounding node (pickEdge)');
ok(/isNext=true/.test(grab('nodeMapModel')) && /Mag \\u00b7 next/.test(grab('nodeRoleBadge')),'next-target class exists');
ok(/Defl \\u00b7 '\+roleAb/.test(grab('deflectionBlock')),'defl card abbreviated name');
// (v10.50) three version spots: @version header, part1 console.log, footer version marker.
ok(/@version\s+14.80/.test(src) && /v'\+GPTS_VERSION\+' part1 loaded/.test(src) && />v'\+GPTS_VERSION\+'<\/span>/.test(src),'version 10.56 in all three spots');
console.log('test_read_v1047: '+p+' passed, '+f+' failed'); process.exit(f?1:0);
