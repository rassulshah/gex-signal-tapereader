// (v11.3) PB ENTRY — the level to watch for the pullback/deflection toward the Next Stop. Picker order, grade,
// face line, and the SEQUENCED outcome (touch first, then deflect DIR_PTS before a close through).
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
function v(n){ return src.match(new RegExp('var '+n+'\\s*=[\\s\\S]*?;\\n'))[0]; }
global.window={__gptsDebug:{}}; global.PB_REACH=5; global.PB_MIN_PCT=20; global.DEFLECT_ZONE=0.5; global.DIR_PTS=0.5; global.RULE_UNLOCK_N=20; global.FEAT_FWD=10;
global.PAL={ink:'#fff',sub:'#999',longAccent:'#2ec27e',shortAccent:'#f0616d',blue:'#4a90d9',amber:'#f2b45a'};
global.fmtLvl=(x)=>String(x); global.fmtSpan=(x)=>String(x); global.effN=(n)=>Math.floor(n/10); global.featStatsCached=()=>({byKey:{}});
eval(v('PBENTRY_RULES')); eval(['pbEntryPick','pbEntryHtml','_pbEntryRecord','_pbEntryOutcome'].map(ex).join('\n'));
global.STATE={SPY:{price:7712}};
let LEG, DIR, MAP, FLOW, NS, TRIG;
global.legEngine=()=>LEG; global.directionGrade=()=>DIR; global.nodeMapModel=()=>MAP; global.nodeFlow=()=>FLOW; global.nextStopPick=()=>NS; global.deflTriggerState=()=>TRIG;
function reset(){ LEG={dir:'none'}; DIR={trendState:'dn',capped:null,inputs:{}}; MAP={ok:true,levels:[{k:7716,pos:true,isCeil:true},{k:7710,pos:false,isFlr:true},{k:7720,pos:true,isStrongMag:true}]}; FLOW={ok:true,lean:'dn',nodes:[{k:7716,state:'acm',pct:60},{k:7710,state:'hold',pct:80},{k:7720,state:'dec',pct:40}]}; NS={ok:true,level:7710,dir:-1}; TRIG=''; }

// ---- 1. picker order
reset(); LEG={dir:'dn',legId:3,pbDetected:{k:7716,step:2},predictedPB:false,pbZone:null,lastPB:{k:7716}};
let p=pbEntryPick('SPY');
ok(p.ok && p.level===7716 && p.rule==='leg.pb' && p.dir===-1 && p.state==='acm' && p.pol==='+', '1a dn leg with a detected PB → PB Entry = that node, deflection DOWN, state acm, +γ', p);
ok(p.grade==='B' && p.nextStop===7710 && p.dist===4, '1b grade B (leg + Acm + SMA dn agrees + within reach); Next Stop carried (7710)', [p.grade,p.nextStop,p.dist]);
TRIG='✗'; p=pbEntryPick('SPY');
ok(p.rule!=='leg.pb' && p.rule!=='leg.lastPB', '1c a ✗-latched PB (broken) is skipped — and not re-offered as the last PB either', p.rule);
reset(); TRIG='✓↓'; LEG={dir:'dn',legId:3,pbDetected:{k:7716,step:2}}; p=pbEntryPick('SPY');
ok(p.rule==='leg.pb' && p.latched==='✓↓', '1d a ✓ latch is carried on the pick (deflection under way)', p.latched);
reset(); LEG={dir:'dn',legId:4,pbDetected:null,predictedPB:true,pbZone:{lo:7712,hi:7716.5},lastPB:null}; p=pbEntryPick('SPY');
ok(p.rule==='leg.pbZone' && p.level===7716.5 && p.zoneLo===7712 && p.zoneHi===7716.5, '1e no node yet but a predicted zone → the far edge of the zone, zone carried (forming)', p);
reset(); LEG={dir:'dn',legId:4,pbDetected:null,predictedPB:false,pbZone:null,lastPB:{k:7716}}; p=pbEntryPick('SPY');
ok(p.rule==='leg.lastPB' && p.level===7716, '1f else the leg’s last PB above price (dn leg)', p.rule);
reset(); LEG={dir:'up',legId:5,pbDetected:null,predictedPB:false,pbZone:null,lastPB:{k:7716}}; p=pbEntryPick('SPY');
ok(p.rule!=='leg.lastPB', '1g an up-leg last PB ABOVE price is not a pullback level (must be below)', p.rule);
reset(); p=pbEntryPick('SPY');
ok(p.rule==='map.pb' && p.level===7716 && p.dir===-1, '1h no leg: Next Stop is below (7710) → the accumulating node ABOVE price (7716) is the pullback candidate, deflection down', p);
reset(); FLOW.nodes=[{k:7716,state:'dec',pct:60}]; p=pbEntryPick('SPY');
ok(p.rule==='wall.opp' && p.level===7716, '1i no acm node on that side → the nearer wall/gate on that side', p.rule);
reset(); FLOW.nodes=[]; MAP.levels=[]; p=pbEntryPick('SPY');
ok(!p.ok, '1j nothing qualifying → no line (never invents a level)');
reset(); LEG={dir:'dn',legId:3,pbDetected:{k:7716,step:2}}; DIR={trendState:'dn',capped:'chop'}; p=pbEntryPick('SPY');
ok(p.grade==='C', '1k chop caps the grade to C', p.grade);
reset(); LEG={dir:'dn',legId:3,pbDetected:{k:7716,step:2}}; FLOW.nodes[0].state='dec'; p=pbEntryPick('SPY');
ok(p.grade==='C' && p.state==='dec', '1l a Dec pullback node is never B', [p.grade,p.state]);

