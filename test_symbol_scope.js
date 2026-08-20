// (v11.4.2) SYMBOL SCOPE — the panel is mapped to SPY/QQQ. Any other instrument's book must be
// recognised as ITSELF and dropped, never renamed to SPY and recorded under the SPY name.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('^var '+n+'\\s*=.*$','m'))[0]; }
global.window={__gptsDebug:{}}; global.LASTDISP={SPY:null,QQQ:null}; global.LASTFEED={SPY:null,QQQ:null}; global.LASTVEX={SPY:null,QQQ:null};
global.observeFeedCadence=()=>{};
eval(v('SYM_SEEN')); eval(v('FEED_REJECTS'));
eval(['symFromUrl','feedNewestT','onFeed'].map(ex).join('\n'));

// ---- 1. the real symbol comes back
ok(symFromUrl('https://api.skylit.ai/gex/levels?symbol=SPY&data_type=gamma')==='SPY', '1a SPY');
ok(symFromUrl('https://api.skylit.ai/gex/levels?symbol=QQQ&data_type=gamma')==='QQQ', '1b QQQ');
ok(symFromUrl('https://api.skylit.ai/gex/levels?symbol=USO&data_type=gamma')==='USO', '1c USO is USO — NOT silently renamed SPY (the v11.4.1 bug)', symFromUrl('...symbol=USO'));
ok(symFromUrl('https://api.skylit.ai/gex/levels?symbol=GLD&v=2')==='GLD', '1d GLD');
ok(symFromUrl('https://api.skylit.ai/gex/levels?data_type=gamma&symbol=iwm')==='IWM', '1e case-insensitive, any parameter order');
ok(symFromUrl('https://api.skylit.ai/gex/levels?nosymbol=1')==='SPY', '1f no symbol in the URL → SPY, as before');

// ---- 2. a foreign book is never written into the SPY state
const NOW=Math.floor(Date.now()/1000);
const spyBook={levels:[{t:NOW-60,s:765.4,l:[{k:767,v:9e7}]}]};
const usoBook={levels:[{t:NOW-60,s:71.2,l:[{k:71,v:4e6}]}]};
onFeed('SPY','gamma',spyBook,false);
ok(LASTFEED.SPY && LASTFEED.SPY.j.levels[0].s===765.4, '2a the SPY book is held');
onFeed('USO','gamma',usoBook,false);
ok(LASTFEED.SPY.j.levels[0].s===765.4, '2b a USO book does NOT overwrite LASTFEED.SPY (it would have been recorded as SPY levels)', LASTFEED.SPY.j.levels[0].s);
ok(LASTFEED.USO===undefined, '2c ...and no USO state is invented either');
ok(SYM_SEEN.USO && SYM_SEEN.USO.n===1, '2d the sighting IS counted, so the panel can say the tape exists but is not mapped', SYM_SEEN.USO);
onFeed('GLD','vanna',usoBook,false);
ok(SYM_SEEN.GLD && SYM_SEEN.GLD.n===1 && LASTVEX.SPY===null, '2e the same guard covers the vanna capture');
onFeed('QQQ','gamma',{levels:[{t:NOW-60,s:520,l:[{k:520,v:5e7}]}]},false);
ok(LASTFEED.QQQ && LASTFEED.QQQ.j.levels[0].s===520, '2f QQQ still works — the mapped pair is unchanged');
console.log('test_symbol_scope: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
