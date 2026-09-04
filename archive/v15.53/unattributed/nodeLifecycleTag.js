// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeLifecycleTag(L){
  var lc=L.lifecycle; if(!lc || !lc.stage) return '';
  var map={
    Fresh:    { lab:'Fresh',  col:PAL.longAccent, tip:'FRESH \u2014 untested this session; full strength (~80% reaction on 1st tap). Academy: target fresh positioning.' },
    Tested:   { lab:'Tested', col:PAL.gold,       tip:'TESTED \u2014 tapped once, held; partially absorbed (~66% on the 2nd tap).' },
    Delivered:{ lab:'Used',   col:PAL.sub,        tip:'DELIVERED \u2014 tapped '+lc.taps+'\u00d7 (graveyard); reaction largely spent (~33% on a 3rd tap). Academy: do NOT target used levels.' },
    Decaying: { lab:'Decay',  col:PAL.shortAccent,tip:'DECAYING \u2014 weakening with no interaction (quiet death); positioning rolling off.' }
  };
  var m=map[lc.stage]; if(!m) return '';
  // (v10.35) compact DOT form: single letter, % + full note in the hover (not on the face).
  var letter={Fresh:'F',Tested:'T',Delivered:'U',Decaying:'D'}[lc.stage]||'?';
  var pr=(lc.prob!=null)?(' \u00b7 ~'+lc.prob+'% reaction'):'';
  // (v10.47 A.3, approved mockup) compact "● T2" (letter + tap count), no circle border
  return '<span title="'+m.tip.replace(/"/g,'')+pr+'" style="color:'+m.col+';font-size:8px;font-weight:800;white-space:nowrap">\u25CF '+letter+(lc.taps?lc.taps:'')+'</span>';
}