// ---- 2. face line
reset(); LEG={dir:'dn',legId:3,pbDetected:{k:7716,step:2}};
const h=pbEntryHtml('SPY');
ok(/PB Entry:/.test(h) && /↑ 7716/.test(h) && /\+4 pts/.test(h) && /Acm/.test(h) && /defl ↓ → 7710/.test(h) && />B</.test(h) && /white-space:normal/.test(h), '2a two lines: level line tight, gray context on a wrapping second line', null);
ok(/title="PB Entry — where to look for the pullback/.test(h) && /never an instruction/.test(h) && /eff n 0, need 20/.test(h), '2b hover: why this level, how it is scored, no % without n, descriptive', null);
ok(!/\bbuy\b|\bsell\b|stop loss|position|\bR:R\b/i.test(h), '2c no trade language on the face or hover');

// ---- 3. record shape
reset(); LEG={dir:'dn',legId:3,pbDetected:{k:7716,step:2}};
const rec=_pbEntryRecord('SPY',{});
ok(rec.level===7716 && rec.dir===-1 && rec.grade==='B' && rec.state==='acm' && rec.pol==='+' && rec.nextStop===7710 && rec.px===7712 && rec.inval===7716.5 && rec.tgt===7710, '3a record: level/dir/grade/state/pol/nextStop/px + tgt/inval for the frame', rec);

// ---- 4. SEQUENCED outcome on the bar path
function fwd(path){ let mfe=0,mae=0; path.forEach(b=>{ mfe=Math.max(mfe,b.h-7712); mae=Math.min(mae,b.l-7712); }); return {mfe,mae,path,first:null}; }
let o=_pbEntryOutcome(rec, fwd([{h:7713,l:7711,c:7712},{h:7715,l:7712,c:7714},{h:7716.2,l:7714,c:7715},{h:7715.5,l:7714.8,c:7715},{h:7715,l:7713,c:7713.5}]));
ok(o.hit===1 && o.touched && !o.broke && o.touchBar===2 && o.deflectedPts>=0.5, '4a touched the level (bar 3) then moved 0.5+ away before any close through → HIT', o);
o=_pbEntryOutcome(rec, fwd([{h:7713,l:7711,c:7712},{h:7715,l:7712,c:7714},{h:7716.2,l:7714,c:7715},{h:7717.5,l:7715.5,c:7717.2},{h:7719,l:7717,c:7718.5}]));
ok(o.hit===0 && o.touched && o.broke, '4b touched then CLOSED through by more than the zone before deflecting → MISS (broke)', o);
o=_pbEntryOutcome(rec, fwd([{h:7713,l:7711,c:7712},{h:7714,l:7711,c:7713},{h:7714.5,l:7712,c:7713.5}]));
ok(o.hit===null && !o.touched && o.approach>0 && o.approach<1, '4c never touched → hit null (not a miss), approach recorded', o);
o=_pbEntryOutcome(rec, fwd([{h:7716.8,l:7715.6,c:7716.4},{h:7717,l:7716.3,c:7716.4},{h:7716.9,l:7715.9,c:7716.0},{h:7716.2,l:7714.9,c:7715.2}]));
ok(o.hit===1 && o.touchBar===0 && !o.broke, '4d a wicky touch that overshoots inside the zone (no close beyond +0.5) and then deflects → HIT (−γ style)', o);
const recUp={level:7710,dir:1,px:7712,rule:'leg.pb',grade:'B'};
o=_pbEntryOutcome(recUp, fwd([{h:7712,l:7710.3,c:7711},{h:7711.5,l:7710.2,c:7711.2},{h:7712.4,l:7711,c:7712.2}]));
ok(o.hit===1 && o.touched, '4e mirror: up-deflection off a floor at 7710 → HIT', o);

// ---- 5. (v11.3.1) pullback context on the record: depth, SMA-50 frame, node life
eval(['_pbCtx'].map(ex).join('\n'));
global.ledgerNode=(sym,k)=>({ firstT: Math.floor(Date.now()/1000)-20*60, m15: 9, fromPeak: 4 });
// dn leg pulled back up: 30 candles — down from 7720 to 7708 (low at bar 20), then a slow grind up to 7712
let cs=[]; for(let i=0;i<=20;i++){ const c=7720-0.6*i; cs.push({o:c+0.3,h:c+0.5,l:c-0.3,c:c}); }
for(let i=1;i<=9;i++){ const c=7708+0.45*i; cs.push({o:c-0.35,h:c+0.2,l:c-0.5,c:c}); }
global.closedCandles=()=>cs; global.atr=()=>0.6; STATE.SPY.price=7712;
reset(); LEG={dir:'dn',legId:3,pbDetected:{k:7716,step:2,rolledFrom:7718}};
const rc=_pbEntryRecord('SPY',{});
ok(rc.pb && Math.abs(rc.pb.depth-0.35)<0.06 && rc.pb.bars===9 && rc.pb.paceAtr<1, '5a depth ≈ retrace/leg (≈0.35, shallow), 9 pullback bars, slow pace in ATRs', rc.pb);
ok(rc.pb && rc.pb.counterShare>=0.9, '5b the pullback bars close WITH the pullback (grind up, counterShare ~1)', rc.pb&&rc.pb.counterShare);
ok(rc.sma && rc.sma.nodeSide && typeof rc.sma.d==='number' && typeof rc.sma.dAtr==='number' && rc.sma.levelD>rc.sma.d, '5c SMA-50 frame: price-to-SMA and level-to-SMA (signed, ATR units), node side named', rc.sma);
ok(rc.nodeCtx && rc.nodeCtx.age===20 && rc.nodeCtx.fresh===true && rc.nodeCtx.m15===9 && rc.nodeCtx.rolledFrom===7718 && rc.nodeCtx.step===2, '5d node life: fresh (20m old — dealers stepping in), building (m15 +9), rolled from 7718, step 2', rc.nodeCtx);
global.ledgerNode=()=>({ firstT: Math.floor(Date.now()/1000)-300*60, m15:-12, fromPeak:30 });
const rc2=_pbEntryRecord('SPY',{});
ok(rc2.nodeCtx && rc2.nodeCtx.fresh===false && rc2.nodeCtx.m15===-12, '5e an old, bleeding node reads fresh:false, m15 −12', rc2.nodeCtx);
// ---- 6. (v11.3.1) STACKED pullback nodes recorded: strongest / deepest / chosen rank / span
reset(); LEG={dir:'dn',legId:3,pbDetected:{k:7716,step:2,rolledFrom:7718}};
FLOW={ok:true,lean:'dn',nodes:[{k:7716,state:'acm',pct:60,m15:9},{k:7716.5,state:'acm',pct:85,m15:4},{k:7717.5,state:'hold',pct:35,m15:0},{k:7710,state:'hold',pct:80}]};
const rc3=_pbEntryRecord('SPY',{});
ok(rc3.stack && rc3.stack.n===3 && rc3.stack.strongest===7716.5 && rc3.stack.chosenRank===2, '6a a 3-node stack around the pick: strongest named (7716.5 at 85%), picked node ranked', rc3.stack);
ok(rc3.stack && rc3.stack.deepest===7717.5 && rc3.stack.span===1.5, '6b deepest (most retracement, 7717.5) and the stack span named', rc3.stack&&[rc3.stack.deepest,rc3.stack.span]);
ok(rc3.stack.members.length===3 && rc3.stack.members[0].pct===85 && typeof rc3.stack.members[0].depth==='number', '6c members carry pct/state/m15/depth, strongest first', rc3.stack.members[0]);
FLOW={ok:true,lean:'dn',nodes:[{k:7716,state:'acm',pct:60}]};
const rc4=_pbEntryRecord('SPY',{});
ok(rc4.stack==null, '6d a lone node records no stack');
console.log('test_pb_entry: '+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
