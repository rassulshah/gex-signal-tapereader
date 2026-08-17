// (v10.49 E) DESCRIPTIVE TRADE FRAME — zone / inval / tgt / path. Map words only:
// where the level is, where the read stops being true, what is next, what lies between.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m+(g!==undefined?' -> '+g:''));} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+g:''));} };

global.mul=function(a,b){ return a/(1/b); };
global.fmtNum=function(x){ return (Math.round(x*100)/100).toString(); };
var MODEL={ ok:true, px:773.66, kingK:775, emphasis:'above',
  levels:[{k:777},{k:776},{k:775},{k:774},{k:773},{k:772},{k:771}],
  airpocket:{ ok:false, pockets:[] }, cluster:{ ok:false, regions:[] } };
global.nodeMapModel=function(){ return MODEL; };

eval(['tradeFrame','frameTextOf'].map(ex).join('\n'));
global.FRAME_HALF=0.25;
global.FRAME_FALLBACK=0.5;

// ================= 1. the zone is the strike ± 0.25 =================
var f=tradeFrame('SPY',{k:773},1);
ok(f.zone[0]===772.75 && f.zone[1]===773.25, '1a zone = 773 ± 0.25', f.zone.join('..'));
ok(f.k===773 && f.dir===1, '1b strike + direction carried');

// ================= 2. INVAL = the next node BEYOND, against the direction =================
ok(f.inval===772, '2a going up from 773, the read fails below 772', f.inval);
var fd=tradeFrame('SPY',{k:773},-1);
ok(fd.inval===774, '2b going down from 773, the read fails above 774', fd.inval);
// no node beyond -> half a strike past
MODEL.levels=[{k:773},{k:775}];
ok(tradeFrame('SPY',{k:773},1).inval===772.5, '2c no node below -> k−0.5 fallback', tradeFrame('SPY',{k:773},1).inval);
ok(tradeFrame('SPY',{k:775},-1).inval===775.5, '2d no node above -> k+0.5 fallback', tradeFrame('SPY',{k:775},-1).inval);
MODEL.levels=[{k:777},{k:776},{k:775},{k:774},{k:773},{k:772},{k:771}];

// ================= 3. TGT = the next node in the direction, CAPPED AT THE KING =====
ok(tradeFrame('SPY',{k:773},1).tgt===774,  '3a next node up from 773 is 774', tradeFrame('SPY',{k:773},1).tgt);
ok(tradeFrame('SPY',{k:773},-1).tgt===772, '3b next node down from 773 is 772');
// a sparse board: the next node up is PAST the King -> capped to the King
MODEL.levels=[{k:780},{k:775},{k:773},{k:771}]; MODEL.kingK=775;
ok(tradeFrame('SPY',{k:773},1).tgt===775, '3c the target never overshoots the King', tradeFrame('SPY',{k:773},1).tgt);
MODEL.levels=[{k:790},{k:773},{k:771}];
ok(tradeFrame('SPY',{k:773},1).tgt===775, '3d a far node is replaced by the King itself');
MODEL.levels=[{k:773},{k:771}];
ok(tradeFrame('SPY',{k:773},1).tgt===775, '3e no node ahead at all -> the King becomes the target');
MODEL.kingK=null;
ok(tradeFrame('SPY',{k:773},1).tgt===null, '3f no node and no King -> tgt null, never invented');
MODEL.kingK=775; MODEL.levels=[{k:777},{k:776},{k:775},{k:774},{k:773},{k:772},{k:771}];

// ================= 4. PATH from the air-pocket / cluster detectors =================
ok(tradeFrame('SPY',{k:773},1).path==='wall', '4a nodes all the way -> wall');
MODEL.airpocket={ ok:true, pockets:[{lo:773.5, hi:774.5, type:'Air Pocket'}] };
ok(tradeFrame('SPY',{k:773},1).path==='air', '4b an air pocket between zone and target -> air');
MODEL.airpocket={ ok:true, pockets:[{lo:760, hi:762, type:'Air Pocket'}] };
ok(tradeFrame('SPY',{k:773},1).path==='wall', '4c a pocket that does not overlap the span is ignored');
MODEL.cluster={ ok:true, regions:[{lo:773.5, hi:774.5, n:3}] };
ok(tradeFrame('SPY',{k:773},1).path==='cluster', '4d a cluster over the span -> cluster');
MODEL.airpocket={ ok:true, pockets:[{lo:773.5, hi:774.5}] };
ok(tradeFrame('SPY',{k:773},1).path==='air', '4e air wins over cluster (it is the faster pathway)');
MODEL.airpocket={ ok:false, pockets:[] }; MODEL.cluster={ ok:false, regions:[] };

// ================= 5. the rendered string is the locked vocabulary =================
var txt=frameTextOf(tradeFrame('SPY',{k:773},1));
ok(/^zone 773±\.25/.test(txt), '5a starts with the zone band', txt);
ok(/inval <772/.test(txt),     '5b inval reads with the "<" side going up', txt);
ok(/tgt 774 \(wall\)/.test(txt),'5c target carries its path', txt);
var txtDn=frameTextOf(tradeFrame('SPY',{k:773},-1));
ok(/inval >774/.test(txtDn),   '5d inval flips to ">" going down', txtDn);
ok(frameTextOf(null)==='' && frameTextOf({k:null})==='', '5e no frame -> empty string');
// VOCABULARY RED LINE
var banned=/\b(entry|stop|size|buy|sell|long|short)\b/i;
ok(!banned.test(txt) && !banned.test(txtDn), '5f the frame text never uses an execution word');
ok(/zone/.test(txt) && /inval/.test(txt) && /tgt/.test(txt), '5g exactly the locked words: zone / inval / tgt / path');

// ================= 6. defensive =================
ok(tradeFrame('SPY', null, 1).zone===null, '6a null level -> empty frame, no throw');
global.nodeMapModel=function(){ throw new Error('boom'); };
var s=tradeFrame('SPY',{k:773},1);
ok(s.zone && s.zone[0]===772.75, '6b a throwing model still yields the zone band', s.zone.join('..'));
global.nodeMapModel=function(){ return MODEL; };
var z=tradeFrame('SPY',{k:773},0);
ok(z.tgt!=null || z.inval!=null, '6c dir 0 falls back to the model emphasis rather than giving up');

// ================= 7. source guards =================
ok(/vocabulary is LOCKED to zone \/ inval \/ tgt \/ path/i.test(src) || /zone \/ inval \/ tgt \/ path/.test(src), '7a the locked vocabulary is documented');
ok(/var FRAME_HALF=0\.25/.test(src), '7b the zone half-width is a named constant');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
