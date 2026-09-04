// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function pipCopyStyles(doc){
  try{
    // the PiP window is a separate document with no stylesheets of its own
    var sheets=document.styleSheets;
    for(var i=0;i<sheets.length;i++){
      var sh=sheets[i], txt='';
      try{ var rules=sh.cssRules||[]; for(var r=0;r<rules.length;r++) txt+=rules[r].cssText; }
      catch(eX){ continue; }               // cross-origin sheet: skip, never throw
      if(!txt) continue;
      var st=doc.createElement('style'); st.textContent=txt; doc.head.appendChild(st);
    }
    var base=doc.createElement('style');
    // ⚠⚠ (v12.5) THE REAL REASON THE POP-OUT COULD SHRINK BUT NEVER GROW — AND IT WAS HERE,
    // not in the drag handler. `#gpts-panel` below asks for `height:100% !important`. A PERCENTAGE
    // HEIGHT AGAINST AN AUTO-HEIGHT PARENT COMPUTES TO `auto`. html and body had no height, so the
    // panel silently fell back to its CONTENT height and the `100%` did nothing at all.
    // Measured live 2026-08-24 in the user's own pop-out: window innerHeight 598, panel box 1104.
    // That number is the whole bug. makeResizable captures `oh` from the panel's rect at pointerdown,
    // so every drag started from 1104 instead of 598:
    //   drag DOWN  1px → target 1105 → min(maxOuter, 1105+95) → slams the 820 screen clamp instantly
    //   drag UP  500px → target  604 → 699 outer                → shrinks, proportionally, fine
    // Shrink worked, grow was dead on the first pixel. Adding `height:100%` here made the panel
    // measure 597 against a 598 window in the same live test, and the drag maps 1:1 both ways.
    // ⚠ FIVE versions (v11.93/95/96/97, v12.3/12.4) went at the DRAG for a bug that lived in the BOX.
    // v12.4's pointer capture is a real fix for a real edge case and stays — but it was never this.
    // The lesson is the v12.2 lesson repeated: MEASURE THE BOX BEFORE BLAMING THE HANDLER.
    base.textContent='html,body{margin:0;padding:0;height:100%;background:'+PAL.bg+';color:'+PAL.ink+';'+
      'font:12px/1.4 Inter,Arial,sans-serif;overflow:hidden}'+
      // (v11.93) THE POPPED-OUT PANEL CAN BE RESIZED VERTICALLY AGAIN.
      // It was pinned `height:auto; min-height:100vh` and the grip was hidden outright, so the panel
      // was forced to fill the PiP window and there was nothing to drag — the user had a resize in
      // Skylit and lost it the moment they popped out. `min-height` becomes a floor small enough to
      // get under, the panel keeps whatever height the grip sets, and the grip comes back.
      // ⚠ WIDTH STAYS PINNED TO 100%. In a PiP window the width IS the window's width; a panel
      // narrower than its own window just leaves a dead strip, so the grip is vertical-only here.
      // ⚠⚠ (v11.96) THE HEIGHT REGRESSION I CAUSED AT v11.93.
      // The original rule was `height:auto !important; min-height:100vh` — the panel took its CONTENT
      // height and the window scrolled. v11.93 replaced that whole rule to un-pin it for the grip and
      // DROPPED `height:auto`, so the inline height `restoreSize()` writes from the IN-PAGE size
      // (580.977px) applied in the pop-out and clamped it there.
      // Measured live 2026-08-24: window 598px, panel stuck at 581px, #gpts-body content 1021px.
      // Nearly half the panel was cut, and dragging could only reach the 590px ceiling I had set.
      // ⚠ NO `!important` ON HEIGHT. The stylesheet supplies the default (auto = fit the content) and an
      // inline height from a grip drag has to be able to WIN, or the grip stops working again.
      '#gpts-panel{position:static !important;top:auto !important;left:auto !important;right:auto !important;'+
      // (v12.2) FILL the window and scroll INSIDE. v11.96 made the panel content-height and let the
      // window scroll, which left the grip at the true bottom of a 1000px panel inside a 600px window
      // — unreachable. A panel that fills its window and scrolls internally has its grip on screen
      // always, and the window edge becomes the honest way to make it bigger.
      'width:100% !important;max-width:100% !important;height:100% !important;'+
      'border:0 !important;border-radius:0 !important;box-shadow:none !important;z-index:auto !important}'+
      '#gpts-body{cursor:default !important}'+
      // pinned to the WINDOW, not the panel, so it never scrolls out of reach as the panel grows
      // ⚠⚠ (v11.97) THE GRIP WAS SITTING ON THE OS WINDOW-RESIZE CORNER.
      // Measured live: grip at y=582 in a 598px window — a 16px box in the exact bottom-right corner.
      // That is where the operating system's own window-resize handle lives, so the OS took the press
      // and the page never saw a mousedown. A synthetic drag dispatched INTO the document worked
      // perfectly (997 -> 1117px), which is what proves the handler was never the problem: the pointer
      // simply never reached it.
      // ⚠ Inset from BOTH edges, and make it bigger. A handle you cannot hit is the same as no handle,
      // and this is the third attempt at this one bug — v11.93 unhid it, v11.95 pinned it here.
      // ⚠⚠ (v12.6) THE GRIP IS GONE FROM EVERY POP-OUT, and this is the honest end of a long chase.
      // In a PiP window Chrome reverts every height change, so the handle CANNOT work - five trials,
      // `asked=713 -> settled=693 (reverted)`. In a normal window the OS window edge already does the
      // job, so a handle would be redundant. Either way the panel fills its window (v12.5) and scrolls
      // inside, which is what the grip was standing in for. A control that cannot act is worse than no
      // control: it invites the drag that fails. The grip stays IN-PAGE, where it works.
      '#gpts-grip{display:none !important}';
    doc.head.appendChild(base);
  }catch(e){}
}
