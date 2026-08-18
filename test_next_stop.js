// v11.1 — NEXT STOP. User: "a header above the read saying Next Stop: 7789 ... until you have data
// use whatever you can ... data is collected and analyzed and tested and refined and the llm also
// tries to improve this." Pins: the picker order, the grade rules, the header line, the two-horizon
// features + outcome (touch within zone / approach / wrongFirst), Analysis tiles, rules seed.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};
function mul(a,b){return a/(1/b);} global.mul=mul;
global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d',sub:'#8b98a9',ink:'#e6edf3',line:'#21262d',gold:'#f2cc60',blue:'#4a90d9',amber:'#f2b45a'};
global.PB_MIN_PCT=20; global.DEFLECT_ZONE=0.5; global.RULE_UNLOCK_N=20; global.FEAT_FWD=10; global.DIR_PTS=0.5;
global.dispIsFut=()=>false; global.futMark=()=>''; global.dispR=()=>1; global.fmtFut=String; global.window={};
global.effN=(n)=>Math.floor(n/10);
eval(['fmtNum','fmtLvl','fmtSpan','nextStopPick','nextStopHtml'].map(ex).join('\n'));
// NEXTSTOP_RULES var
eval(src.slice(src.indexOf('var NEXTSTOP_RULES='), src.indexOf('};', src.indexOf('var NEXTSTOP_RULES='))+2));

let LEG=null, DIR=null, MODEL=null, FLOW=null, TRIG='';
global.STATE={SPY:{price:768.3}};
global.legEngine=()=>LEG; global.directionGrade=()=>DIR; global.nodeMapModel=()=>MODEL; global.nodeFlow=()=>FLOW;
global.atr=()=>0.5; global.deflTriggerState=()=>TRIG; global.killActive=()=>false; global.featStatsCached=()=>({byKey:{}});

