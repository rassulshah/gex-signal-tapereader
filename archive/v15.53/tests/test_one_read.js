// v11.0.1 — ONE READ. User (2026-08-18): combine the read and the structure text; Acm/Dec words;
// "rolling down from X to Y"; the ⚑ banner folded into the read on the bar a pullback node lands
// ("Resistance pullback node formed at 7716.25 rolled down from 7726.25."); PB tag plain; no SPXW tag.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};
function mul(a,b){return a/(1/b);} global.mul=mul;
global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',sub:'#8b98a9',ink:'#e6edf3',line:'#21262d',gold:'#f2cc60',card:'#0d1117'};
global.PB_MIN_PCT=20; global.PB_REACH=5; global.MAP_ACM=8; global.MAP_DEC=-8; global.MAP_DROP=25;
global.dispIsFut=()=>false; global.futMark=()=>''; global.dispR=()=>1; global.fmtFut=String; global.FUTMODE={fam:null};
eval(['fmtNum','fmtLvl','mapWordCap','mapSentence','mapLineText'].map(ex).join('\n'));

let FLOW=null; global.nodeFlow=()=>FLOW;
function flow(o){ return Object.assign({ok:true,px:768.3,nodes:[],transfers:[],widening:null,lean:'none',leanWhy:''},o); }
const T=(side,from,to)=>({side:side,from:from,to:to,dir:(to<from)?'dn':'up',fromState:'dec'});

