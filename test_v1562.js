// ============================================================================================
// test_v1562.js — (v15.62) THE MOCKUP'S LOOK IS THE PANEL'S LOOK · 📚 LEARN.
//   "for the analysis and testing tabs, use the look and feel that you gave me with the mockups" (the third time)
//   "I also want a learn tab … a deflection learning doc … a scale from 0 to 100"
//   One source: tools/panel-css.py (the mockups' CSS, scoped) == PANEL_CSS; the tabs render in the mockups' own
//   classes and the skeleton of the Analysis tab equals the mockup generator's skeleton for the same subject.
//   tools/learn-seed.py → examples.json == LEARN_SEED; the gauge arithmetic agrees across Python and JS.
// ============================================================================================
const fs=require('fs'), vm=require('vm'), cp=require('child_process');
const SRC=process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js';
const src=fs.readFileSync(SRC,'utf8');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,400):''));} };
const lit=(name)=>{ const i=src.indexOf('var '+name+'='); const j=src.indexOf('\n', i); return new Function(src.slice(i, j)+' return '+name+';')(); };

// ---- 1 · one stylesheet -------------------------------------------------------------------------------
const cssTool=cp.execSync('python3 tools/panel-css.py',{encoding:'utf8'}).trim();
const PANEL_CSS=lit('PANEL_CSS');
ok(PANEL_CSS===cssTool,'1a PANEL_CSS in the script equals tools/panel-css.py (the mockups’ CSS, scoped) — run `python3 tools/panel-css.py --splice`',{script:PANEL_CSS.length,tool:cssTool.length});
ok(/^#gpts-body \.g3pan\{--g-bg:#0b0e14/.test(PANEL_CSS) && /#gpts-body \.g3pan \.sech \.dc\{margin-left:auto/.test(PANEL_CSS) && /#gpts-body \.g3pan \.flow\{display:flex/.test(PANEL_CSS) && /\.g3pan\.g3scaled\{zoom:var\(--g-scale,1\)\}/.test(PANEL_CSS),'1b …scoped under #gpts-body .g3pan: the variables, the section header, the flow strip, the scale');
ok(!/\.wrap\{|h1\{|\.lede\{|\.stage\{|\.scaler\{/.test(PANEL_CSS),'1c …and none of the mockup PAGE’s rules leaked into the panel');
ok(/PANEL_CSS;\s*\/\/ \(v15\.62\)/.test(src) && /'@media \(prefers-reduced-motion: reduce\)\{#gpts-body \.g3pulse\{animation:none;opacity:1\}\}'\+/.test(src) && src.indexOf('PANEL_CSS;')>src.indexOf("#gpts-body .g3swept em{"),'1d the stylesheet is appended to the panel’s style element (after the v15.63 grid rules)');

// ---- 2 · the tabs render in the mockups’ classes ------------------------------------------------------
const { JSDOM }=require('jsdom');
function boot(store){
  const dom=new JSDOM('<!doctype html><html><body></body></html>',{ url:'https://app.skylit.ai/atlas', pretendToBeVisual:true }); const win=dom.window;
  win.matchMedia=q=>({matches:false,media:q,addListener(){},removeListener(){},addEventListener(){},removeEventListener(){}});
  win.requestAnimationFrame=cb=>0; win.cancelAnimationFrame=()=>{};
  Object.defineProperty(win,'localStorage',{ value:{ getItem:k=>(k in store)?store[k]:null, setItem:(k,v)=>{store[k]=String(v);}, removeItem:k=>{delete store[k];}, clear:()=>{}, key:i=>Object.keys(store)[i]||null, get length(){return Object.keys(store).length;} }, configurable:true });
  win.indexedDB=undefined; win.setInterval=()=>0; win.clearInterval=()=>{}; win.setTimeout=()=>0; win.clearTimeout=()=>{}; win.fetch=()=>new Promise(()=>{});
  const i0=src.indexOf('(function(){'), i1=src.lastIndexOf('})();'); const ctx=vm.createContext(win); win.window=win;
  vm.runInContext(src.slice(i0+'(function(){'.length, i1), ctx, { filename:'gex.user.js' });
  const run=c=>vm.runInContext(c,ctx); run('buildPanel()'); return { run, win };
}
const store={};
store['gpts_studies_v1']=fs.readFileSync('learning/studies.json','utf8'); store['gpts_register_v1']=fs.readFileSync('learning/register.json','utf8');
const W=JSON.parse(fs.readFileSync('data/es-1min/SWEEPS.json','utf8')); store['gpts_sweeps_v1']=JSON.stringify({corpus:W.corpus,lookup:W.lookup,ledger:W.ledger,cells:W.cells});
store['gpts_suite_v1']=fs.readFileSync('learning/suite.json','utf8');
store['gpts_asubj_v1']='K';
let B=boot(store); let run=B.run, doc=B.win.document;
run("['K1','K2','K3','K4','K5','K6'].forEach(function(k){ TAB_SECTIONS['sj-'+k]=true; })");   // the mockup shows every section open
run('RENDER_ERRS.length=0'); run('__gptsDebug.showAnalysis(true)');
let errs=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
ok(errs.length===0,'2a the Analysis tab renders with nothing swallowed',errs);
let pan=doc.querySelector('#gpts-body .g3pan.g3scaled');
ok(!!pan,'2b the tab body is wrapped in the mockup’s panel (.g3pan.g3scaled)');
ok(!/Did the dashboard tell the truth\?/.test(doc.body.innerHTML),'2c the header line the mockup never had is gone');
const tabs=[...doc.querySelectorAll('#gpts-body .g3pan .tabs span')].map(e=>e.textContent.trim());
ok(tabs.length===8 && /Analysis/.test(tabs[1]) && /Learn/.test(tabs[3]) && /Rec/.test(tabs[4]) && doc.querySelector('#gpts-body .g3pan .tabs span.on').textContent.indexOf('Analysis')>=0,'2d the tab bar is the mockup’s .tabs: eight tabs (v15.70: Rec after Learn), the active one .on',tabs);
// the mockup generator's skeleton for the same subject
const mockHtml=cp.execSync("python3 -c \"import sys,importlib; sys.path.insert(0,'tools'); M=importlib.import_module('mockup-from-studies'); print(M.panel([s for s in M.S['subjects'] if s['key']=='K'][0]))\"",{encoding:'utf8'});
const mdoc=new JSDOM('<div id="m">'+mockHtml+'</div>').window.document;
function skeleton(root){
  const out=[];
  root.querySelectorAll('.sec').forEach(sec=>{
    const sech=sec.querySelector('.sech'); if(!sech) return;
    const n=(sech.querySelector('.n')||{}).textContent||'';
    if(!/^K\d/.test(n.trim())) return;               // the subject's subsections only (the panel adds TRACK and live evidence)
    out.push('sec '+n.trim()+' ['+[...sech.children].map(c=>c.className).join(',')+'] rows='+sec.querySelectorAll('.sc').length+
      ' row=['+([...(sec.querySelector('.sc')||{children:[]}).children].map(c=>c.className.split(' ')[0]).join(','))+'] rs='+sec.querySelectorAll('.rs').length+' note='+sec.querySelectorAll('.note').length);
  });
  return out;
}
const skPanel=skeleton(pan), skMock=skeleton(mdoc);
ok(skPanel.length===6 && JSON.stringify(skPanel)===JSON.stringify(skMock),'2e the Analysis tab’s skeleton for subject K equals the mockup generator’s — same sections, same header parts, same row parts, same row counts, same result lines',{panel:skPanel,mock:skMock});
ok(doc.querySelectorAll('#gpts-body .g3pan .subj span').length===mdoc.querySelectorAll('.subj span').length && doc.querySelector('#gpts-body .g3pan .subj span.on').textContent===mdoc.querySelector('.subj span.on').textContent,'2f the subject strip: the same seven pills, the same one lit');
ok((doc.querySelector('#gpts-body .g3pan .hd')||{}).textContent.replace(/\s+/g,' ').indexOf(mdoc.querySelector('.hd b').textContent)===0,'2g the .hd line starts with the subject’s name as the mockup’s does');
ok(!!doc.querySelector('#gpts-body .g3pan .sec .sech .n') && !!doc.querySelector('#gpts-body .g3pan .foot') && doc.querySelector('#gpts-body .g3pan .foot').textContent.indexOf('scale')>=0,'2h …sections in .sec/.sech, and the mockup’s .foot with the scale control');
ok(!!doc.querySelector('#gpts-body .g3pan .sec .sech[data-gsec="track-K"]') && !!doc.querySelector('#gpts-track-K'),'2i the TRACK field is a section in the same chrome (his v15.55 ask kept)');
// the scale control persists and the wrapper carries it
run('setPanelScale(1.55)'); ok(store['gpts_tabscale_v1']==='1.55' && /--g-scale:1\.55/.test(doc.querySelector('#gpts-body .g3pan.g3scaled').getAttribute('style')),'2j the scale control persists and the body carries --g-scale');
run('setPanelScale(1)');
// the Testing tab
run('RENDER_ERRS.length=0'); run('__gptsDebug.showTesting(true)');
errs=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
ok(errs.length===0,'2k the Testing tab renders with nothing swallowed',errs);
pan=doc.querySelector('#gpts-body .g3pan.g3scaled');
const titles=[...pan.querySelectorAll('.sec > .sech > .t')].map(e=>e.textContent.trim());
ok(!!pan.querySelector('.flow') && pan.querySelectorAll('.flow > div > b').length===6 && /The loop, today/.test((pan.querySelector('.hd')||{}).textContent),'2l the Testing tab: the mockup’s .hd loop line and the six-cell .flow strip');
ok(JSON.stringify(titles.slice(0,6))===JSON.stringify(['THE REGISTER','THE GATE','ON THE DASHBOARD','THE RECORD','THE NIGHTLY','THE SUITE']),'2m ① – ⑥ in the mockup’s order with the mockup’s titles',titles);
ok(pan.querySelectorAll('table th').length>10 && !!pan.querySelector('.bar') && !!pan.querySelector('.row .k'),'2n the register table, the n/minN bar and the .row/.k/.v rows are the mockup’s');
ok(!/What is being tested, and what would settle it\?/.test(doc.body.innerHTML),'2o the old Testing header is gone');

// ---- 3 · 📚 Learn -----------------------------------------------------------------------------------------
const seed=lit('LEARN_SEED'); const file=JSON.parse(fs.readFileSync('learning/deflections/examples.json','utf8'));
ok(JSON.stringify(seed)===JSON.stringify(file),'3a LEARN_SEED equals learning/deflections/examples.json (edit tools/learn-seed.py, run it, re-splice)');
const md=fs.readFileSync('learning/deflections/LEARNING.md','utf8');
ok(file.examples.every(e=>md.indexOf('### '+e.id+' —')>=0) && file.rules.every(r=>md.indexOf('**'+r.id+' · ')>=0),'3b LEARNING.md carries every example and every rule of the json (one source)');
ok(file.examples.every(e=>fs.existsSync(e.img)),'3c every example’s image is on disk',file.examples.map(e=>e.img));
ok(file.examples.length>=4 && file.examples.every(e=>e.blind===null) && file.rules.some(r=>r.status==='REFUTED') && file.protocol.some(p=>/rewritten to absorb/.test(p)),'3d honest state: four taught examples, none blind; a refuted rule stays; the protocol forbids rewriting a rule to fit its counter-example');
const gPy=JSON.parse(cp.execSync("python3 -c \"exec(open('tools/learn-seed.py').read().split('if __name__')[0]); import json; print(json.dumps(gauge(EXAMPLES, RULES)))\"",{encoding:'utf8'}));
run('__gptsDebug.showLearn(true)'); run('RENDER_ERRS.length=0'); run('__gptsDebug.showLearn(true)');
errs=JSON.parse(run('JSON.stringify(__gptsDebug.renderErrors())')||'[]');
ok(errs.length===0,'3e the Learn tab renders with nothing swallowed',errs);
const gJs=JSON.parse(run('JSON.stringify(__gptsDebug.learn().gauge)'));
ok(gJs.value===gPy.value && gJs.identify===gPy.identify && gJs.predict===gPy.predict && Math.abs(gJs.breadth-gPy.breadth)<0.11 && gJs.confirmed===gPy.confirmed,'3f the gauge arithmetic agrees between Python and the panel (today: '+gPy.value+'/100)',{py:gPy,js:gJs});
ok(gJs.value<10 && gJs.identify===0 && gJs.predict===0,'3g …and it cannot flatter: taught examples move only breadth; identify and predict are zero until earned');
pan=doc.querySelector('#gpts-body .g3pan.g3scaled'); const html=pan.innerHTML;
const lt=[...pan.querySelectorAll(':scope > .sec > .sech > .t')].map(e=>e.textContent.trim());
ok(JSON.stringify(lt.slice(0,5))===JSON.stringify(['THE PROTOCOL','WHAT I HAVE LEARNED','THE FACTORS I CHECK','THE EXAMPLES','WHAT THE CORPUS ALREADY SAYS']),'3h ① – ⑤ of the Learn tab',lt);
ok(new RegExp('>'+gPy.value+'</div><div class="dm"[^>]*>OF 100').test(html) && /identify<\/b> 0\/60/.test(html) && /predict<\/b> 0\/30/.test(html) && /the scorer is not built/.test(html),'3i the gauge shows the number, its three parts and why each is what it is');
const newest=file.examples[file.examples.length-1];
ok(file.examples.every(e=>html.indexOf('data-gsec="lrn-ex-'+e.id+'"')>=0) && html.indexOf(newest.img)>=0 && /taught with the answer; not scored/.test(html) && html.indexOf('<img src="https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main/'+newest.img+'"')>=0,'3j every example is a section; the newest is open with its image from the repo, and a taught example says it is not scored');
ok(file.rules.every(r=>html.indexOf('>'+r.id+'<')>=0) && /REFUTED/.test(html) && /CONFIRMED/.test(html),'3k every rule with its status');
// the fetch path: a file with blind reads moves identify
const D2=JSON.parse(JSON.stringify(file)); for(let i=0;i<6;i++){ D2.examples.push({ id:'T'+i, date:'2026-09-04', chart:'t', img:'', given:'blind', his:'', legs:[], call:{kind:'PULLBACK',summary:''}, blind:{ kind:'PULLBACK', right:(i<5) }, rules:[] }); }
store['gpts_learn_v1']=JSON.stringify(D2);
run('__gptsDebug.showLearn(true)'); const g2=JSON.parse(run('JSON.stringify(__gptsDebug.learn().gauge)'));
const wl=(r,n)=>{ const z=1.96,p=r/n,d=1+z*z/n,c=p+z*z/(2*n),m=z*Math.sqrt((p*(1-p)+z*z/(4*n))/n); return Math.max(0,(c-m)/d); };
ok(g2.blindN===6 && g2.blindRight===5 && Math.abs(g2.identify-60*wl(5,6))<0.15 && g2.identify<30 && g2.value>gJs.value,'3l six blind reads, five right → identify is 60 × the Wilson lower bound ('+(60*wl(5,6)).toFixed(1)+'), not 60 × 83%; the gauge moves',g2);
delete store['gpts_learn_v1'];
// mutual exclusion
run('__gptsDebug.showLearn(true)'); run('__gptsDebug.showDashboard()');
ok(!/THE FACTORS I CHECK/.test(doc.body.innerHTML) && /(the read · from the stats|<em>SWEPT<\/em>)/.test(doc.body.innerHTML),'3m the Dashboard clears the Learn view');
run('__gptsDebug.showLearn(true)'); run('__gptsDebug.showAnalysis(true)');
ok(!/THE FACTORS I CHECK/.test(doc.body.innerHTML) && /TRACK SOMETHING UNDER/.test(doc.body.innerHTML),'3n …and so does Analysis');

// ---- 4 · the installer carries the doc and the images ------------------------------------------------
const manifest=cp.execSync('python3 tools/build-installer.py --list',{encoding:'utf8'});
ok(/learning\/deflections\/examples\.json/.test(manifest) && /learning\/deflections\/LEARNING\.md/.test(manifest) && file.examples.every(e=>manifest.indexOf(e.img)>=0),'4a the installer manifest carries examples.json, LEARNING.md and every image');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
