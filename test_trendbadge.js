// Issue G: trend badge renders code+count+arrow+slope for each state; warming-up on na.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function mul(a,b){return a/(1/b);} global.mul=mul;
global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',amber:'#f2b45a',sub:'#8b98a9',card:'#12161f',line:'#1e2530'};
global.CFG={trendMA:{SPY:'50'}};
let TV; global.trendVerdict=function(){return TV;};
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(['trendWordOf','trendColorOf','trendCodeOf','trendBadgeHtml'].map(ex).join('\n'));
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n+(g!==undefined?' -> '+g:''));}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+g:''));}};
TV={state:'dn',up:4,dn:16,win:20,slope:-0.4}; let h=trendBadgeHtml('SPY');
ok('DN code',/DN/.test(h)&&!/DN-BRK/.test(h),true); ok('DN count 16/20',/16\/20/.test(h),true);
ok('DN falling slope glyph',h.indexOf('\u2198')!==-1,true); ok('DN colored red',h.indexOf('#f0616d')!==-1,true);
TV={state:'up',up:18,dn:2,win:20,slope:0.3}; h=trendBadgeHtml('SPY');
ok('UP code + count 18/20',/UP/.test(h)&&/18\/20/.test(h),true); ok('UP rising slope',h.indexOf('\u2197')!==-1,true); ok('UP green',h.indexOf('#2ec27e')!==-1,true);
TV={state:'up-broken',up:12,dn:6,win:20,slope:0.05}; h=trendBadgeHtml('SPY');
ok('UP-BRK code amber',/UP-BRK/.test(h)&&h.indexOf('#f2b45a')!==-1,true);
TV={state:'flat',up:8,dn:9,win:20,slope:0}; h=trendBadgeHtml('SPY');
ok('SIDE code + flat slope',/SIDE/.test(h)&&h.indexOf('\u2192')!==-1,true);
TV={state:'na',up:0,dn:0,win:0,slope:0}; h=trendBadgeHtml('SPY');
ok('na shows warming up, no digits',/warming up/.test(h)&&!/\d\/\d/.test(h),true);
console.log(f===0?'\nALL PASS':'\n'+f+' FAILED'); process.exit(f===0?0:1);
