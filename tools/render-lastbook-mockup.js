const {chromium}=require('playwright'); const fs=require('fs');
const C=JSON.parse(fs.readFileSync('/tmp/lad.json','utf8'));
C.LAD_W=618; C.LAD_ROC=534; C.LAD_ROCW=84; C.LAD_ROLL=84;
// REAL book measured on the live tape 2026-08-27: SPXW King 7690, kingKd 12,680 ($12.68M).
// %King values are the operator's own tape. ⚠ The DELTA DOLLARS are ILLUSTRATIVE - every d15 on the
// page was 0 after the close, which is the whole point of the feature, so they cannot be measured
// from tonight. They are labelled as such on the mockup itself.
const KING_USD=12.68e6, DFULL=KING_USD*0.45;
const N=[
 {k:7800,p:-74,st:'WEAKENING',mk:'',      tap:0, d:-2.6e6, roc:'−4% −11% ▲6%'},
 {k:7750,p: 74,st:'BUILDING', mk:'',      tap:0, d: 3.9e6, roc:'+6% +14%'},
 {k:7730,p: 22,st:'',         mk:'INPLAY',tap:1, d: 0.4e6, roc:'+1% +3%'},
 {k:7715,p: 50,st:'BUILDING', mk:'DEFENDING',tap:2, d: 1.8e6, roc:'+5% +9%'},
 {k:7700,p:-84,st:'TURN DN',  mk:'BREAKING', tap:1, d:-1.2e6, roc:'−3% −7% ▲4%'},
 {k:7690,p:-100,st:'',        mk:'ATTRACTING',tap:0,d: 5.2e6, roc:'+9% +18%', role:'KING'},
 {k:7670,p:-49,st:'SPENT',    mk:'',      tap:3, d:-0.9e6, roc:'−2% −5%'}];
