// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function projRecs(sc){
  var recs=[];
  function pct(a,b){ return b?Math.round(100*a/b):null; }
  if(sc.covN>=20){ var cv=pct(sc.cov,sc.covN);
    if(cv<60) recs.push({sev:'high', t:'Cone too narrow: coverage '+cv+'% < 70% target — raise cone multiplier (0.8 → ~1.0).'});
    else if(cv>85) recs.push({sev:'low', t:'Cone too wide: coverage '+cv+'% — tighten multiplier for a more informative band.'});
    else recs.push({sev:'ok', t:'Cone coverage '+cv+'% — within target band.'});
  } else recs.push({sev:'info', t:'Cone coverage: recording ('+sc.covN+'/20 samples to unlock).'});
  if(sc.etaMed!=null && sc.etaErrs.length>=15){
    if(sc.etaMed>6) recs.push({sev:'med', t:'ETA runs late (median +'+sc.etaMed+'m): approach-rate window too short — try 5-bar rate.'});
    else if(sc.etaMed<-6) recs.push({sev:'med', t:'ETA runs early (median '+sc.etaMed+'m): price accelerates into targets — consider convexity term.'});
    else recs.push({sev:'ok', t:'ETA error median '+sc.etaMed+'m — acceptable.'});
  } else recs.push({sev:'info', t:'ETA error: recording ('+sc.etaErrs.length+'/15).'});
  if(sc.succN>=10){ var sr=pct(sc.succ,sc.succN);
    if(sr<SUCC_META.hz30) recs.push({sev:'high', t:'Succession hit '+sr+'% < the measured '+SUCC_META.hz30+'% — check the node STATE split before touching the 60% threshold; size alone is the weakest of the three.'});
    else recs.push({sev:'ok', t:'Succession hit '+sr+'% (n='+sc.succN+') — at or above the measured '+SUCC_META.hz30+'% (n='+SUCC_META.n+'); the live rate replaces the label.'});
  } else recs.push({sev:'info', t:'Succession: '+sc.succN+'/10 live samples — label uses the '+SUCC_META.sessions+'-session measurement ('+SUCC_META.hz30+'% <=30m, n='+SUCC_META.n+').'});
  if(sc.pinN>=5){ var pr=pct(sc.pin,sc.pinN);
    recs.push({sev:(pr>=60?'ok':'med'), t:'Pin-band hit '+pr+'% (n='+sc.pinN+').'+(pr<60?' Widen band to ±0.75 or require taps<3.':'')});
  } else recs.push({sev:'info', t:'Pin band: '+sc.pinN+'/5 flagged closes recorded.'});
  if(sc.reachN>=20){ recs.push({sev:'ok', t:'Target reach by close: '+pct(sc.reach,sc.reachN)+'% (n='+sc.reachN+').'}); }
  return recs;
}
