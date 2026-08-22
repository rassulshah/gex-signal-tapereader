// (v11.32) THE NODE CHART — Atlas-style node bands inside the panel.
//
// The user circled three marker rows on an Atlas screenshot (~7706, ~7692.5, ~7686.5) and asked for
// the same picture in the panel, so a pullback into a node is something you SEE rather than infer
// from a list of numbers.
//
// Colour: purple = put-dominant, yellow = call-dominant. %King from Skylit's King cell is SIGNED, so
// polarity comes from the data and is never inferred from where price happens to be sitting.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(a,b,m)=>ok(JSON.stringify(a)===JSON.stringify(b),m,{got:a,want:b});
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}

global.window={};
const T=1787342220000;
global.HIST={ SPY:{}, QQQ:{} };
global.STATE={ SPY:{price:766.0, candles:[]}, QQQ:{} };
global.FUTMODE={ ok:true, fam:'ES', chart:'ES', underlying:'SPY', r:10.05, futPx:7698 };
global.dispIsFut=()=>true; global.dispR=()=>10.05;
global.ifLadder=()=>({err:'off for this test'});
global.g3esc=(x)=>String(x==null?'':x);
global.g3tip=()=>'';
global.dispNum=(x)=>x==null?'–':String(Math.round(x*100)/100);
global.NCHART_MIN=90; global.NCHART_H=132; global.HIST_MAX=130;
eval(ex('closedCandles')); eval(ex('nodeChartHtml'));
// The legend always carries both colour swatches, so a naive indexOf on the whole block would
// report "yellow is present" for a chart with nothing yellow plotted. Assert on the SVG only.
const plot=(sym)=>{ const h=nodeChartHtml(sym||'SPY'); const a=h.indexOf('<svg'), b=h.indexOf('</svg>');
  return (a<0||b<0)?'':h.slice(a,b); };
