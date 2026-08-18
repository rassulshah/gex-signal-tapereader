// v10.56 PART D — THE IN-PLAY CARD, user-decided fields:
//   row 1: ● · strike · role · leg tag · TRIGGER (✓↓/✓↑/✗/blank) · GRADE
//   row 2: S✓ Q✗ V✓ · decision · tgt X · inval Y (+ take/pass) ONLY when tradeable; else "S/Q/V · skip"
//   REMOVED from the face: R:R text, %King, polarity text, tap, ▶ setup, Acm, activity, entry
//   (all in the question-first row hover). Not in contact: "watching — not in contact · nearest zone X away".
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function grab(name){ var i=src.indexOf('function '+name+'('); if(i<0) throw new Error('MISSING '+name);
  var d=0,st=false,j=i; for(;j<src.length;j++){var c=src[j]; if(c==='{'){d++;st=true;} else if(c==='}'){d--; if(st&&d===0){j++;break;}}} return src.slice(i,j); }
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};

const blk=grab('deflZonesBlock');
// isolate the in-play card: from the r1 comment to the closing of the card div
const i1=blk.indexOf('// r1'); const i2=blk.indexOf('// ---- 2. (v10.57) NODES ON WATCH');
const card=blk.slice(i1,i2);
ok('0 the in-play card block is located', i1>0 && i2>i1);
const r1=card.slice(0, card.indexOf('// r2')); const r2=card.slice(card.indexOf('// r2'));

// ---- row 1: order of fields ----
const pos=(s,re)=>{ const m=re.exec(s); return m?m.index:-1; };
const iDot=pos(r1,/●/), iK=pos(r1,/fmtLvl\(L\.k\)/), iRole=pos(r1,/zoneRole\(L\)/), iLeg=pos(r1,/legZoneTagHtml\(legR, L\)/), iTrig=pos(r1,/trigHtml/), iGrade=pos(r1,/zoneGradePill\(ng, gradeTip\)/);
ok('1a r1 has dot · strike · role · leg tag · trigger · grade in that order',
   iDot>=0 && iK>iDot && iRole>iK && iLeg>iRole && iTrig>iLeg && iGrade>iTrig, {iDot,iK,iRole,iLeg,iTrig,iGrade});
ok('1b r1 does NOT print %King on the face', !/pct\+'%/.test(r1) && !/of King/.test(r1.replace(/title="[^"]*"/g,'')));
ok('1c r1 does NOT print the polarity word / tap / Acm on the face', !/tapWord\(|acm\.m15|sharp|clean/.test(r1.replace(/title="'\+rowTip/,'')));
ok('1d the row hover is question-first and carries what left the face (%King, polarity, tap, Acm, entry, R:R, reaction now, ▶ setup)',
   /rowTip=\('What is this node/.test(blk) && /of King/.test(blk) && /sharp/.test(blk) && /tap'/.test(blk) && /Acm 15m/.test(blk) && /entry '\+fmtLvl\(L\.k\)/.test(blk) && /rrText\(rr\)/.test(blk) && /reaction now/.test(blk) && /▶ setup/.test(blk));

// ---- row 2 ----
ok('2a r2 leads with S/Q/V confluence (zoneConfHtml)', /zoneConfHtml\(ng\.inputs\.conf\)/.test(r2) && r2.indexOf('zoneConfHtml')<r2.indexOf('tradeable'));
ok('2b the frame (tgt · inval) and the buttons render ONLY when tradeable', /\(tradeable\s*\?\s*\(/.test(r2) && /var btns = tradeable \?/.test(blk));
ok('2c on skip the row says just "skip"', />skip<\/span>/.test(r2));
ok('2d not in contact -> "watching — not in contact · nearest zone X away"', /watching \\u2014 not in contact/.test(r2) && /nearest zone '\+fmtSpan\(Math\.abs\(px-L\.k\)\)\+' away/.test(r2));
ok('2e R:R text is NOT on the face (rrText only inside the hover strings)', !/rrText\(/.test(card.replace(/title="[^"]*"/g,'').replace(/decTip|rowTip/g,'')));
ok('2f the R:R gate still applies silently (thin => not tradeable)', /var thin=\(rr!=null && rr<RR_MIN\)/.test(blk) && /tradeable = inContact && !thin/.test(blk));
ok('2g tgt/inval come from the frame (fr.tgt / fr.inval) with the (air) mark', /'tgt '\+fmtLvl\(fr\.tgt\)\+\(tgtAir\?'\(air\)':''\)/.test(blk) && /'inval '/.test(blk));

// ---- trigger: latched, direction glyph, hover, colours ----
global.PAL={longAccent:'#2ec27e',shortAccent:'#f0616d'}; global.TRIG_TIP='Has the deflection occurred? ✓ = rejection candle closed away from the node; ✗ = closed through it. Latched on bar close — does not flicker.';
global.window={};
eval(grab('deflTriggerHtml'));
ok('3a blank while unresolved', deflTriggerHtml({state:''})==='' && deflTriggerHtml(null)==='');
ok('3b ✓↓ renders green with the direction glyph', /✓↓/.test(deflTriggerHtml({state:'✓↓'})) && /#2ec27e/.test(deflTriggerHtml({state:'✓↓'})));
ok('3c ✓↑ renders green', /✓↑/.test(deflTriggerHtml({state:'✓↑'})) && /#2ec27e/.test(deflTriggerHtml({state:'✓↑'})));
ok('3d ✗ renders red', /✗/.test(deflTriggerHtml({state:'✗'})) && /#f0616d/.test(deflTriggerHtml({state:'✗'})));
ok('3e hover answers "Has the deflection occurred?" and says it does not flicker', /Has the deflection occurred/.test(deflTriggerHtml({state:'✗'})) && /does not flicker/.test(deflTriggerHtml({state:'✗'})));
ok('3f the card reads the LATCH (deflTrigger), not the live reactionQuality, for the mark', /trigHtml=deflTriggerHtml\(trig\)/.test(blk) && /var trig=.*deflTrigger\(/.test(blk));
ok('3g reactionQuality survives only as the "reaction now" hover input', /reaction now: '\+rq\.q/.test(blk));

// ---- enrollment ----
ok('4a defl.trigger is a registered FEATURE', /registerFeature\(\{\s*key:'defl\.trigger'/.test(src));
ok('4b rules.json seeds defl.trigger', /"id":\s*"defl\.trigger"/.test(fs.readFileSync('./learning/rules.json','utf8')));
ok('4c LLM brief evaluates defl.trigger as the deflection hit-rate', /defl\.trigger[\s\S]{0,300}hit-rate/i.test(fs.readFileSync('./docs/LLM-NIGHTLY-BRIEF.md','utf8')));
console.log('test_inplay_card: '+p+' passed, '+f+' failed');
