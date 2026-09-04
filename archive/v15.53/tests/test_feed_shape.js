// (v11.13) THE FEED SHAPE PROBE. It exists to settle one question with data: can InsiderFinance's levels
// be computed from the SPXW lane we already receive? That turns on whether the payload ever splits gamma
// into CALL and PUT — their Call Wall is defined on call gamma specifically, so with no split it is not
// reproducible from any amount of our data, and with a split it becomes computable without scraping anyone.
// The probe reports SHAPE ONLY — key names, counts, ranges, two sample rows — never the payload itself.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.window={__gptsDebug:{}};
eval(['feedKeyScan','feedShape'].map(ex).join('\n'));
global.LASTFEEDURL='https://app.skylit.ai/api/gex/levels?symbol=SPY';

const lane=(ks,extra)=>({ levels:[{t:1,l:[]},{t:2,l:ks.map(k=>Object.assign({k:k,v:1000*k,d:1},extra||{}))}] });

// ---------- no call/put anywhere: the verdict must be unambiguous ----------
{
  global.LASTFEED={ SPY:{ j:Object.assign(lane([760,765,770]), { derived:[ Object.assign({source:'SPXW', ratio:0.0995}, lane([7600,7650,7700])) ] }) } };
  const s=feedShape('SPY');
  ok(s.native.strikes===3,'the native lane strike count is reported',s.native.strikes);
  ok(s.native.kMin===760 && s.native.kMax===770,'and its strike range',[s.native.kMin,s.native.kMax]);
  ok(s.native.strikeStep===5,'and the strike step, which says how fine the grid is',s.native.strikeStep);
  ok(s.derivedLanes.length===1 && s.derivedLanes[0].source==='SPXW','the SPXW lane is found',s.derivedLanes);
  ok(s.derivedLanes[0].shape.kMin===7600,'reported on ITS OWN scale, unconverted',s.derivedLanes[0].shape.kMin);
  ok(s.derivedLanes[0].ratio===0.0995,'with the ratio it carries');
  ok(s.callPutKeys.length===0,'no call/put fields found',s.callPutKeys);
  ok(/not reproducible/.test(s.verdict),'and the verdict says the Call Wall cannot be reproduced',s.verdict);
  ok(Array.isArray(s.native.sample) && s.native.sample.length===2,'exactly two sample rows — shape, not a dump',s.native.sample&&s.native.sample.length);
  ok(s.native.rowKeys.join(',')==='k,v,d','the row keys are named so the fields are auditable',s.native.rowKeys);
}
// ---------- call/put present: the verdict must flip, not stay pessimistic ----------
{
  global.LASTFEED={ SPY:{ j:Object.assign(lane([760,765],{callGex:5,putGex:-7}), { derived:[] }) } };
  const s=feedShape('SPY');
  ok(s.callPutKeys.length>0,'call/put fields are detected when present',s.callPutKeys);
  ok(/PRESENT/.test(s.verdict),'and the verdict flips',s.verdict);
  ok(s.callPutKeys.some(h=>/callGex/.test(h.path)),'the path to the field is reported so we know if it is per-strike',s.callPutKeys);
}
// ---------- the scan must reach nested shapes without dumping them ----------
{
  const hits=feedKeyScan({a:{b:{c:{putWall:7}}}}, /put/i);
  ok(hits.length===1 && hits[0].path==='a.b.c.putWall','nested keys are found with a full path',hits);
  // the defect this pins: the feed's `levels` array is a time series whose FIRST snapshot is often empty,
  // so scanning index 0 alone reported "no call/put fields" off a payload that had them in the last one.
  const tail=feedKeyScan({levels:[{l:[]},{l:[{k:1,callGex:5}]}]}, /call/i);
  ok(tail.length===1,'a field only present in the LAST array element is still found',tail);
  ok(/\[1\]/.test(tail[0].path),'and the path names which element it was found in',tail[0].path);
  const deep=feedKeyScan({a:{b:{c:{d:{e:{putWall:7}}}}}}, /put/i);
  ok(deep.length===0,'but the scan stops at depth 4 rather than walking a huge payload forever',deep);
  const many={}; for(let i=0;i<40;i++) many['put'+i]=i;
  ok(feedKeyScan(many,/put/i).length===12,'and caps at 12 hits so the output stays readable',feedKeyScan(many,/put/i).length);
  ok(feedKeyScan(null,/put/i).length===0,'a null object scans to nothing rather than throwing');
}
// ---------- refusal beats a misleading answer ----------
{
  global.LASTFEED={};
  ok(/no feed captured/.test(feedShape('SPY').err||''),'with no feed it says so rather than reporting an empty book',feedShape('SPY'));
  global.LASTFEED={ SPY:{ j:{ levels:[] , derived:[]} } };
  const s=feedShape('SPY');
  ok(s.native.strikes===0 && s.native.kMin===null,'an empty lane reports zero strikes and a null range, not 0/0',[s.native.strikes,s.native.kMin]);
}
console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