// (v11.38) The three-zone chart draws GEX and DEX profile RECTS in the same purple/yellow the node
// markers use, so a colour test on the whole SVG would pass on a profile bar. Node markers are the
// triangle <path> elements — assert on those alone.
const marks=(sym)=>(plot(sym).match(/<path d="M[^"]*Z" fill="[^"]*" opacity="[^"]*"\/>/g)||[]).join('');

function bars(n){ const a=[]; for(let i=0;i<n;i++){ const t=T-(n-1-i)*180000, b=765.5+Math.sin(i/4)*0.6;
  a.push({t:t,b:t,o:b,h:b+0.25,l:b-0.25,c:b+(i%2?0.1:-0.1)}); } return a; }
function node(k,v,n){ const seq=[]; for(let i=0;i<n;i++) seq.push({t:T-(n-1-i)*180000, v:v}); HIST.SPY[k.toFixed(2)]={last:T,seq:seq}; }

// ---- it draws ----
{
  STATE.SPY.candles=bars(30);
  node(766.5,-60,20); node(765.0,+70,20);
  const h=nodeChartHtml('SPY');
  ok(h.length>0,'a chart is produced');
  ok(/<svg /.test(h),'as inline SVG — no external renderer');
  ok(/viewBox="0 0 416 \d+"/.test(h),'sized to the panel width, with height set by the layout');
}
// ---- colour carries POLARITY, from the signed value ----
{
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(766.5,+70,20);                       // positive = put-dominant
  let g=marks();
  ok(g.indexOf('#a371f7')>=0,'a put-dominant node is PURPLE');
  ok(g.indexOf('#e3c341')<0,'and no yellow MARKER is plotted for it');

  HIST.SPY={};
  node(766.5,-70,20);                       // negative = call-dominant
  g=marks();
  ok(g.indexOf('#e3c341')>=0,'a call-dominant node is YELLOW');
  ok(g.indexOf('#a371f7')<0,'and no purple MARKER is plotted for it');
}
{
  // the SAME strike flips colour when its sign flips — polarity is the data, not the position
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(764.0,+70,20);                        // below price, put-dominant
  const below=marks();
  HIST.SPY={}; node(766.4,+70,20);           // above price, same sign
  const above=marks();
  ok(below.indexOf('#a371f7')>=0 && above.indexOf('#a371f7')>=0,
     'a put-dominant node is purple whether it sits above or below price — colour is polarity, not support/resistance');
}
// ---- marker DIRECTION matches the colour ----
{
  HIST.SPY={}; node(766.5,+70,20); STATE.SPY.candles=bars(30);
  const put=nodeChartHtml('SPY');
  HIST.SPY={}; node(766.5,-70,20);
  const call=nodeChartHtml('SPY');
  ok(put!==call,'the two polarities render differently, not just in colour');
}
// ---- strength ----
{
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(766.5,+90,20);
  const strong=marks().match(/opacity="(0\.\d+)"/g)||[];
  HIST.SPY={}; node(766.5,+20,20);
  const weak=marks().match(/opacity="(0\.\d+)"/g)||[];
  const maxOf=(a)=>Math.max.apply(null,a.map(s=>parseFloat(s.match(/0\.\d+/)[0])));
  ok(maxOf(strong)>maxOf(weak),'a stronger node is drawn brighter',{strong:maxOf(strong),weak:maxOf(weak)});
}
{
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(766.5,+1,20);                         // 1% of King is noise, not a level
  ok(marks().indexOf('#a371f7')<0,'a node under the noise floor is not drawn at all');
}
// ---- the chart survives thin and missing data ----
{
  HIST.SPY={}; STATE.SPY.candles=bars(2);
  eq(nodeChartHtml('SPY'),'','too few bars produces nothing rather than a broken axis');
}
{
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  const h=nodeChartHtml('SPY');
  ok(h.length>0,'no node history at all still draws the price');
  ok(/still filling/.test(h),'and it SAYS the history is still filling instead of implying there are no nodes');
}
{
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(900,+80,20);                           // a far strike must not flatten the price scale
  ok(nodeChartHtml('SPY').length>0,'a distant node does not break the chart');
  ok(marks().indexOf('#a371f7')<0,'and it is excluded rather than compressing every candle into a line');
}
{
  // history older than the window is clipped, not smeared across it
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  HIST.SPY['766.50']={last:T, seq:[{t:T-8*3600000, v:80}]};
  ok(marks().indexOf('#a371f7')<0,'a sample from outside the window is dropped');
  ok(!/<text[^>]*a371f7/.test(plot()),'and the row is not labelled either — a price with no markers beside it reads as a level that is not there');
}
// ---- the two books stay distinguishable ----
{
  HIST.SPY={}; STATE.SPY.candles=bars(30); node(766.5,+70,20);
  global.ifLadder=()=>({err:null, rows:[{id:'CR·CR0', k:7700, disp:7738, und:766.2},
                                        {id:'PS', k:7650, disp:7688, und:765.1}]});
  const h=nodeChartHtml('SPY');
  ok(/stroke-dasharray/.test(h),'IF levels are DASHED lines — not the same visual language as the Skylit markers');
  ok((h.match(/stroke-dasharray/g)||[]).length>=2,'and each is drawn in two segments so the centred label is never overprinted');
  ok(/>CR [\d.]+</.test(h),'and they are labelled with name AND price, so you never have to remember which book a line came from');
  ok(/IF · structure/.test(h) && /Skylit · flow/.test(h),'the two zones are named on the face — structure left, flow right');
  global.ifLadder=()=>({err:'off for this test'});
}
{
  HIST.SPY={}; STATE.SPY.candles=bars(30); node(766.5,+70,20);
  const h=nodeChartHtml('SPY');
  ok(!/▲call/.test(h),'no legend row on the face — the panel has no vertical space for a key');
}

// ---- (v11.33) A BAND IS A LEVEL YOU CAN READ ------------------------------------------------
// Prices in the right gutter. One placement pass for price, IF levels and nodes together, because
// three independent passes overprint each other into an unreadable stack.
{
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(766.5,-80,20); node(765.2,+70,20);
  global.ifLadder=()=>({err:'off'});
  const g=plot();
  const nums=(g.match(/<text[^>]*text-anchor="middle"[^>]*>([^<]+)<\/text>/g)||[]).map(t=>t.replace(/<[^>]*>/g,''));
  ok(nums.length>=2,'centred labels are drawn',nums);
  ok(nums.some(n=>Math.abs(parseFloat(n)-766.5*10.05)<1),'a strong node still carries its price, converted to the chart instrument',nums);
  ok(nums.some(n=>Math.abs(parseFloat(n)-STATE.SPY.price*10.05)<1),'and current price is labelled too',nums);
}
{
  // a weak node keeps its markers but does not earn a label
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(766.5,+8,20);
  const g=plot();
  ok(g.indexOf('#a371f7')>=0,'a weak node is still plotted');
  const nums=(g.match(/<text[^>]*text-anchor="middle"[^>]*>([^<]+)<\/text>/g)||[]).map(t=>t.replace(/<[^>]*>/g,''));
  ok(!nums.some(n=>Math.abs(parseFloat(n)-766.5*10.05)<1),'but it is not labelled — labels are for levels, not noise',nums);
}
{
  // COLLISION: two strong nodes a hair apart must not overprint
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(766.50,+80,20); node(766.52,+78,20);
  const g=plot();
  const nums=(g.match(/<text[^>]*text-anchor="middle"[^>]*>([^<]+)<\/text>/g)||[]).map(t=>t.replace(/<[^>]*>/g,''));
  const near=nums.filter(n=>Math.abs(parseFloat(n)-766.5*10.05)<3);
  ok(near.length<=1,'only one of two overlapping rows is labelled',nums);
  const marks=(g.match(/#a371f7/g)||[]).length;
  ok(marks>=40,'while BOTH rows keep every marker — the data is not hidden, only the duplicate number',marks);
}
{
  // priority: price and the IF levels get the gutter before the nodes do
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  node(765.15,+80,20);
  global.ifLadder=()=>({err:null, rows:[{id:'PS', k:7650, disp:7688, und:765.12}]});
  const g=plot();
  ok(/PS 7688/.test(g),'the IF level is labelled with its NAME and its price',(g.match(/<text[^>]*>[^<]*<\/text>/g)||[]).slice(0,2));
  global.ifLadder=()=>({err:'off'});
}

// ---- (v11.39) FLOW BARS POINT INWARD AND SHOW GROWTH AS SEGMENTS ------------------------------
// Two corrections. Bars grow LEFTWARD from the right edge so both profiles point in toward price.
// And growth is the SEGMENTS, not a tick mark: dim = held over an hour, mid = added 60m..15m ago,
// bright = the last 15 minutes. A node accumulating shows a bright leading edge.
{
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  // a node that has grown steadily: small an hour ago, larger 15m ago, largest now
  const seq=[]; for(let i=0;i<30;i++) seq.push({t:T-(29-i)*180000, v:20+i*2.5});
  HIST.SPY['766.50']={last:T,seq:seq};
  const g=plot();
  const rects=(g.match(/<rect[^>]*height="4\.4"[^>]*\/>/g)||[]);
  ok(rects.length>=3,'a growing node draws three growth segments, not one bar',rects.length);
  const ops=rects.map(r=>parseFloat((r.match(/opacity="([\d.]+)"/)||[])[1]));
  ok(ops.includes(0.3)&&ops.includes(0.58)&&ops.includes(0.95),'dim / mid / bright — oldest to newest',ops);
  // inward: the widest segment must START further left than the narrowest
  const xs=rects.map(r=>parseFloat((r.match(/x="([\d.]+)"/)||[])[1]));
  ok(Math.min(...xs)<Math.max(...xs),'segments stack leftward from a common right edge',xs);
  const rightEdges=rects.map(r=>parseFloat((r.match(/x="([\d.]+)"/)||[])[1])+parseFloat((r.match(/width="([\d.]+)"/)||[])[1]));
  ok(Math.max(...rightEdges)<=412.1,'and nothing spills past the right edge of the panel',Math.max(...rightEdges));
}
{
  // a node that shrank gets its lost ground marked, so decay is visible rather than merely absent
  HIST.SPY={}; STATE.SPY.candles=bars(30);
  const seq=[]; for(let i=0;i<30;i++) seq.push({t:T-(29-i)*180000, v:90-i*2.2});
  HIST.SPY['766.50']={last:T,seq:seq};
  const g=plot();
  ok(/<line[^>]*opacity="0\.35"/.test(g),'a bleeding node shows the ground it lost');
}
{
  HIST.SPY={}; STATE.SPY.candles=bars(30); node(766.5,+70,20);
  const h=nodeChartHtml('SPY');
  ok(!/call-dominant/.test(h.replace(/title="[^"]*"/g,'')),'the legend ROW is gone from the face — there is no vertical space for a key');
  ok(/KEY — green bars are positive gamma/.test(src),'and the key lives in the chart hover instead');
}
console.log('\n'+pass+' pass / '+fail+' fail');