// 1. active down leg → the magnet
LEG={dir:'dn',magnet:{k:766,isKing:true},pbDetected:{k:769},legId:1}; DIR={trendState:'dn',capped:null,inputs:{}}; MODEL={levels:[],kingK:766}; FLOW={ok:true,lean:'none',nodes:[]};
let ns=nextStopPick('SPY');
ok('1a a down leg → Next Stop is the magnet, dir −1', ns.ok && ns.level===766 && ns.rule==='leg.magnet' && ns.dir===-1, ns);
ok('1b leg + SMA agree, within reach → grade B', ns.grade==='B', ns.grade);
TRIG='✓↓'; ns=nextStopPick('SPY');
ok('1c with the ✓ latched on the pullback node the rule reads leg.pbTarget (same level)', ns.rule==='leg.pbTarget' && ns.level===766, ns.rule);
TRIG='';
DIR={trendState:'flat',capped:'chop',inputs:{}}; ns=nextStopPick('SPY');
ok('1d chop cap → grade C even with a leg', ns.grade==='C', ns.grade);
// 2. no leg, Map lean → the accumulating node that way
LEG={dir:'none'}; DIR={trendState:'flat',capped:null,inputs:{}}; FLOW={ok:true,lean:'up',nodes:[{k:770,side:'above',state:'acm',pct:45},{k:772,side:'above',state:'acm',pct:60},{k:766,side:'below',state:'hold',pct:100}]};
ns=nextStopPick('SPY');
ok('2a Map lean up → the nearest accumulating node above (770), rule map.lean, grade C (structure-only)', ns.level===770 && ns.rule==='map.lean' && ns.dir===1 && ns.grade==='C', ns);
// 3. nothing → wall on the King's side
FLOW={ok:true,lean:'none',nodes:[]}; MODEL={levels:[{k:770,isCeil:true},{k:766,isFlr:true},{k:764,isKing:true}],kingK:764};
ns=nextStopPick('SPY');
ok('3a no leg, no lean → the next wall on the King\'s side (766, King below)', ns.level===766 && ns.rule==='wall.king' && ns.dir===-1, ns);
// 4. King is price → nearer wall
MODEL={levels:[{k:770,isCeil:true},{k:767,isFlr:true}],kingK:768.3};
ns=nextStopPick('SPY');
ok('4a price on the King → the nearer wall (767)', ns.level===767 && ns.rule==='wall.near', ns);
// 5. nothing at all → no line (never invents)
MODEL={levels:[],kingK:null}; ns=nextStopPick('SPY');
ok('5a with no nodes at all there is NO Next Stop (nothing invented)', ns.ok===false && nextStopHtml('SPY')==='');
// 6. header line
LEG={dir:'dn',magnet:{k:766,isKing:true},pbDetected:{k:769},legId:1}; DIR={trendState:'dn',capped:null,inputs:{}}; MODEL={levels:[],kingK:766};
const h=nextStopHtml('SPY');
ok('6a the header reads "Next Stop: ↓ <level> −pts · 30–60m" + grade at the right, with a question-first hover', /Next Stop:/.test(h) && /↓ 766/.test(h) && /−2\.3 pts/.test(h) && /30–60m/.test(h) && /margin-left:auto[^>]*>B</.test(h) && /Why this level\?/.test(h));
ok('6a2 the level is RED when below price (green when above)', /color:#f0616d">↓ 766/.test(h));
ok('6b the hover shows the measured hit-rate only with n (dashes before)', /— \(eff n 0, need 20\)/.test(h));
ok('6c descriptive, never an instruction', /never an instruction/.test(h) && !/\b(buy|sell|enter|go long|go short)\b/i.test(h.replace(/title="[^"]*"/,'')));
ok('6d rendered ABOVE the read', /nextStopHtml\(__asym\)[\s\S]{0,200}readBlock44\(__asym\)/.test(src));
// 7. features + outcome
ok('7a nextStop (30m) and nextStop.60 (60m) are registered, the 60m one with a 20-bar window', /registerFeature\(\{ key:'nextStop', label:[^}]*fwd:FEAT_FWD,/.test(src) && /registerFeature\(\{ key:'nextStop\.60'[^}]*fwd:FEAT_FWD\*2,/.test(src));
eval(src.slice(src.indexOf('  function _nextStopOutcome(rec, fwd){'), src.indexOf('  registerFeature({ key:\'nextStop\', label')));
let o=_nextStopOutcome({level:766,px:768.3,rule:'leg.magnet',grade:'B',dist:2.3},{mfe:0.3,mae:-2.0,first:'dn'});
ok('7b outcome: level 2.3 below, price got within 0.5 → HIT, approach ~0.87, not wrong-first', o.hit===1 && o.approach>=0.85 && o.wrongFirst===false, o);
o=_nextStopOutcome({level:766,px:768.3,rule:'leg.magnet',grade:'B',dist:2.3},{mfe:0.9,mae:-0.8,first:'up'});
ok('7c ...price went UP 0.9 first and only got 0.8 of 2.3 down → miss, approach 0.35, wrongFirst', o.hit===0 && o.approach<0.4 && o.wrongFirst===true, o);
o=_nextStopOutcome({level:770,px:768.3,rule:'map.lean',grade:'C',dist:1.7},{mfe:1.3,mae:-0.2,first:'up'});
ok('7d level above: 1.7 needed, 1.3 reached (within the 0.5 zone) → hit', o.hit===1, o);
ok('7e a record with no level is unscorable (null), never a miss', _nextStopOutcome({level:null},{mfe:1,mae:-1}).hit===null);
// 8. enrollment on the tabs + rules
ok('8a Analysis ① carries Next Stop 30m and 60m tiles', /tabTile\('Next Stop 30m'/.test(src) && /tabTile\('Next Stop 60m'/.test(src));
const RJ=JSON.parse(fs.readFileSync('./learning/rules.json','utf8'));
ok('8b rules.json seeds nextStop + nextStop.60', !!RJ.rules['nextStop'] && !!RJ.rules['nextStop.60']);
ok('8c the LLM brief asks the review to grade Next Stop by rule and propose re-ordering', /nextStop/.test(fs.readFileSync('./docs/LLM-NIGHTLY-BRIEF.md','utf8')));
console.log('test_next_stop: '+p+' passed, '+f+' failed');