// ---- the twelve structure sentences ----
FLOW=flow({transfers:[T('ceil',7735,7730)],lean:'dn'});
let m=mapSentence('SPY',null,true,{});
ok('1 ceiling rolling down', m.s==='Ceiling rolling down from 7735 Dec to 7730 Acm. Pullback node likely at 7730.', m.s);
ok('1h ...Acm green / Dec red words, capitalised, no arrows', /Acm/.test(m.html) && /Dec/.test(m.html) && /#2ec27e/.test(m.html) && /#f0616d/.test(m.html) && !/[→▲▼]/.test(m.html));
FLOW=flow({transfers:[T('ceil',7725,7730)]});
m=mapSentence('SPY',null,true,{});
ok('2 ceiling rolling up', m.s==='Ceiling rolling up from 7725 Dec to 7730 Acm. Room above to 7730.', m.s);
FLOW=flow({transfers:[T('flr',7710,7715)],lean:'up'});
m=mapSentence('SPY',null,true,{});
ok('3 floor rolling up', m.s==='Floor rolling up from 7710 Dec to 7715 Acm. Pullback node likely at 7715.', m.s);
FLOW=flow({transfers:[T('flr',7715,7710)]});
m=mapSentence('SPY',null,true,{});
ok('4 floor rolling down', m.s==='Floor rolling down from 7715 Dec to 7710 Acm. Room below to 7710.', m.s);
FLOW=flow({transfers:[T('ceil',7735,7730),T('flr',7715,7710)],lean:'dn'});
m=mapSentence('SPY',null,true,{});
ok('5 both rolling down', m.s==='Ceiling rolling down from 7735 Dec to 7730 Acm, floor rolling down from 7715 Dec to 7710 Acm — structure leaning down. Pullback node likely at 7730. Magnet 7710.', m.s);
FLOW=flow({transfers:[T('ceil',7725,7730),T('flr',7710,7715)],lean:'up'});
m=mapSentence('SPY',null,true,{});
ok('6 both rolling up', m.s==='Floor rolling up from 7710 Dec to 7715 Acm, ceiling rolling up from 7725 Dec to 7730 Acm — structure leaning up. Pullback node likely at 7715. Magnet 7730.', m.s);
FLOW=flow({transfers:[T('ceil',7735,7730),T('flr',7710,7715)]});
m=mapSentence('SPY',null,true,{});
ok('7 compression', m.s==='Ceiling rolling down from 7735 Dec to 7730 Acm, floor rolling up from 7710 Dec to 7715 Acm — range compressing to 7715–7730. Break pending, direction undecided.', m.s);
FLOW=flow({transfers:[T('ceil',7725,7730),T('flr',7715,7710)]});
m=mapSentence('SPY',null,true,{});
ok('8 expansion', m.s==='Ceiling rolling up from 7725 Dec to 7730 Acm, floor rolling down from 7715 Dec to 7710 Acm — range widening to 7710–7730. Rotation between them.', m.s);
FLOW=flow({transfers:[T('ceil',7725,7730),T('ceil',7735,7730)]});
m=mapSentence('SPY',null,true,{});
ok('9 converging ceilings', m.s==='Ceilings converging on 7730: 7725 rolling up, 7735 rolling down. 7730 becoming the resistance.', m.s);
FLOW=flow({widening:{dead:[7728],up:7730,dn:7715,lo:7715,hi:7730}});
m=mapSentence('SPY',null,true,{});
ok('10 middle emptying', m.s==='7728 Dec between 7730 and 7715, both Acm. Middle emptying — faster travel between them.', m.s);
FLOW=flow({nodes:[{k:7710,side:'below',state:'acm',pct:60}]});
m=mapSentence('SPY',{dir:'dn',magnet:{k:7715,isKing:true}},true,{});
ok('11 magnet moving', m.s==='7710 Acm below the King — magnet moving down to 7710.', m.s);
FLOW=flow({nodes:[{k:7730,side:'above',state:'hold',pct:40},{k:7715,side:'below',state:'hold',pct:70}]});
m=mapSentence('SPY',null,true,{});
ok('12 holding', m.s==='Structure holding: 7730 ceiling, 7715 floor, no transfer under way.', m.s);

// ---- combination rules ----
FLOW=flow({transfers:[T('ceil',7735,7730),T('flr',7715,7710)],lean:'dn'});
m=mapSentence('SPY',{dir:'dn',magnet:{k:7710}},true,{saidPB:true,saidMagnet:true});
ok('13 when the leg sentence already named the pullback node and the magnet, the structure sentence repeats neither', m.s==='Ceiling rolling down from 7735 Dec to 7730 Acm, floor rolling down from 7715 Dec to 7710 Acm — structure leaning down.', m.s);
m=mapSentence('SPY',null,false,{});
ok('14 no trend + lean → the caveat "SMA-50 has no trend: structure leads, trend unconfirmed."', /structure leads, trend unconfirmed\.$/.test(m.s), m.s);
m=mapSentence('SPY',{dir:'up',magnet:{k:7730}},true,{});
ok('15 lean against a confirmed trend → "Structure rolling against the trend — caution."', /against the trend — caution\.$/.test(m.s), m.s);

// ---- the read: one paragraph, no Map label, no banner; the ⚑ event folded in ----
const rb=ex('readBlock44');
ok('16 the read renders sentenceHtml (leg + structure in ONE paragraph) and no separate Map line / banner', /bodyHtml=\(v\.sentenceHtml\|\|v\.sentence\)/.test(rb) && !/mapLineHtml\(/.test(rb) && !/legBannerHtml\(/.test(rb));
ok('17 the bar a pullback node lands: the read takes the ⚑ style (red down / green up) and fires the pbNode alert', /var evt=!!v\.event;/.test(rb) && /fireAlert\('pbNode'/.test(rb) && /⚑/.test(rb));
ok('18 no "Map:" label anywhere on the face', !/>Map:<\/b>/.test(rb));
const lv=ex('legVoice');
ok('19 sentence 3 carries "rolled down from X" only on the landing bar (leg.event pbDetected)', /var landed=\(leg\.event==='pbDetected'\);/.test(lv) && /rolled '\+\(dn\?'down':'up'\)\+' from '\+fmtLvl\(rf\)/.test(lv));
const zt=ex('legZoneTag');
ok('20 the PB tag is plain "PB" — the roll step is in the hover', /var lab='PB';/.test(zt) && !/lab\+=' · '/.test(zt) && /Roll step:/.test(zt));
const dz=ex('deflZonesBlock');
ok('21 no SPXW tag on the rows (hover only)', !/mapSrcHtml\(L\)\+/.test(dz));
// futures tick
global.FUT_TICK={ES:0.25,NQ:0.25}; global.FUTMODE={fam:'ES'};
eval(['futTick','fmtFut'].map(ex).join('\n'));
ok('22 ES levels round to the 0.25 tick (7716.36 → 7716.25, 7726.41 → 7726.5)', fmtFut(7716.36)==='7716.25' && fmtFut(7726.41)==='7726.5' && fmtFut(7772.19)==='7772.25');
global.FUTMODE={fam:null}; eval(ex('fmtFut'));
ok('23 SPY/QQQ levels unchanged (no tick rounding)', fmtFut(768.36)==='768.36');
console.log('test_one_read: '+p+' passed, '+f+' failed');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
