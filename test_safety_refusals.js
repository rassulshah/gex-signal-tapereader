// (v11.94) REFUSALS — a reading that did not happen must never render as a reading that did.
//
// Every case here was found by audit on 2026-08-24 and every one was live. The shared shape: a
// function returns its NEUTRAL value for "no data", "not applicable" and "it threw", and the renderer
// turns that neutral into a confident sentence. The project already had this at emPiles (D-6) and
// kingRoll (v11.88); these are the instances that survived.
//
// ⚠ ALL FOUR FIXES FIRED ZERO ASSERTIONS in the first mutation sweep. That is why this file exists.
const fs=require('fs');
const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0;
const ok=(m,c,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
const eq=(m,a,b)=>ok(m,a===b,{got:a,want:b});
function ex(n){
  const re=new RegExp('function\\s+'+n+'\\s*\\(','g'); const m=re.exec(src);
  if(!m) throw new Error('not found: '+n);
  let i=src.indexOf('{',m.index), d=0;
  for(let k=i;k<src.length;k++){ if(src[k]==='{')d++; else if(src[k]==='}'){ d--; if(d===0) return src.slice(m.index,k+1); } }
  throw new Error('unbalanced: '+n);
}

// ---------- 1. A TRUNCATED CONFIRM TALLY MUST NOT RENDER AS A COMPLETE ONE ----------
// The catch wrapped the WHOLE assembly and `out` was returned regardless. A throw after SKEW and
// ACCUM left confirms.length===2, and confColour paints green when nConf===nLive — so the face could
// read "2 of 2 confirm" IN GREEN off half the evidence. Maximum displayed conviction, truncated input.
{
  global.window={};
  let TV={state:'dn'}, SK={dir:-1,err:null}, AC={dir:-1};
  global.trendVerdict=()=>TV;
  global.trendWindowRead=()=>({win:20, up:4});
  global.skewRead=()=>SK; global.accumAsym=()=>AC;
  global.paRead=()=>({ok:true,dir:-1});
  global.crossRead=()=>({ok:true,dir:-1});
  global.kingRollRead=()=>({ok:true,dir:-1});
  global.driftRead=()=>({verdict:'AGREE-DN',dir:-1,overlap:true,label:'x'});
  eval(ex('biasVotes'));
  const good=biasVotes('SPY');
  eq('1a a complete assembly counts four', good.confirms.length, 4);
  ok('1b and is marked complete', good.complete===true);
  ok('1c with no break recorded', !good.broke);

  // now make the THIRD confirm THROW. Its own try/catch degrades it to null — which is correct, one
  // broken input must not take the tally down — but a read that THREW and one that merely had nothing
  // to say both rendered as the same grey dash, so a permanently-crashing confirm was indistinguishable
  // from a quiet one and could sit there for weeks.
  global.crossRead=()=>{ throw new Error('cross exploded'); };
  const bad=biasVotes('SPY');
  ok('1d the throw is RECORDED against that read', /cross exploded/.test((bad.errs||{}).CROSS||''), bad.errs);
  eq('1e the other three still count — one broken input does not take the tally down', bad.nConf, 3);
  eq('1f CROSS itself abstains rather than voting', bad.confirms.filter(c=>c.k==='CROSS')[0].d, null);
  ok('1g and the assembly still completed', bad.complete===true);
  // (v15.53) 1h/1i pinned secBias's chip, archived (C-flagged-off)
  // the STRUCTURAL backstop still zeroes a genuinely truncated assembly
  ok('1j a truncated assembly is still zeroed rather than rendered short',
     /if\(!out\.complete\)\{ out\.nConf=0; out\.nLive=0; out\.confirms=\[\]; \}/.test(ex('biasVotes')));
  // (v15.53) the renderer assertions pinned secBias, archived
}

// ---------- 2. 'hold' IS A MEASUREMENT ----------
{
  global.nodeFlow=()=>({ ok:true, nodes:[{k:765,state:'acm'}] });
  eval(ex('mapStateOf'));
  eq('2a a strike present returns its real state', mapStateOf('SPY',{k:765}), 'acm');
  eq('2b a strike ABSENT from the set is UNKNOWN, not holding', mapStateOf('SPY',{k:999}), null);
  global.nodeFlow=()=>({ ok:false });
  eq('2c an unavailable node-flow read is UNKNOWN', mapStateOf('SPY',{k:765}), null);
  global.nodeFlow=()=>{ throw new Error('boom'); };
  eq('2d and a THROWN error is UNKNOWN — it was returning "hold" for all three', mapStateOf('SPY',{k:765}), null);
}

// ---------- 3. THE OTHER HALF: THE CHIP ----------
// mapStateOf returning null is worth nothing if the renderer paints null as "holding". Neither fix
// works alone, which is exactly why the first mutation sweep caught neither.
{
  global.MAP_ACM=8; global.MAP_DEC=-8; global.MAP_DROP=25;   // ⚠ MAP_DROP is only reached on the DISSIPATING branch — without it the mutant CRASHED, and a crash counts as zero FAIL lines, so the mutation read as "caught nothing"
  global.PAL={ sub:'#8b98a9', longAccent:'#2ec27e', shortAccent:'#f0616d' };
  eval(ex('mapChipHtml'));
  const unk=mapChipHtml(null), held=mapChipHtml('hold'), acm=mapChipHtml('acm');
  // ⚠ check the VISIBLE LABEL, not "an em dash appears somewhere". Without the guard the null falls
  // through to the LAST branch and renders "DISSIPATING" — whose tip contains an em dash, so a loose
  // /—/ test passed on a chip making a STRONGER false claim than the one being fixed.
  const label=(h)=>{ const m=/>([^<]*)<\/span>\s*$/.exec(h); return m?m[1]:''; };
  eq('3a an UNKNOWN state renders a dash as its LABEL', label(unk), '&mdash;');
  // ⚠ test the LABEL, not the whole markup: the UNKNOWN tip deliberately says "not the same as
  // holding", so scanning the markup for state words fails on the correct implementation.
  // Without the guard this label reads "dec" and the tip claims DISSIPATING — a STRONGER false claim
  // than the "holding" that started this.
  ok('3b and the label is never a state word — dec/acm/gone/holding',
     !/^(dec|acm|gone|hold|holding)$/.test(label(unk)), label(unk));
  ok('3c the tip states it is UNKNOWN', /UNKNOWN/.test(unk));
  ok('3d nor the measurement sentence it has no basis for', !/no 15m change beyond/.test(unk));
  ok('3e it says plainly that it is not the same as holding', /not the same as holding/.test(unk));
  eq('3f a genuine hold still LABELS holding', label(held), 'holding');
  ok('3g with its measurement sentence', /no 15m change beyond/.test(held));
  ok('3h and a real state is unaffected', /ACCUMULATING/.test(acm));
}

// ---------- 4. A HALF-BUILT FUTURES SCALE IS THE MOST EXPENSIVE FAILURE HERE ----------
// Defaults are ok:true, r:1, and out.fam is assigned BEFORE the ratio. A throw in between left
// {fam:'ES', ok:true, r:1} — dispIsFut() true, dispR() 1, futMark() empty — so every level rendered
// as a SPY-scale number wearing an ES label. Off by ~10x with no approximation marker.
{
  global.FUTCFG={ mode:'auto' };
  global.FUT_UNDERLYING={ ES:'SPY' }; global.FUT_FAMILY={ ES:'ES' };
  global.FUTR={};
  global.futDetect=()=>({ sym:'ES', px:7660, src:'chart' });
  global.feedUnderlyingPx=()=>763;
  global.futRatioStep=()=>({ r:10.04, live:true, approx:false, src:'live' });
  eval(ex('futModeCompute'));
  const good=futModeCompute();
  eq('4a a clean build carries the real ratio', good.r, 10.04);
  eq('4b and the family', good.fam, 'ES');
  ok('4c marked built', good.built===true);

  global.futRatioStep=()=>{ throw new Error('ratio exploded'); };
  const bad=futModeCompute();
  ok('4d a throw AFTER fam is set is recorded', !!bad.err, bad.err);
  eq('4e the family is CLEARED — fam:"ES" with r:1 is what made SPY numbers wear an ES label', bad.fam, null);
  eq('4f ok is false', bad.ok, false);
  eq('4g the ratio is an honest 1', bad.r, 1);
  ok('4h and it says the scale is unavailable', /futures scale unavailable/.test(bad.msg||''), bad.msg);
  ok('4i never silently approximate', bad.approx===false);
}

// ---------- 5. GATEKEEPER: ONE BRANCH MEANS CLEAR, THE REST ARE REFUSALS ----------
// Both callers collapsed every early return to gkK=null and rendered "No gatekeeper — clear path to
// the King." Five different refusals and one genuine all-clear, rendered identically. D-6 again.
{
  const gk=ex('gatekeeper');
  ok('5a a genuine no-blocker sets clear:true', /out\.clear=true/.test(gk));
  const whys=(gk.match(/out\.why=/g)||[]).length;
  ok('5b and every refusal carries its own reason', whys>=4, whys);
  ok('5c no price / no walls says so', /no price or no wall set/.test(gk));
  ok('5d no King says so', /no King to gatekeep toward/.test(gk));
  ok('5e price AT the King is its own case', /price is AT the King/.test(gk));
  // the ONLY clear branch must be the one that actually walked the path
  const clearAt=gk.indexOf('out.clear=true');
  const betweenAt=gk.indexOf('if(!between.length)');
  ok('5f and clear is set ONLY after the intervening nodes were actually counted', clearAt>betweenAt && betweenAt>0);
}

// ---------- 6. kingRoll's ABSENT/ZERO SPLIT MUST HOLD AT EVERY CALL SITE ----------
// v11.88 fixed the vote path and MISSED two: the node-map row builder and the dir.kingRoll recorder.
{
  // ⚠ strip comments first — the v11.88 note ABOUT kingRoll() mentions it by name, and counting prose
  // as a call site is the same mistake that has now broken five assertions in this project.
  const code=src.replace(/\/\*[\s\S]*?\*\//g,'').split('\n').map(l=>l.replace(/^\s*\/\/.*$/,'')).join('\n');
  const raw=[...code.matchAll(/[^a-zA-Z]kingRoll\(/g)].length;
  const decl=(code.match(/function kingRoll\(/g)||[]).length;
  const inRead=(code.match(/return \{ ok:true, dir:kingRoll\(sym\)\|\|0 \}/g)||[]).length;
  ok('6a kingRoll is declared once', decl===1, decl);
  ok('6b and called raw ONLY from inside kingRollRead — every other site goes through the guard',
     raw === decl + inRead, {rawCalls:raw, decl:decl, insideRead:inRead});
  ok('6c the node-map row uses the guarded read', /rollR=\(role==='King'\) \? kingRollRead\(sym\)/.test(src));
  ok('6d and the recorder records null for an absent history, never a neutral vote',
     /vote:\(rr&&rr\.ok\)\?rr\.dir:null/.test(src));
}
console.log('\n'+pass+' pass / '+fail+' fail');
process.exit(fail?1:0);
