// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function testingInsights(){
  var s=studyLoad(), out={says:[],change:[],improve:[],next:[]};
  var kp=studyPct(s.kingPull&&s.kingPull.all); if(kp!=null) out.says.push('King pulls price '+kp+'% at 30m (n='+s.kingPull.all.n+') — everything else repels.');
  var d2=s.kingPull&&s.kingPull.byDist&&s.kingPull.byDist['2']; if(d2&&d2.n){ out.says.push('Pull peaks at 2 strikes ('+studyPct(d2)+'%, n='+d2.n+'); inside 1 strike price orbits.'); }
  var h11=s.kingPull&&s.kingPull.byHour&&s.kingPull.byHour['11']; if(h11&&h11.n){ out.says.push('The 11am CT hour is the strongest pull window ('+studyPct(h11)+'%, n='+h11.n+').'); }
  var orep=studyPct(s.othersRepel); if(orep!=null) out.says.push('Non-King mass repels '+orep+'% (n='+s.othersRepel.n+').');
  var rB=s.reached&&s.reached.Building, rF=s.reached&&s.reached.Fading;
  if(rB&&rB.n&&rF&&rF.n) out.says.push('Acm walls are sturdiest (reached '+studyPct(rB)+'%) vs Fading '+studyPct(rF)+'% — keep Acm in STATE.');
  out.change.push('READ arms Pull only in the 1.5–3 strike zone; ORBIT shows contact, not direction.');
  out.change.push('Suppress trend/confluence/King-verdict direction claims when regime=CHOP (they ran contrarian).');
  out.improve.push('Most buckets are ⚖ (n<20 per day) — needs ~15 sessions before rates are trusted.');
  out.improve.push('QQQ/VIX now recording every bar; cross-market confluence becomes testable in ~2 weeks.');
  out.improve.push('Episode records (snap.ep) let us separate repel-at-contact from drift-from-distance — add that split.');
  out.next.push('Does a +γ Ceil push harder than a −γ one? (polarity recorded since 08-14)');
  out.next.push('Does the 11am pull window survive 10 more days, or is it a 4-day artifact?');
  out.next.push('Contender ≥60% + King bleeding → does price go to the contender AFTER the crown relocates?');
  out.next.push('Acm wall on FIRST touch → hold rate vs later touches (tap-decay).');
  return out;
}
