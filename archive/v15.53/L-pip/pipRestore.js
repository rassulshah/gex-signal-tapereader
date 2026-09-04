// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function pipRestore(){
  try{
    if(PANEL && PIPHOST && PIPHOST.parent){
      if(PIPHOST.next && PIPHOST.next.parentNode===PIPHOST.parent) PIPHOST.parent.insertBefore(PANEL, PIPHOST.next);
      else PIPHOST.parent.appendChild(PANEL);
      // the inline geometry the panel had before it was moved is restored by restorePos/restoreSize
      PANEL.style.position='fixed';
      // (v11.96) the in-page size the pop-out cleared
      try{ if(PIPHOST.h) PANEL.style.height=PIPHOST.h; if(PIPHOST.w) PANEL.style.width=PIPHOST.w; }catch(eR){}
      try{ restorePos(); restoreSize(); zoomApply(); }catch(e1){}
    }
  }catch(e){}
  PIPWIN=null; PIPHOST=null;
  try{ render(); }catch(e2){}
}
