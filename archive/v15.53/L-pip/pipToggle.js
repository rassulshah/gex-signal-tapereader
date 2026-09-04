// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function pipToggle(){
  try{
    if(pipOpen()){ try{ PIPWIN.close(); }catch(e){} return; }
    if(winOpen()){ try{ WINWIN.close(); }catch(e){} }   // one panel, one home
    if(!pipSupported()){
      alert('This Chrome does not expose Document Picture-in-Picture.\n\nUse Chrome 116+ , or drag the tab out into its own window.');
      return;
    }
    // (v11.35) Size from the SCREEN, not from the panel's current width. Opening at ~369px on a 2032px
    // monitor was the whole complaint: the window could be dragged bigger, but nothing in it grew.
    var r=PANEL?PANEL.getBoundingClientRect():{width:440,height:700};
    var sw=(screen&&screen.availWidth)||1440, sh=(screen&&screen.availHeight)||900;
    var wantW=Math.round(Math.max(420, Math.min(900, Math.max(r.width||440, sw*0.26))));
    var wantH=Math.round(Math.max(600, Math.min(sh-80, sh*0.88)));
    window.documentPictureInPicture.requestWindow({ width:wantW, height:wantH }).then(function(w){
      PIPWIN=w;
      pipCopyStyles(w.document);
      // remember where the panel came from so it goes back to exactly that spot
      PIPHOST={ parent:PANEL.parentNode, next:PANEL.nextSibling,
                 h:PANEL.style.height, w:PANEL.style.width };   // (v11.96) remember the in-page size
      w.document.body.appendChild(PANEL);
      // (v11.96) CLEAR THE IN-PAGE INLINE SIZE. restoreSize() writes an explicit height for the
      // in-page panel; carried into the pop-out it overrides the stylesheet and pins the panel to the
      // old height inside a window that has nothing to do with it. The pop-out starts at content
      // height and the grip can then set its own.
      try{ PANEL.style.height=''; PANEL.style.width=''; }catch(eSz){}
      w.addEventListener('pagehide', pipRestore);
      try{ zoomApply(); }catch(eZ){}
      try{ render(); }catch(e2){}
    }).catch(function(err){
      // requestWindow rejects without a user gesture, and that is the usual cause
      try{ console.warn('[GPTS] pop-out refused:', err && err.message); }catch(e3){}
    });
  }catch(e){}
}