const LEV={7700:'PDH · CW0',7690:'PDC',7670:'IBL · PW',7750:'CW'};
const KINGS=[{k:7690,book:'SPXW',taps:2},{k:7689,book:'SPY',taps:0},{k:7716,book:'QQQ',approx:1,taps:0}];
const LO=7655,HI=7815,H=250,NOW=7730.11,EH=7752,EL=7708;
const y=p=>(H-((p-LO)/(HI-LO))*H);
const F="-apple-system,'Segoe UI',Roboto,Helvetica,Arial,sans-serif";
function panel(mode){
  const live=(mode==='live');   // live = tonight (flat) ; frozen = after tomorrow's close
  let h=`<div class="wrap"><div class="lad">`;
  h+=`<i class="chute"></i><i class="dbase"></i><span class="dtag">15M $</span>`;
  [[EH,'EH'],[EL,'EL']].forEach(e=>{ h+=`<i class="eml" style="top:${y(e[0])}px"></i>`; });
  N.forEach(n=>{
    const t=y(n.k), pct=Math.abs(n.p), neg=n.p<0, k=neg?'acc':'brk';
    const len=Math.max(9,pct/100*C.LAD_NMAX), txt=(neg?'−':'+')+pct+'%';
    const role=n.role||'';
    const pctW=txt.length*5.6, roleW=role?role.length*6.2:0;
    const roomPct=len>=Math.max(C.LAD_PCT_IN_BAR,pctW+8), roomTyp=role&&len>=(pctW+roleW+11);
    const barTip=C.LAD_NODE+len+2, fitL=(x,w)=>{const m=C.LAD_CH-2-w; return x<=m?x:(m>=barTip?m:null);};
    if(LEV[n.k]) h+=`<span class="lv" data-c="lv" style="top:${t}px">${LEV[n.k]}</span>`;
    h+=`<span class="px" data-c="px" style="top:${t}px">${n.k}</span>`;
    h+=`<i class="bar ${k}" data-c="bar" style="top:${t}px;width:${len}px">${roomPct?'<b>'+txt+'</b>':''}${roomTyp?role:''}</i>`;
    if(!roomPct){const L=fitL(C.LAD_NODE+len+4,pctW+2); if(L!=null) h+=`<span class="kp ${k}" data-c="kp" style="top:${t}px;left:${L}px">${txt}</span>`;}
    if(role&&!roomTyp){const L=fitL(C.LAD_NODE+len+(roomPct?4:34),roleW+2); if(L!=null) h+=`<span class="role" data-c="role" style="top:${t}px;left:${L}px">${role}</span>`;}
    if(n.mk) h+=`<span class="mk" data-c="mk" style="top:${t}px">${n.mk}</span>`;
    if(n.tap) h+=`<span class="tap t${Math.min(n.tap,3)}" data-c="tap" style="top:${t}px">${n.tap}×</span>`;
    // THE DIFFERENCE BETWEEN THE TWO PANELS IS EXACTLY THIS BLOCK AND THE STATE/ROC COLUMNS.
    if(!live){
      const up=n.d>0, mag=Math.max(2,Math.min(Math.abs(n.d),DFULL)/DFULL*C.LAD_DMAX);
      const usd=(Math.abs(n.d)>=1e6?('$'+(Math.abs(n.d)/1e6).toFixed(1)+'M'):('$'+Math.round(Math.abs(n.d)/1e3)+'K'));
      h+=`<i class="dbar ${up?'up':'dn'}" data-c="dbar" style="top:${t}px;left:${C.LAD_DAX-mag}px;width:${mag}px"></i>`;
      h+=`<span class="dval ${up?'up':'dn'}" data-c="dval" style="top:${t}px">${up?'+':'−'}${usd}</span>`;
      if(n.st) h+=`<span class="st s${n.st[0]}" data-c="st" style="top:${t}px">${n.st}</span>`;
      h+=`<span class="roc" data-c="roc" style="top:${t}px">${n.roc}</span>`;
    } else {
      h+=`<span class="roc flat" data-c="roc" style="top:${t}px">0% 0%</span>`;
    }
  });
  // mirrors ladderHtml: the nudge list is SEEDED with the EM pill rows, because the crowns and the
  // expected-move edges share one chute and are the same collision.
  // mirrors ladderHtml v14.56 / mockup-ladder-v11.html:343-354 — crowns nudge among THEMSELVES and
  // record where they land; the EM labels are emitted LAST and step around the crowns AND price.
  const CHUTEY=[]; const used=[];
  KINGS.slice().sort((a,b)=>a.k-b.k).forEach(K=>{
    let t=y(K.k),g=0; while(g++<8&&used.some(u=>Math.abs(u-t)<15)) t+=15; used.push(t); CHUTEY.push(t);
    h+=`<span class="king k${K.book}${K.approx?' approx':''}" data-c="king" style="top:${t}px"><b>${K.approx?'~':''}${K.k}</b><i>${K.book}</i>${(K.taps&&!live)?`<b class="kt">${K.taps}×</b>`:''}</span>`;
  });
  h+=`<span class="now" data-c="now" style="top:${y(NOW)}px">${NOW.toFixed(0)}</span>`;
  CHUTEY.push(y(NOW));
  [[EH,'EH'],[EL,'EL']].forEach(e=>{ let t=y(e[0]),g=0;
    while(g++<4 && CHUTEY.some(u=>Math.abs(u-t)<15)) t+=15;
    if(CHUTEY.some(u=>Math.abs(u-t)<15)) return;   // nowhere clear: the amber line still marks it
    CHUTEY.push(t);
    h+=`<span class="empill" data-c="empill" style="top:${t}px">${e[1]} ${e[0]}</span>`; });
  h+=`</div></div>`;
  const foot = live
    ? `<div class="foot"><span class="fdim">SPXW · 0DTE rolled to 08-28 · velocities 0</span></div>`
    : `<div class="foot"><span class="fbadge">● 2026-08-27 book — frozen 15:00</span></div>`;
  return h+foot;
}
let doc=`<style>
 body{margin:0;background:#0d1117;color:#e6edf3;font-family:${F};font-variant-numeric:tabular-nums;padding:16px}
 .cols{display:flex;gap:26px;align-items:flex-start}
 h2{font-size:12px;letter-spacing:.08em;text-transform:uppercase;color:#8b98a9;margin:0 0 2px}
 .sub{font-size:10.5px;color:#5b6675;margin:0 0 8px;max-width:470px;line-height:1.5}
 .wrap{overflow-x:auto;width:454px;background:#11161f;border:1px solid #2a3340;border-radius:5px;padding:8px 0}
 .lad{position:relative;min-width:${C.LAD_W}px;height:${H}px}
 .chute{position:absolute;left:${C.LAD_CH}px;width:${C.LAD_CHW}px;top:0;bottom:0;background:rgba(255,255,255,.035);border-left:1px solid #333e4d;border-right:1px solid #333e4d}
 .dbase{position:absolute;left:${C.LAD_DAX}px;top:0;bottom:0;width:1px;background:#4a5568}
 .dtag{position:absolute;left:${C.LAD_DAX+4}px;top:0;font-size:6px;font-weight:900;letter-spacing:.08em;color:#5b6675;background:#11161f;padding:0 3px;border-radius:2px}
 .lv{position:absolute;left:0;width:${C.LAD_LVL}px;text-align:right;font-size:8.4px;font-weight:800;color:#8b98a9;white-space:nowrap;transform:translateY(-50%)}
 .px{position:absolute;left:${C.LAD_PXC}px;width:${C.LAD_PXW}px;text-align:right;font-size:8.4px;font-weight:800;color:#c9d1da;transform:translateY(-50%)}
 .bar{position:absolute;left:${C.LAD_NODE}px;height:12px;border-radius:2px;transform:translateY(-50%);display:flex;align-items:center;justify-content:space-between;padding:0 3px 0 4px;box-sizing:border-box;font-size:8px;font-weight:900;overflow:hidden}
 .bar.acc{background:#a371f7;color:#1b1030}.bar.brk{background:#e3c341;color:#2a2408}
 .kp{position:absolute;font-size:8px;font-weight:800;transform:translateY(-50%);white-space:nowrap}
 .kp.acc{color:#a371f7}.kp.brk{color:#e3c341}
 .role{position:absolute;font-size:8px;font-weight:900;transform:translateY(-50%);white-space:nowrap;color:#a371f7}
 .eml{position:absolute;left:${C.LAD_CH-6}px;width:${C.LAD_CHW+12}px;height:1px;background:#f2b45a;opacity:.75}
 .empill{position:absolute;left:${C.LAD_CH+2}px;width:${C.LAD_CHW-4}px;font-size:7.4px;font-weight:900;color:#f2b45a;background:rgba(242,180,90,.12);border:1px solid rgba(242,180,90,.55);border-radius:7px;padding:0 4px;height:13px;box-sizing:border-box;transform:translateY(-50%);white-space:nowrap;z-index:5;display:flex;align-items:center;justify-content:center}
 .king{position:absolute;left:${C.LAD_CH+2}px;width:${C.LAD_CHW-4}px;font-size:8px;font-weight:900;border-radius:7px;padding:0 3px;height:14px;box-sizing:border-box;transform:translateY(-50%);white-space:nowrap;z-index:6;display:flex;align-items:center;gap:2px}
 .king b{font-weight:900;font-size:8px}.king i{font-style:normal;font-size:5.6px;font-weight:900;opacity:.85}
 .kSPXW{color:#e3c341;background:rgba(227,195,65,.14);border:1px solid rgba(227,195,65,.6)}
 .kSPY{color:#cdb4fa;background:rgba(205,180,250,.14);border:1px solid rgba(205,180,250,.6)}
 .kQQQ{color:#5fd3bc;background:rgba(95,211,188,.12);border:1px dashed rgba(95,211,188,.7)}
 .kt{margin-left:auto;font-size:6.4px;font-weight:900;line-height:9px;padding:0 3px;border-radius:5px;background:rgba(242,180,90,.22);color:#f2b45a}
 .now{position:absolute;left:${C.LAD_CH+2}px;width:${C.LAD_CHW-4}px;height:15px;background:#fff;color:#0d1117;border-radius:8px;font-size:9px;font-weight:900;display:flex;align-items:center;justify-content:center;transform:translateY(-50%);z-index:9}
 .mk{position:absolute;left:${C.LAD_MK}px;width:${C.LAD_MKW}px;font-size:7px;font-weight:900;transform:translateY(-50%);white-space:nowrap;background:#11161f;padding:0 2px;box-sizing:border-box;border-radius:3px;z-index:7;color:#4fd1e0;overflow:hidden}
 .tap{position:absolute;left:${C.LAD_TAP}px;width:${C.LAD_TAPW}px;font-size:7px;font-weight:900;transform:translateY(-50%);padding:0 3px;border-radius:5px;background:rgba(255,255,255,.10);box-sizing:border-box;color:#c9d1da}
 .tap.t2{color:#f2b45a;background:rgba(242,180,90,.20)}.tap.t3{color:#e0645f;background:rgba(224,100,95,.24)}
 .dbar{position:absolute;height:9px;transform:translateY(-50%);border-radius:1px 0 0 1px}
 .dbar.up{background:#2ec27e}.dbar.dn{background:#e0645f}
 .dval{position:absolute;left:${C.LAD_DLAB}px;width:${C.LAD_DLABW}px;font-size:7.4px;font-weight:900;transform:translateY(-50%);white-space:nowrap}
 .dval.up{color:#2ec27e}.dval.dn{color:#e0645f}
 .st{position:absolute;left:${C.LAD_ST}px;width:${C.LAD_STW}px;font-size:8.4px;font-weight:900;transform:translateY(-50%);white-space:nowrap;color:#f2b45a}
 .st.sB{color:#7cc7ff}.st.sT{color:#e0645f}.st.sS{color:#6c7889}
 .roc{position:absolute;left:${C.LAD_ROC}px;font-size:8.4px;font-weight:800;transform:translateY(-50%);white-space:nowrap;color:#2ec27e}
 .roc.flat{color:#39424e}
 .foot{width:454px;margin-top:6px;font-size:9px;font-weight:800}
 .fdim{color:#5b6675}
 .fbadge{color:#7cc7ff}
 .note{font-size:10.5px;color:#8b98a9;line-height:1.65;max-width:960px;margin-top:18px;border-top:1px solid #2a3340;padding-top:12px}
 .note b{color:#e6edf3}.note .w{color:#f2b45a}
</style>
<div class="cols">
 <div><h2>Tonight — what you have</h2><div class="sub">The close rolled the chain to 08-28. Every 15m delta is 0, so STATE, the delta column and the roll arrows have nothing to say. The panel is <b>flat, not blank</b>.</div>${panel('live')}</div>
 <div><h2>After tomorrow's close — v14.55</h2><div class="sub">The last healthy reading of the session is served instead, badged with the clock time it froze. Same columns, real numbers, nothing live.</div>${panel('frozen')}</div>
</div>
<div class="note">
 <b>%King values are your real tape</b> — SPXW King 7690, kingKd 12,680 ($12.68M), measured at 17:11 CT.
 <span class="w">⚠ The delta dollars are ILLUSTRATIVE</span> — every d15 on the page was 0 after the close, which is
 the whole reason this feature exists, so they could not be measured from tonight.<br>
 <b>Both panels are 454px wide, your real panel body</b>, so the horizontal scroll you see is the honest 164px.
 The ladder is 618 and the container scrolls rather than clips — nothing is ever dropped silently.
</div>`;
fs.writeFileSync('/tmp/lbmock.html',doc);
(async()=>{
  const b=await chromium.launch({executablePath:'/opt/pw-browsers/chromium'});
  const p=await b.newPage({viewport:{width:1020,height:460}});
  const errs=[]; p.on('pageerror',e=>errs.push(String(e)));
  await p.goto('file:///tmp/lbmock.html');
  const R=await p.evaluate(C=>{
    const out=[];
    document.querySelectorAll('.lad').forEach((lad,pi)=>{
      const els=[...lad.querySelectorAll('[data-c]')].map(e=>{const r=e.getBoundingClientRect();
        return {c:e.dataset.c,txt:(e.textContent||'').trim().slice(0,14),l:+r.left.toFixed(1),rt:+r.right.toFixed(1),t:+r.top.toFixed(1),b:+r.bottom.toFixed(1)};});
      const hits=[]; for(let i=0;i<els.length;i++)for(let j=i+1;j<els.length;j++){const a=els[i],z=els[j];
        if(a.l<z.rt&&z.l<a.rt&&a.t<z.b&&z.t<a.b) hits.push(a.c+'"'+a.txt+'" x '+z.c+'"'+z.txt+'"');}
      const lr=lad.getBoundingClientRect(), CL=lr.left+C.LAD_CH, CR=lr.left+C.LAD_CH+C.LAD_CHW;
      const allow=['king','empill','now'];
      const intr=els.filter(e=>!allow.includes(e.c)&&e.l<CR&&CL<e.rt).map(e=>e.c+'"'+e.txt+'"');
      out.push({panel:pi,n:els.length,hits,intr});
    });
    return out;
  },C);
  await p.screenshot({path:'mockups/lastbook-v1455-MOCKUP.png',fullPage:true});
  await b.close();
  console.log('pageerrors:',errs.length?errs:'none');
  R.forEach(r=>console.log(`panel ${r.panel}: ${r.n} elements | overlaps: ${r.hits.length?r.hits.join(' , '):'none'} | chute intrusions: ${r.intr.length?r.intr.join(' , '):'none'}`));
})();
