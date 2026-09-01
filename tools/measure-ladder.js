// measure-ladder.js — LAY THE LADDER OUT IN A REAL BROWSER AND MEASURE IT.
//
// ⚠⚠ WHY THIS EXISTS. jsdom has no layout engine: every box measures 0, `scrollTop` never moves and
// `max-height` does nothing. So the two properties the operator actually asked for — "the ladder
// should be drawn from the expected move low to the expected move high" and "allowing me to scroll
// up and down" — could only be GREPPED, and greps are what this project keeps getting caught by.
// Chromium is already in this container. This renders the real markup with the real CSS and asks
// the browser the same questions the operator asks with his eyes.
//
//     node tools/render-face.js <day> <hh:mm> --page      # writes /tmp/face-page.html
//     node tools/measure-ladder.js
const { chromium } = require('playwright');
(async () => {
  // ⚠ the container pins the browsers under /opt/pw-browsers and the npm-installed playwright
  // expects its own versioned path — point it at what is actually here rather than downloading.
  const fs = require('fs');
  const cand = ['/opt/pw-browsers/chromium-1194/chrome-linux/chrome',
                '/opt/pw-browsers/chromium_headless_shell-1194/chrome-linux/headless_shell'];
  const exe = cand.find(p => { try { return fs.existsSync(p); } catch (e) { return false; } });
  const b = await chromium.launch(exe ? { executablePath: exe } : {});
  const p = await b.newPage({ viewport: { width: 900, height: 900 } });
  await p.goto('file:///tmp/face-page.html');
  await p.waitForTimeout(120);
  const m = await p.evaluate(() => {
    const w = document.querySelector('.g3ladwrap');
    const lad = document.querySelector('.g3lad');
    if (!w || !lad) return { err: 'no ladder' };
    const cs = getComputedStyle(w);
    const wr = w.getBoundingClientRect();
    const pill = n => { const e = [...document.querySelectorAll('.g3ldempill')]
      .find(x => x.textContent.trim().startsWith(n)); if (!e) return null;
      const r = e.getBoundingClientRect();
      return { top: Math.round(r.top - wr.top), bottom: Math.round(r.bottom - wr.top),
               text: e.textContent.trim(), fullyVisible: r.top >= wr.top - 0.5 && r.bottom <= wr.bottom + 0.5 }; };
    // scroll it and see whether it moves
    const before = w.scrollTop;
    w.scrollTop = 99999; const maxDown = w.scrollTop;
    w.scrollTop = 0;     const maxUp = w.scrollTop;
    w.scrollTop = before;
    return {
      wrapClass: w.className, overflowY: cs.overflowY, maxHeight: cs.maxHeight,
      clientH: w.clientHeight, scrollH: w.scrollHeight, scrollTop: Math.round(before),
      canScroll: w.scrollHeight > w.clientHeight, scrollRange: maxDown - maxUp,
      ladderH: lad.getBoundingClientRect().height,
      EH: pill('EH') || pill('~EH'), EL: pill('EL') || pill('~EL')
    };
  });
  console.log(JSON.stringify(m, null, 1));
  await b.close();
})();
