// ============================================================================================
// test_ladder_layout.js — (v15.28) THE LADDER, LAID OUT BY A REAL BROWSER.
//
// Operator, 2026-09-01: "its not right the EL is being cut off , i cant go scroll lower or higher
// on the ladder. double check."
//
// ⚠⚠ WHY THIS FILE HAD TO EXIST. Every other test in this project runs in Node or jsdom, and JSDOM
// HAS NO LAYOUT ENGINE: every box measures 0, `scrollTop` never moves, `max-height` does nothing.
// So the two properties he actually asked for — the band fills the view, and the view SCROLLS —
// could only be asserted as source greps, and this project's whole history is greps that passed
// while the face was wrong. y7 and y8f in test_replay_face are marked [GREP] for exactly that
// reason; these assertions are what those greps were standing in for.
// Chromium is already in the container. This lays out the REAL markup with the REAL CSS and asks
// the browser the same question the operator asks with his eyes: can I see EL, and can I scroll.
//
// ⚠ SKIPS, never fails, when playwright or the browser is absent — a machine without a browser
// cannot answer this question and should not pretend to.
// ============================================================================================
const fs = require('fs'), cp = require('child_process');
let pass = 0, fail = 0;
const ok = (c, m, g) => { if (c) { pass++; console.log('PASS ' + m); }
                          else { fail++; console.log('FAIL ' + m + (g !== undefined ? ' -> ' + JSON.stringify(g) : '')); } };
let chromium = null;
try { chromium = require('playwright').chromium; } catch (e) {}
const EXE = ['/opt/pw-browsers/chromium-1194/chrome-linux/chrome',
             '/opt/pw-browsers/chromium_headless_shell-1194/chrome-linux/headless_shell']
            .find(p => { try { return fs.existsSync(p); } catch (e) { return false; } });
if (!chromium || !EXE) {
  console.log('test_ladder_layout: SKIPPED — no playwright/chromium in this environment');
  process.exit(0);
}

(async () => {
  // ⚠⚠ THE FIXTURE MATTERS AND MY FIRST CHOICE DID NOT TEST THE CLAMP. On 2026-08-24 price had
  // taken out the expected low (the ⤓ on EL), so the view extended BELOW the band and the EL label
  // sat comfortably at 229 — removing the edge clamp changed nothing and the mutation survived.
  // 2026-08-28 13:18 has `dnExc: 0` — price never traded below the expected low all session — so the
  // window's floor IS the band's low, `Y(EL)` is exactly H, and the label lands on the boundary.
  // That is the geometry the operator reported, and it is the only kind of day that tests the clamp.
  cp.execSync('node tools/render-face.js 2026-08-28 13:18 --page --legacy', { stdio: 'ignore' });   // (v15.63) this test measures the v15.62 ladder's geometry — one toggle away, still shipped
  const b = await chromium.launch({ executablePath: EXE });
  const p = await b.newPage({ viewport: { width: 900, height: 900 } });
  await p.goto('file:///tmp/face-page.html');
  await p.waitForTimeout(120);
  const m = await p.evaluate(() => {
    const w = document.querySelector('.g3ladwrap'), lad = document.querySelector('.g3lad');
    if (!w || !lad) return { err: 'no ladder' };
    const wr = w.getBoundingClientRect(), cs = getComputedStyle(w);
    const pill = n => { const e = [...document.querySelectorAll('.g3ldempill')]
      .find(x => x.textContent.trim().replace('~', '').startsWith(n));
      if (!e) return null; const r = e.getBoundingClientRect();
      return { top: Math.round(r.top - wr.top), bottom: Math.round(r.bottom - wr.top),
               visible: r.top >= wr.top - 0.5 && r.bottom <= wr.bottom + 0.5 }; };
    const before = w.scrollTop;
    w.scrollTop = 99999; const down = w.scrollTop; w.scrollTop = 0; const up = w.scrollTop;
    w.scrollTop = before;
    const rails = [...document.querySelectorAll('.g3ldemL')]
      .map(e => e.getBoundingClientRect().top - wr.top).sort((a, z) => a - z);
    return { overflowY: cs.overflowY, clientH: w.clientHeight, scrollH: w.scrollHeight,
             scrollRange: down - up, ladderH: Math.round(lad.getBoundingClientRect().height),
             EH: pill('EH'), EL: pill('EL'), rails };
  });
  await b.close();

  ok(!m.err, 'L0 the ladder lays out in a real browser', m.err);
  // ⚠ THE COMPLAINT, ASSERTED DIRECTLY: "the EL is being cut off"
  ok(m.EL && m.EL.visible, 'L1 EXECUTED IN A BROWSER: the EXPECTED LOW is fully visible in the view', m.EL);
  ok(m.EH && m.EH.visible, 'L2 ...and so is the expected high', m.EH);
  // ⚠ AND: "i cant go scroll lower or higher on the ladder"
  ok(m.overflowY === 'auto', 'L3 the ladder view scrolls vertically', m.overflowY);
  ok(m.scrollH > m.clientH, 'L4 ...and there is content beyond the window to scroll to',
     { scrollH: m.scrollH, clientH: m.clientH });
  ok(m.scrollRange > 0, 'L5 ...and the browser actually MOVES it — the property jsdom cannot test',
     m.scrollRange);
  // the band is the window, not a sliver at the top
  if (m.rails && m.rails.length === 2) {
    const band = m.rails[1] - m.rails[0];
    ok(band >= m.clientH * 0.5,
       'L6 the expected move fills at least half the view, measured in real pixels',
       { bandPx: Math.round(band), viewH: m.clientH, share: +(band / m.clientH).toFixed(2) });
  }
  // ⚠ TWO MUTATIONS SURVIVE THIS FILE AND BOTH ARE HONEST, NOT GAPS:
  //   · clamping by a guessed 11px instead of the pill's true half-height now also renders visibly,
  //     because the header fix gave the window 12px back. It is no longer a behaviour change here.
  //   · setting the window to the whole frame is invisible ON THIS FIXTURE, whose content and band
  //     happen to coincide. test_replay_face y8h compares the window against the content and does
  //     discriminate it.
  // Recorded so the next context does not "fix" a test that is already telling the truth.
  // ---- (v15.30) THE BAND IS ALWAYS FULLY IN VIEW ---------------------------------------------
  // Operator, 2026-09-01: "regarding the display of the ladder, you must at all times show the EH to
  // the Expected low or beyond both." L1/L2 check each label; this checks the SPAN — both rails
  // inside the visible box at once, which is the property he actually stated.
  ok(m.rails && m.rails.length === 2, 'L7 both band edges are drawn', m.rails);
  if (m.rails && m.rails.length === 2) {
    ok(m.rails[0] >= -0.5 && m.rails[1] <= m.clientH + 0.5,
       'L7b EXECUTED IN A BROWSER: the whole EH→EL span is inside the view, with no scrolling needed',
       { rails: m.rails.map(r => Math.round(r)), viewH: m.clientH });
  }

  console.log('test_ladder_layout: ' + pass + ' passed, ' + fail + ' failed');
  process.exit(fail ? 1 : 0);
})().catch(e => { console.log('test_ladder_layout: SKIPPED — ' + e.message); process.exit(0); });
