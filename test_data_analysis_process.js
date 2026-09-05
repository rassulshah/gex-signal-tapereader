// ============================================================================================
// test_data_analysis_process.js — THE DATA ANALYSIS PROCESS CANNOT BE FORGOTTEN. Operator, 2026-09-04: "How will you make
//   sure that this process is not forgotten by future contexts" · "i want to fine tune everything so that we don't have to
//   play around changing the structure that we are deciding on now and that is solid". The answer this project has learned:
//   a rule enforced by a test is followed; a rule enforced by a checklist is followed until it is busy. So: the seven links
//   must be in the document, the panel's plan, PROCESS.md, the config and the skill — or the suite goes red.
// ============================================================================================
const fs=require('fs');
let pass=0, fail=0;
const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g).slice(0,400):''));} };
const src=fs.readFileSync(process.env.GPTS_SRC||'./current/gex-signal-tapereader.user.js','utf8');
const LINKS=['CAPTURE','ANALYSIS','TESTING','LEARNING','REC','DASHBOARD','SCORE'];
const TABS=['Dashboard','Analysis','Testing','Learn','Rec','Architecture','Roadmap','Open Items'];

const doc=fs.existsSync('design/DATA-ANALYSIS-PROCESS.md')?fs.readFileSync('design/DATA-ANALYSIS-PROCESS.md','utf8'):'';
ok(doc.length>3000,'d1 design/DATA-ANALYSIS-PROCESS.md exists and is a document, not a stub',doc.length);
ok(LINKS.every((l,i)=>new RegExp('\\*\\*'+(i+1)+' · '+l+'\\*\\*').test(doc)),'d2 the seven links are in the document, numbered, in order',LINKS.filter((l,i)=>!new RegExp('\\*\\*'+(i+1)+' · '+l+'\\*\\*').test(doc)));
ok(/one definition travels all seven/.test(doc) && /The operator's one step:\*\* none, since v15\.71 — the panel writes the day itself after the close/.test(doc) && /the 💾 remains as the override/.test(doc) && /The one stage that still waits for a session:\*\* the review/.test(doc),'d3 one definition end to end · no step at the close (v15.71; the 💾 the override) · the one stage that still waits for a session — all said');
ok(/\*\*confirmed\*\*/.test(doc) && /\*\*provisional\*\*/.test(doc) && /\*\*doctrine\*\*/.test(doc) && /\*\*descriptive\*\*/.test(doc),'d4 the four degrees of knowledge');
ok((doc.match(/^\d+\. /gm)||[]).length>=10 && /Nothing on the face changes except through Rec/.test(doc) && /learning\/markets\.json/.test(doc),'d5 the ten rules, Rec as the only path to the face, the markets file');
ok(TABS.every(t=>doc.includes(t)) && /No ninth tab/.test(doc),'d6 the eight tabs, the final set — no ninth');
ok(/just click on the save button once a day/.test(doc) && /lets call the process Data Analysis process/.test(doc),'d7 his words are quoted, not paraphrased');

// the panel: the plan carries the process; the eight tabs exist; the ⚙ tab names the document
const P=JSON.parse(/var PLAN_SEED=(\{.*?\});\n/.exec(src)[1]);
ok(P.process && P.process.name==='the Data Analysis process' && P.process.doc==='design/DATA-ANALYSIS-PROCESS.md' && JSON.stringify(P.process.links)===JSON.stringify(LINKS),'p1 PLAN_SEED.process names the process, its document and the seven links');
ok(P.tabs.length===8 && JSON.stringify(P.tabs.map(t=>t.tab))===JSON.stringify(['Dashboard','Analysis','Testing','Architecture','Roadmap','Open Items','Learn','Rec']),'p2 the plan lists exactly eight tabs',P.tabs.map(t=>t.tab));
const bar=(src.match(/function analysisTabBar\(\)\{[\s\S]*?\n\}/)||[''])[0];
ok(/tab\('Dashboard'/.test(bar) && /Analysis'/.test(bar) && /Testing'/.test(bar) && /Learn'/.test(bar) && /Rec'/.test(bar) && /Architecture'/.test(bar) && /Roadmap'/.test(bar) && /Open Items'/.test(bar) && (bar.match(/tab\(/g)||[]).length===9,'p3 the tab bar renders the eight tabs (and the tab() helper) — no ninth',(bar.match(/tab\(/g)||[]).length);
ok(/Source: design\/DATA-ANALYSIS-PROCESS\.md \(the process, named 2026-09-04\)/.test(src),'p4 the ⚙ Architecture tab names the document as its first source');
ok(/REC_VIEW/.test(src) && /function recBlock\(\)/.test(src) && /reco:\(function\(\)\{ try\{ return recoExport\(dk\); \}/.test(src),'p5 the Rec tab exists and his decisions ride the day export');

// the loop document, the config, the skill
const pr=fs.readFileSync('design/PROCESS.md','utf8');
ok(/THE PROCESS HAS A NAME: the Data Analysis process/.test(pr) && LINKS.every(l=>pr.includes(l)),'l1 design/PROCESS.md names the process and its seven links');
const cfg=JSON.parse(fs.readFileSync('.gex-config.json','utf8'));
ok(cfg.theWhatAndTheHow && /design\/DATA-ANALYSIS-PROCESS\.md/.test(cfg.theWhatAndTheHow.process||'') && LINKS.every(l=>(cfg.theWhatAndTheHow.process||'').includes(l)) && (cfg.theWhatAndTheHow.pinnedBy||[]).includes('test_data_analysis_process.js'),'l2 .gex-config.json points a load at the document and names this test');
const skill=fs.readFileSync('skills/gex/SKILL.md','utf8');
ok(/design\/DATA-ANALYSIS-PROCESS\.md/.test(skill) && /Read it\nFIRST, in full/.test(skill.replace(/\r/g,'')) && /the nightly runs on HIS\nmachine/i.test(skill.replace(/\r/g,'')),'l3 the load procedure reads the document first and says the nightly runs on his machine');
const arch=fs.readFileSync('design/ARCHITECTURE.md','utf8');
ok(/The process this machinery runs is the Data Analysis process/.test(arch) && LINKS.every(l=>arch.includes(l)),'l4 the generated ARCHITECTURE.md carries the process and its links');
// the markets file
const M=JSON.parse(fs.readFileSync('learning/markets.json','utf8'));
ok(M.schema===1 && M.ledgerMarket==='SPY' && M.markets.SPY && M.markets.SPY.status==='live' && M.markets.GC && M.markets.GC.status==='price only' && M.markets.NQ && M.markets.CL,'m1 learning/markets.json: SPY live; NQ · GC · CL price only',Object.keys(M.markets));
ok(M.markets.SPY.turnTolPts===0.5 && M.markets.SPY.contPts===0.3 && M.markets.SPY.fwdBars===10 && M.markets.SPY.minBarsDay===20 && M.markets.SPY.barMinutes===3,'m2 the SPY entry carries the numbers the code used to hard-code');
const dcp=parseFloat((src.match(/var DEFL_CONT_PTS\s*=\s*([\d.]+)/)||[])[1]), dfb=parseInt((src.match(/var DEFL_FWD_BARS\s*=\s*(\d+)/)||[])[1],10);
ok(dcp===M.markets.SPY.contPts && dfb===M.markets.SPY.fwdBars,'m3 the panel’s DEFL_CONT_PTS / DEFL_FWD_BARS equal the SPY entry (one place for the numbers)',[dcp,dfb]);
const pt=fs.readFileSync('tools/nightly/patterns.py','utf8');
ok(/def market_config\(root=ROOT\):/.test(pt) && /TURN_TOL, BAR_MS, MIN_BARS = _MC\['turnTol'\], _MC\['barMs'\], _MC\['minBars'\]/.test(pt),'m4 patterns.py reads the ledger market’s tolerances from markets.json');

console.log('\n'+pass+' passed, '+fail+' failed'); process.exit(fail?1:0);
