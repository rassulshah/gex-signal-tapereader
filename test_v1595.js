// test_v1595.js — (v15.95) THE SETTINGS PANEL, TRIMMED.
//
// Operator, 2026-09-10: "there are a lot of things on the settings that are old and not in use. review what needs to be
// removed and its associated code in the javascript file" — after he could not scroll the ⚙ panel to its foot. The review
// traced every control to whether anything reads it. The inert ones are cut WITH their code; the live ones stay. This test
// pins BOTH sides: what must be gone (control + handler + CFG plumbing) and what must remain (Node Thresh, Trend, the six
// alerts that actually fire). A future context must not "restore" a dead toggle, and must not delete a live one.
const fs=require('fs'); const src=fs.readFileSync('./v10.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };

ok(/@version\s+15\.9[5-9]/.test(src) && /var GPTS_VERSION='15\.9[5-9]';/.test(src), '0a v15.95 or later in both spots');

// ---------- 1. the legacy BO-pullback signal block is GONE (control + handler + CFG) ----------
{
  ok(!/gpts-bopb/.test(src), '1a the BO Pullback toggle (gpts-bopb) is removed');
  ok(!/gpts-ftreq/.test(src), '1b BO Followthrough Req (gpts-ftreq) is removed');
  ok(!/gpts-seg\b/.test(src) && !/function segBtn/.test(src), '1c Signal Type (gpts-seg) and its segBtn() builder are removed');
  ok(!/CFG\.boPb\s*=/.test(src) && !/\bo\.boPb\b/.test(src), '1d CFG.boPb — the flag nothing consumed — is gone (no assignment, no load)');
  ok(!/CFG\.ftReq\s*=/.test(src) && !/\bo\.ftReq\b/.test(src), '1e CFG.ftReq is gone (no assignment, no load)');
  ok(!/CFG\.dir=/.test(src) && !/o\.dir==='both'/.test(src), '1f CFG.dir (the Signal Type value) is gone — no assignment, no load (an unrelated o.dir===\'dn\' elsewhere is a different object)');
  ok(!/>BO Pullback</.test(src) && !/Signal Type</.test(src) && !/BO Node Thresh/.test(src), '1g no "BO Pullback" / "Signal Type" / "BO Node Thresh" rendered label survives on the face');
}

// ---------- 2. Compact node cells is GONE (superseded by the v15.63 ladder-grid) ----------
{
  ok(!/gpts-compact/.test(src), '2a Compact node cells (gpts-compact) is removed');
  ok(!/CFG\.compact\s*=/.test(src) && !/\bo\.compact\b/.test(src), '2b CFG.compact — no consumer — is gone (no assignment, no load)');
}

// ---------- 3. the pbNode alert row is GONE; the six that FIRE stay ----------
{
  ok(!/alertRow\('pbNode'/.test(src) && !/pbNode:\s*\{/.test(src), '3a the pbNode alert row and its default are removed (it never fired)');
  ok((src.match(/html\+=alertRow\(/g)||[]).length===6, '3b exactly six alert rows render — pbNode dropped, the six live ones kept', (src.match(/html\+=alertRow\(/g)||[]).length);
  ['feedStale','kingRoll','inplayAccum','dissipate','absorption','trap'].forEach(function(ev){
    ok(new RegExp("fireAlert\\('"+ev+"'").test(src) && new RegExp("alertRow\\('"+ev+"'").test(src), '3c the live alert '+ev+' keeps both its row and its trigger');
  });
}

// ---------- 4. the live controls that LOOKED legacy are KEPT (each has a real consumer) ----------
{
  ok(/gpts-nt\b/.test(src) && /1\. Node Thresh \(% King\)/.test(src) && /MIN_STRENGTH=v/.test(src), '4a Node Thresh stays (renamed, still drives MIN_STRENGTH)');
  ok(/gpts-trend\b/.test(src) && /2\. Trend/.test(src) && /if\(!CFG\.trendOn\) return true;/.test(src), '4b Trend stays (renumbered 2, still read by the trend engine)');
  ok(/gpts-trend-ma/.test(src) && /CFG\.trendMA/.test(src), '4c Trend MA stays');
  ok(/>Tapereader config</.test(src) && !/>BO Pullback Config</.test(src), '4d the stale header "BO Pullback Config" is renamed to "Tapereader config"');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
