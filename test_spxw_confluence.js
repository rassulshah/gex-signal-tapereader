// (v10.55 PART G) SPXW CONFLUENCE, SCORED — but only when it is actually structure.
//
// Before v10.55 `S` was read off the trinity HEADER (a price and a % change), which is a
// tick, not a wall map, so it was display-only and the node grade ignored it. The sidebar
// also renders an SPXW strike LADDER; parsed into the same {king, pct[strike]} shape SPY
// and QQQ have, that IS structure — and then S earns its +1 exactly like Q. With no
// ladder the honest `S–` (or an unscored ✓) stays.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

// ---------------- a SYNTHETIC SPXW ladder, in the sidebar's own text shape ---------
var LADDER=[
  'SPXW  $6412.50  +0.42%',
  '6440   32%',
  '6435   58%',
  '6430  100%',
  '6425   61%',
  '6420   44%',
  '6405  -37%',
  '6400  -55%'
].join('\n');

global.PAL={ blue:'#4a90d9', sub:'#8b98a9' };
var DOMTEXT=null, HEADERS={ SPXW:{ px:6412.5, chg:0.42, kd:null, ks:null } };
global.document={ querySelectorAll:function(){ return DOMTEXT==null?[]:[{ textContent:DOMTEXT }]; } };
global.readTrinityHeaders=function(){ return HEADERS; };

eval(['parseSpxwLadder','spxwWallMap','spxwScored','spxwAgrees','zoneConfHtml'].map(ex).join('\n'));
global.SPXW_MAP=null; global.SPXW_MAP_T=0;
function resetMap(){ global.SPXW_MAP=null; global.SPXW_MAP_T=0; }

// ================= 1. THE PURE PARSER ============================================
var W=parseSpxwLadder(LADDER);
ok(W.ok===true, '1a a synthetic ladder parses into a wall map', W.count+' strikes');
ok(W.king===6430, '1b the King is the largest ABSOLUTE exposure, sign-agnostic', W.king);
ok(W.price===6412.5, '1c ...and the SPXW price comes off the same block', W.price);
ok(W.pct['6400.00']===-55 && W.pct['6435.00']===58, '1d signed %-of-King per strike, like SPY/QQQ', W.pct['6400.00']);
ok(W.count===7, '1e every ladder row is kept (and the header row is NOT mistaken for one)', W.count);
ok(parseSpxwLadder('SPXW $6412.50 +0.42%').ok===false, '1f a header ALONE is not a ladder — no map is claimed');
ok(parseSpxwLadder('').ok===false && parseSpxwLadder(null).ok===false, '1g nothing in, nothing claimed');
ok(parseSpxwLadder('6430 140%\n6420 44%\n6410 30%\n6400 20%').pct['6430.00']===undefined,
   '1h an impossible >100% row is dropped rather than crowned King');

// ================= 2. WITH A LADDER, S IS SCORED ================================
resetMap(); DOMTEXT=LADDER;
ok(spxwScored()===true, '2a a parsed ladder makes S scoreable');
// King 6430 sits ABOVE price 6412.5 -> the structural lean is UP
resetMap(); ok(spxwAgrees(1)===true,  '2b King above price agrees with an UP read', spxwAgrees(1));
resetMap(); ok(spxwAgrees(-1)===false,'2c ...and disagrees with a DOWN read', spxwAgrees(-1));
resetMap(); ok(spxwAgrees(0)===null,  '2d with no direction to check there is nothing to agree with');

// ================= 3. WITHOUT A LADDER, S IS DISPLAY-ONLY =======================
resetMap(); DOMTEXT=null;
ok(spxwScored()===false, '3a no ladder -> S is not scored');
ok(spxwAgrees(1)===true, '3b ...but the header momentum still answers (+0.42% agrees with UP)', spxwAgrees(1));
HEADERS=null; resetMap();
ok(spxwAgrees(1)===null, '3c ...and with no SPXW at all the answer is NULL, never a false ✓', String(spxwAgrees(1)));
ok(/S–|S'\+\(s===true/.test(ex('zoneConfHtml')), '3d the row renders the honest S– in that case');
ok(zoneConfHtml({s:null,q:true,v:false}).indexOf('S–')>=0, '3e ...literally', zoneConfHtml({s:null,q:true,v:false}).replace(/<[^>]+>/g,''));
ok(zoneConfHtml({s:true,q:true,v:true,sScored:true}).indexOf('S✓ ')>=0,
   '3f a SCORED agreement renders a plain S✓', zoneConfHtml({s:true,q:true,v:true,sScored:true}).replace(/<[^>]+>/g,''));
ok(zoneConfHtml({s:true,q:true,v:true,sScored:false}).indexOf('S✓·')>=0,
   '3g an UNSCORED agreement is marked so the two can never be confused',
   zoneConfHtml({s:true,q:true,v:true,sScored:false}).replace(/<[^>]+>/g,''));

// ================= 4. THE NODE GRADE ONLY MOVES ON STRUCTURE ====================
var NG=ex('nodeGrade');
ok(/if\(s===true && sScored\) score\+=1;/.test(NG), '4a S adds +1 ONLY when a ladder made it scoreable');
ok(/sScored:sScored/.test(NG), '4b ...and the record says which kind of S it was');
ok(/if\(q\) score\+=1;/.test(NG) && /if\(v\) score\+=1;/.test(NG), '4c Q and V are unchanged');
// with no ladder present the grade arithmetic is byte-identical to v10.54
global.spxwScored=function(){ return false; };
ok(/typeof spxwScored==='function'/.test(NG),
   '4d the lookup is guarded, so a page with no sidebar cannot break the grade');

// ================= 5. IT IS PARSED FROM THE SIDEBAR, CACHED, AND CHEAP ==========
ok(/\[class\*="trinity"\]/.test(ex('spxwWallMap')), '5a the ladder is read from the trinity sidebar');
ok(/SPXW_MAP_T\)<15000/.test(ex('spxwWallMap')), '5b ...and cached, not re-parsed on every node');
resetMap(); DOMTEXT=LADDER; spxwWallMap();
ok(global.SPXW_MAP && global.SPXW_MAP.king===6430, '5c the cache holds the parsed map', global.SPXW_MAP.king);

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
