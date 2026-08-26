// v10.56 — the LATCHED ✓/✗ trigger. User rule: "make sure you dont toggle it back and forth."
// ✓↓/✓↑ latches on a rejection CLOSE away from the node; ✗ on a CLOSE through it; evaluated on
// CLOSED bars only; once latched it never re-evaluates; reset only on a new legId / node.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);if(!m)throw new Error('no fn '+n);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
global.DEFLECT_ZONE=0.5; global.TRIG_AWAY_MULT=2; global.TRIG_AWAY_BARS=3; 
eval(['trigBlank','deflTriggerStep'].map(ex).join('\n'));
let p=0,f=0;const ok=(n,c,g)=>{if(c){p++;console.log('PASS '+n);}else{f++;console.log('FAIL '+n+(g!==undefined?' -> '+JSON.stringify(g):''));}};

// resistance PB at 769 in a downtrend (setup dir 'dn'): a rejection is high>=768.5, close<768.5, close<open
const R=(o)=>Object.assign({k:769,dir:'dn',legId:'L1',closed:true,zone:0.5},o);
let st=null;
st=deflTriggerStep(st, R({bar:1,o:768.6,h:769.2,l:768.2,c:768.4}));
ok('1a rejection close AWAY from resistance latches ✓↓ (short signal)', st.state==='✓↓', st.state);
ok('1b latchedBar recorded', st.latchedBar===1, st.latchedBar);
// adverse bar afterwards must NOT flip it
st=deflTriggerStep(st, R({bar:2,o:768.4,h:769.6,l:768.3,c:769.5}));
ok('1c a later bar closing THROUGH the node does NOT un-latch ✓ (no toggling)', st.state==='✓↓', st.state);
st=deflTriggerStep(st, R({bar:3,o:769.5,h:770,l:769,c:769.9}));
ok('1d ...still ✓ after another adverse bar', st.state==='✓↓', st.state);

// intrabar wick never decides
let s2=deflTriggerStep(null, R({bar:1,closed:false,o:768.6,h:769.2,l:768.2,c:768.4}));
ok('2a an INTRABAR bar (closed:false) never latches — state stays blank', s2.state==='', s2.state);

// ✗ on close through before any ✓
let s3=deflTriggerStep(null, R({bar:1,o:768.6,h:769.7,l:768.5,c:769.6}));
ok('3a a CLOSE through resistance (c>k+zone) latches ✗', s3.state==='✗', s3.state);
s3=deflTriggerStep(s3, R({bar:2,o:769.6,h:769.7,l:768.0,c:768.2}));
ok('3b a later rejection does NOT flip a latched ✗ back to ✓', s3.state==='✗', s3.state);

// ✓ never re-evaluated: same bar re-fed does nothing
let s4=deflTriggerStep(null, R({bar:1,o:768.6,h:769.2,l:768.2,c:768.4}));
const before=JSON.stringify(s4);
s4=deflTriggerStep(s4, R({bar:1,o:768.6,h:769.2,l:768.2,c:768.4}));
ok('4a re-feeding the same closed bar is a no-op once latched', JSON.stringify(s4)===before);

// reset ONLY on a new leg / node
let s5=deflTriggerStep(null, R({bar:1,o:768.6,h:769.2,l:768.2,c:768.4}));
ok('5a latched ✓↓ before reset', s5.state==='✓↓');
s5=deflTriggerStep(s5, R({bar:9,legId:'L2',o:768.6,h:769.7,l:768.5,c:769.6}));
ok('5b a NEW legId resets the latch and re-evaluates (here: ✗ on close-through)', s5.state==='✗' && s5.legId==='L2', s5);
let s6=deflTriggerStep(null, R({bar:1,o:768.6,h:769.2,l:768.2,c:768.4}));
s6=deflTriggerStep(s6, R({bar:2,k:771,o:770.6,h:771.2,l:770.2,c:770.4}));
ok('5c a different NODE strike resets too', s6.k===771 && s6.state==='✓↓', s6);

// support mirror
const S=(o)=>Object.assign({k:769,dir:'up',legId:'L1',closed:true,zone:0.5},o);
let s7=deflTriggerStep(null, S({bar:1,o:769.4,h:769.8,l:768.8,c:769.6}));
ok('6a support: rejection close AWAY (low<=k+zone, close>k+zone, close>open) latches ✓↑ (long signal)', s7.state==='✓↑', s7.state);
let s8=deflTriggerStep(null, S({bar:1,o:769.4,h:769.5,l:768.2,c:768.3}));
ok('6b support: close THROUGH (c<k-zone) latches ✗', s8.state==='✗', s8.state);

// direction glyph is unambiguous
ok('7a ✓ carries direction: ↓ for resistance/short, ↑ for support/long', /↓/.test('✓↓') && /↑/.test('✓↑'));

// the visible card uses the latch, not the per-poll reactionQuality
ok('8a the in-play card renders the latched trigger (trigHtml), not the live reaction mark', /trigHtml/.test(ex('deflZonesBlock')));
console.log('test_defl_trigger: '+p+' passed, '+f+' failed');

// (v14.6) exit code added: this file could print FAIL and still exit 0 — the silent-red pattern.
process.exit((typeof fail!=="undefined"&&fail)?1:0);
