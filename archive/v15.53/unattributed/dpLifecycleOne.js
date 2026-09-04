// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function dpLifecycleOne(px, printedAt, bars, spot){
  var out={ st:'UNKNOWN', why:'', touches:0, obsFrom:null, crossed:false };
  try{
    if(!(px>0) || !bars || !bars.length) { out.why='no price history in the window'; return out; }
    var tol=px*DP_TOL_PCT;
    var seq=[];
    for(var i=0;i<bars.length;i++){
      var b=bars[i];
      if(!b || typeof b.time!=='number') continue;
      if(printedAt!=null && b.time*1000 < printedAt) continue;   // only what happened AFTER the print
      if(b.high==null || b.low==null || b.close==null) continue;
      seq.push(b);
    }
    if(!seq.length){ out.why='no bars since the print'; return out; }
    out.obsFrom=seq[0].time*1000;
    var side0=(seq[0].close>=px)?1:-1, cur=side0, flips=0, lastFlipIdx=-1, off=true;
    for(var j=0;j<seq.length;j++){
      var c=seq[j];
      var touching=(c.low<=px+tol && c.high>=px-tol);
      if(touching){ if(off){ out.touches++; off=false; } }
      else if(Math.abs(c.close-px)>tol*3) off=true;              // must clearly leave before a re-touch counts
      var sd=(c.close>=px)?1:-1;
      if(sd!==cur){ cur=sd; flips++; lastFlipIdx=j; }
    }
    out.crossed=(flips>0);
    var nowSide=(spot!=null)?((spot>=px)?1:-1):cur;
    var atNow=(spot!=null && Math.abs(spot-px)<=tol);
    if(atNow){ out.st='RETESTING'; out.why='price is on it now'; return out; }
    if(!out.crossed){
      if(out.touches>0){ out.st='HOLDING';
        out.why='tested '+out.touches+' time'+(out.touches>1?'s':'')+' and never closed through'; }
      else { out.st='FRESH'; out.why='never tested since the print'; }
      return out;
    }
    if(nowSide===side0){ out.st='RECLAIMED'; out.why='closed through, then closed back to its own side'; return out; }
    // it is broken. Did price come back and get rejected FROM THE NEW SIDE? that is the flip.
    var tAfter=0, offF=true;
    for(var m=lastFlipIdx+1;m<seq.length;m++){
      var f=seq[m];
      if(f.low<=px+tol && f.high>=px-tol){ if(offF){ tAfter++; offF=false; } }
      else if(Math.abs(f.close-px)>tol*3) offF=true;
    }
    if(tAfter>0){ out.st='FLIPPED';
      out.why='broke, then tested '+tAfter+' time'+(tAfter>1?'s':'')+' from the other side and held \u2014 '+
              (side0>0?'support became resistance':'resistance became support'); }
    else { out.st='BROKEN'; out.why='closed through and has stayed on the far side'; }
    return out;
  }catch(e){ out.why='lifecycle unreadable'; return out; }
}
