// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function nodeMapBlock(){
  var sym=activeSym();   // (v10.55 PART G) whichever underlying is active (SPY, or QQQ under an NQ/QQQ chart)
  var m=nodeMapModel(sym);
  if(!m || !m.ok){ return '<div style="color:'+PAL.sub+';padding:2px 6px;font-size:11px">Node Map \u2014 waiting on node data\u2026</div>'; }
  // (v10.50) The single Deflection-zones ladder IS the Node Map body now. The Node-Map
  // sentence, the air-pocket standalone line, the "\u21A9 King behind" snapback line, the
  // regime/range chips and the legacy two-sided ladder are RETIRED from the live render
  // (their doctrine folded into the relevant hovers). deflZonesBlock is the whole body.
  var __zonesEarly=''; try{ __zonesEarly=deflZonesBlock(sym); }catch(eZE){ __zonesEarly=''; }
  if(__zonesEarly) return __zonesEarly;
  // ---- FALLBACK (no node meaningful enough to zone): the legacy ladder, WITHOUT the
  // retired sentence/airpocket/snapback narration. ----
  var hdrTip=('NODE MAP \u2014 every magnet price will meet on both sides. IDENTITY: \uD83D\uDC51 King (strongest) \u00b7 \uD83D\uDEAA Gate \u00b7 \u2594 Ceil / \u26F0 Flr = the live range \u00b7 \u2605 Mag strong \u00b7 purple = \u2212\u03B3. STATE: Acm/Dec/Steady + %chg. ACTIVITY: Pull \u00b7 BOw \u00b7 BO\u00b7FT \u00b7 Defl \u00b7 Push. Nodes are magnets: they attract and repel; the ladder shows which side, ACTIVITY shows what it is doing. The highlighted side is where price is currently heading (trend + momentum); the other side is still shown. Verdicts sharpen once polarity lands.').replace(/"/g,'');
  // (v10.24) REGIME chip in the header = whole-board GEX-structure read, colored by
  // direction; its instruction (fade edges / stand aside / pullbacks) rides below.
  var rg=m.regime||{label:'Forming',dir:0,conf:'low',why:''};
  var rgCol = rg.dir>0?PAL.longAccent:(rg.dir<0?PAL.shortAccent:(rg.label==='Whipsaw'?PAL.amber:PAL.sub));
  var rgChip = '<span title="'+(rg.why||'').replace(/"/g,'')+'" style="color:'+rgCol+';font-size:9px;font-weight:800;padding:1px 7px;border:1px solid '+rgCol+';border-radius:20px">'+rg.label+(rg.skew!=null&&/Trend/.test(rg.label)?(' '+rg.skew+'\u00d7'):'')+'</span>';
  // (v10.28) \u2464 Step-5 icon in the Node Map header (Map the Flow). Rendered inside
  // the header title so it clicks open the Step-5 popover like the other step icons.
  // (v10.44) RANGE chip = ⛰Flr–▔Ceil (nearest strong magnets each side) + inside/OUT.
  var rangeChip='';
  if(m.range){ var rin=m.range.inside; rangeChip='<span title="Live range = nearest strong magnet below (Flr) to nearest strong magnet above (Ceil), each >= '+FLRCEIL_MIN_PCT+'% of King mass. Inside = rotation between two magnets; outside = the range is being redefined (a break + FT re-anchors it to the next strong magnet)." style="color:'+(rin?PAL.ink:PAL.amber)+';font-size:9px;font-weight:700;padding:1px 7px;border:1px solid '+PAL.line+';border-radius:20px;margin-right:4px">Range <b>'+fmtLvl(m.range.lo)+'\u2013'+fmtLvl(m.range.hi)+'</b> \u00b7 '+(rin?'inside':'OUT')+'</span>'; }
  // (v10.47, user-directed) The header is ONE plain sentence: CONTINUATION / REVERSAL / TBD /
  // NO NODE IN PLAY at the engaged node, and WHY in Step-5 vocabulary (accumulating / decreasing).
  // Range + pattern chips retired from the header (they ride in the hover); imbalance line folded in.
  // (v10.50) Node-Map sentence RETIRED from live render — fallback body is the legacy ladder only.
  var html='';
  // (v10.50) air-pocket standalone line RETIRED (concept preserved by tradeFrame (air) tag on
  // tgt/inval); the snapback "King behind" line RETIRED (folded into the READ layer).
  // (v10.29) Pattern instruction line REMOVED \u2014 the Pattern BADGE in the header (Trend/
  // Whipsaw/Rainbow Road) already names it, and its stance rides in the badge hover.
  // ('regime' renamed to 'Pattern' per Skylit's Patternpedia vocabulary.)
  // (v10.27) S/R IMBALANCE net-read FOLDED IN here (standalone section removed).
  // This is the Step-5 flow conclusion: the DIVERGENCE in build-RATE between the two
  // sides near price (not who is bigger). srBattle is render-cached, so this matches
  // the same value the old section showed. Below it: the tradeable crossover banner.
  // (v10.47) imbalance sentence + crossover banner REMOVED from here — the header sentence carries the flow read.
  // strongest headline (preserves old PROJ job): strongest Sup + strongest Res.
  function strongChip(c, word, col){
    if(!c) return '<span style="color:'+PAL.sub+';font-size:10px">'+word+' \u2013</span>';
    return '<span title="Strongest '+(word==='Sup'?'floor below':'ceiling above')+' (size+build-rate+nearness blend)" style="display:inline-flex;align-items:center;gap:3px;color:'+col+';font-weight:800;font-size:10px;padding:1px 7px;border:1px solid '+col+';border-radius:20px">\u2605 '+word+' '+fmtLvl(c.k)+'<span style="color:'+PAL.sub+';font-weight:600;font-size:9px">'+c.pct+'%</span></span>';
  }
  // (v10.27) Strongest Sup/Res chips REMOVED here \u2014 redundant with the King
  // 3-magnet header (\u2605SUP \u2190 \uD83D\uDC51 \u2192 \u2605RES). Snapback warning retained.
  // (v10.50) snapback "King behind" line RETIRED from live render.
  // (v10.24) RUG flag: the polarity-gated nosedive/squeeze. Shows a hard flag once
  // polarity is verified; until then, a dimmed 'candidate (verify polarity)' note.
  // (v10.29) RUG callout line REMOVED \u2014 geometry / forming-state / targets now live in
  // the RugC/RugF (RRugC/RRugF) per-node BADGE HOVER. m.rug still drives those badges.
  // (v10.37) gatekeeper instruction line REMOVED - the gatekeeper strike + distance now
  // rides in the King badge bottom row (kingBlock). Standalone section also removed.
  // (v10.29) DOUBLE-STACK + CLUSTER callout lines REMOVED \u2014 fully redundant with the
  // per-node DStk / Clst badges; span + node-count + meaning now ride in the badge HOVER.
  // (v10.27) PER-NODE BO TAG: the breakout-pullback lifecycle now rides on the node
  // it belongs to (standalone BO section removed). Finds the most-advanced non-voided
  // setup at this strike and renders its stage chain (BO / BO\u00b7FT / BO\u00b7FT\u00b7TST\u2026),
  // colored by direction (long=green, short=red). Empty when no live setup here.
  function setupTagForNode(k){
    var S=STATE[sym]; if(!S||!S.setups) return '';
    var STAGES=['BO','FT','TST','CONF','GO'];
    // (v10.43 BO-TAG FIX, user-reported) The old selector took the HIGHEST-STAGE
    // setup ever seen at the strike — so a finished GO setup from the morning
    // showed "BO·FT·TST·CONF·GO" all day while the LIVE setup was only at BO·FT.
    // Now: terminal setups (T2/FAILED/EXPIRED) are skipped entirely (the Node
    // Map's resolved-outcome echo already shows those), and among LIVE setups
    // the MOST RECENT wins — the chip reflects what is happening NOW.
    var best=null, bestT=-1;
    for(var key in S.setups){
      var s=S.setups[key];
      if(!s || s.voided || s.strike!==k) continue;
      if(s.outcome==='T2'||s.outcome==='FAILED'||s.outcome==='EXPIRED') continue;
      var tSel=(s.updated||s.ts||0);
      if(tSel>bestT){ bestT=tSel; best=s; }
    }
    var bestRank=best?STAGES.indexOf(best.stage):-1;
    if(!best || bestRank<0) return '';
    // (v10.44, user-directed) Only TWO chips are ever displayed: BOw (initial break,
    // pre-FT) and BO·FT (follow-through confirmed). TST/CONF/GO keep running internally
    // for the scorecard but are NOT shown as a chain.
    var col = best.dir==='long'?PAL.longAccent:PAL.shortAccent;
    var isFT = bestRank>=1;
    var chain = isFT ? 'BO\u00b7FT' : 'BOw';
    var tip = (best.dir==='long'?'Long':'Short')+' breakout at '+fmtLvl(k)+' \u2014 '+(isFT?('FOLLOW-THROUGH confirmed'+(best.ftLenient?' (lenient rule: two progressing closes beyond)':' (bar held fully beyond)')+'. Internal stage: '+best.stage+'.'):'initial break \u2014 first close beyond the node; watching for FT (a bar holding fully beyond, OR two consecutive progressing closes beyond).')+' Requires a '+BO_HL_LOOKBACK+'-bar '+(best.dir==='long'?'high':'low')+' at breakout.';
    return '<span title="'+tip.replace(/"/g,'')+'" style="color:'+col+';font-size:8px;font-weight:800;letter-spacing:.3px;border:1px solid '+col+';border-radius:9px;padding:0 4px;white-space:nowrap">'+chain+'</span>';
  }
  // the two-sided ladder
  function row(L){
    var isPx=false;
    var col=nmVerdictColor(L.verdict, L.side);
    // (v10.26-prep Step 5) node STATUS (Acm/Diss/Steady + reshuffle) and TYPE (+\u03b3/\u2212\u03b3)
    // replace the old bare directional arrow with an explicit, doc-vocabulary identity.
    var statusHtml = nodeStatusTag(L);
    var typeHtml = '';   // (v10.44) ±γ text DROPPED — purple/default identity color carries polarity
    // (v10.33) lifecycle tag: show for tested/used/decaying always; for FRESH only on
    // structurally important nodes (King/Gatekeeper/strong) so untouched minor nodes
    // don't spam a 'Fresh' badge on every row.
    var lcStage=(L.lifecycle&&L.lifecycle.stage)||'';
    var lcImportant=(L.isKing||L.isGatekeeper||L.isStrong);
    var lifeHtml=(lcStage && (lcStage!=='Fresh' || lcImportant)) ? nodeLifecycleTag(L) : '';
    var marks=(L.isStrong?'\u2605':'')+(L.isKing?'\uD83D\uDC51':'')+(L.isGatekeeper?'\uD83D\uDEAA':'')+(L.isRugTarget&&m.rug&&m.rug.shown?'\uD83E\uDDF6':'');
    var bg = L.onEmphasis ? (L.side==='above'?'rgba(240,97,109,0.06)':'rgba(46,194,126,0.06)') : 'transparent';
    // (v10.26-prep) ROLE/SETUP badge replaces the removed predictive verdict pill.
    // Factual (what the node IS), never a Bounce/Break prediction. 'forming' fallback
    // only when the node has too little history to classify AND carries no role.
    var roleBadge = nodeRoleBadge(L);
    var vtag = roleBadge ? roleBadge
      : (L.forming ? '<span style="color:'+PAL.sub+';font-size:8px;font-weight:700">forming</span>' : '');
    // (v10.25 Step 5) ATTRACTION-ONLY stage + resolved-OUTCOME echo. NO deflect/break
    // prediction: accumulation only ATTRACTS. Stage says where price is vs the magnet;
    // the outcome (broke/held/false) is a report the BO state machine resolves over time.
    var stageHtml='';
    var om = outcomeMarker(L.outcome);
    // (v10.34) DEFLECTION takes PRECEDENCE — it is the actionable event (a setup is
    // 'meaningless until there is a deflection'). It is a DETECTED reversal off this
    // node, reported after confirmation, never predicted.
    if(L.deflection){
      var df=L.deflection;
      var dcol=(df.dir>0)?PAL.longAccent:PAL.shortAccent;
      var darrow=(df.dir>0)?'\u2191':'\u2193';
      var roleName=(L.isKing?'King':(L.isGatekeeper?'Gatekeeper':(L.role||'node')));
      var flav=(df.pos===false)?' \u2014 note: \u2212gamma node (accelerant), a deflection here is counter-character'
              :(df.pos===true?' \u2014 +gamma node (deflection expected)':'');
      var dtip=('DEFLECTION off the '+roleName+' at '+fmtLvl(L.k)+' \u2014 price tapped it and reversed '+(df.dir>0?'UP (bounce off support)':'DOWN (rejection off resistance)')+', ~'+df.awayPts+' strikes over '+df.bars+' bars. Detected event (reported after confirmation), not a prediction.'+flav).replace(/"/g,'');
      stageHtml='<span title="'+dtip+'" style="color:'+dcol+';font-size:8.5px;font-weight:800">Defl '+darrow+'</span>';
    } else if(om){
      // resolved: the node echoes what actually happened (report, not prediction).
      stageHtml='<span style="color:'+om.col+';font-size:8.5px;font-weight:800">'+om.txt+'</span>';
    } else if(L.attract && L.attract.stage==='at-node'){
      stageHtml='<span style="color:'+PAL.gold+';font-size:8.5px;font-weight:800">at node \u00b7 watch BO</span>';
    } else if(L.attract && L.attract.stage==='attracting'){
      stageHtml='<span style="color:'+PAL.blue+';font-size:8.5px;font-weight:700">attracting</span>';
    }
    // (v10.27) per-node BO lifecycle chain (BO\u00b7FT\u00b7TST\u00b7CONF\u00b7GO)
    var boTag = setupTagForNode(L.k);
    // ---- (v10.35) 4-ZONE ROW REDESIGN ----------------------------------------
    // ZONE 1 IDENTITY: one merged role pill (icon+word), no duplication.
    var idPill = nodeRolePill(L) || (L.forming ? '<span style="color:'+PAL.sub+';font-size:8px;font-weight:700">forming</span>' : '');
    // (v10.44) ZONE 4 ACTIVITY — magnet vocabulary, ONE chip by priority:
    //   fresh Defl (event) > BO·FT > BOw > Push > Pull > resolved echo > blank.
    // Pull/Push carry the toward-share % (fraction of the last EP_TW_BARS closes that
    // moved NEARER the node). Push off a node BELOW price = green (bullish bounce),
    // above = red. Every chip carries the episode timeline in its hover.
    var ep=L.ep||{};
    var epTip=('Episode @ '+fmtLvl(L.k)+' \u2014 zone '+ep.zone+(ep.dist!=null?(' ('+ep.dist+' strikes)'):'')+
      (ep.tw!=null?(' \u00b7 toward-share '+ep.tw+'% of last '+EP_TW_BARS+' bars'):'')+
      (ep.tagged!=null?(' \u00b7 last tag '+ep.tagged+' bars ago'):' \u00b7 no tag in window')+
      (ep.crosses?(' \u00b7 crossings '+ep.crosses):'')+
      '. Pull >= '+EP_PULL_TW+'% (baseline 54\u201360% \uD83D\uDCCA) \u00b7 Push <= '+EP_PUSH_TW+'% after contact \u00b7 BOw = at the node (watch / initial break) \u00b7 BO\u00b7FT = confirmed follow-through.').replace(/"/g,'');
    var deflFresh = !!(L.deflection && L.deflection.bars<=DEFLECT_CONFIRM+EP_DEFL_HANDOFF);
    var boChip = setupTagForNode(L.k);   // '' | BOw | BO·FT
    var activity='';
    if(deflFresh){ activity=stageHtml; }
    else if(boChip && /FT/.test(boChip)){ activity=boChip; }
    else if(boChip || ep.state==='BOw'){ activity = boChip || ('<span title="'+epTip+' At the node now \u2014 breakout WATCH: first close beyond = initial break; FT confirms." style="color:'+PAL.gold+';font-size:8.5px;font-weight:800;border:1px solid '+PAL.gold+';border-radius:9px;padding:0 5px">BOw</span>'); }
    else if(ep.state==='Push'){ var pcol=(L.side==='below')?PAL.longAccent:PAL.shortAccent; var parr=(L.side==='below')?'\u2191':'\u2193';
      activity='<span title="'+epTip+' PUSH \u2014 price tagged this magnet and is being repelled (don\u2019t fade the exit). \uD83D\uDCCA non-King mass repels 57% at 30m (n=350)." style="color:'+pcol+';font-size:8.5px;font-weight:800;border:1px solid '+pcol+';border-radius:9px;padding:0 5px">Push '+parr+'</span>'; }
    else if(ep.state==='Pull'){ activity='<span title="'+epTip+' PULL \u2014 price closing in on this magnet. \uD83D\uDCCA King pull 69% at 2 strikes (n=59), 74% in the 11am hour." style="color:'+PAL.blue+';font-size:8.5px;font-weight:700;border:1px solid '+PAL.blue+';border-radius:9px;padding:0 5px">Pull</span>'; }
    else if(L.deflection){ activity=stageHtml; }
    else if(om){ activity='<span style="color:'+om.col+';font-size:8.5px;font-weight:800">'+om.txt+'</span>'; }
    var kcol=(L.isKing?PAL.gold:PAL.ink);
    // (v10.47b, user-directed) ONE LINE per node, fixed aligned columns, nothing wraps.
    return '<div style="display:grid;grid-template-columns:96px 66px 78px 1fr;align-items:center;column-gap:4px;font-size:9.5px;padding:2px 5px;border-radius:5px;background:'+bg+';white-space:nowrap">'+
      '<span style="display:flex;align-items:center;gap:3px;overflow:hidden;text-overflow:ellipsis;min-width:0">'+idPill+'</span>'+
      '<span style="font-variant-numeric:tabular-nums;font-weight:700;color:'+PAL.sub+';overflow:hidden"><b style="color:'+kcol+'">'+fmtLvl(L.k)+'</b> \u00b7 '+L.pct+'%</span>'+
      '<span style="display:flex;align-items:center;gap:3px;overflow:hidden">'+statusHtml+typeHtml+'</span>'+
      '<span style="display:flex;align-items:center;justify-content:flex-end;gap:5px;overflow:hidden">'+
        (activity?'<span>'+activity+'</span>':'')+
        (lifeHtml||'')+
      '</span></div>';
  }
  var px=m.px;
  // (v10.50) zones already returned early above; this legacy ladder is the last-resort
  // fallback. Step icon ⑤ RETIRED from the column header (doctrine folded into hovers).
  html+='<div style="display:grid;grid-template-columns:96px 66px 78px 1fr;column-gap:4px;font-size:7px;letter-spacing:.3px;text-transform:uppercase;color:'+PAL.sub+';font-weight:800;padding:0 5px 2px;white-space:nowrap">'+
    '<span style="display:flex;align-items:center;gap:3px">Identity</span>'+
    '<span>Strike \u00b7 %</span>'+
    '<span title="Acm / Dec / Steady + the node\u2019s \u25b2\u25bc% vs its session open">State</span>'+
    '<span style="text-align:right" title="Pull \u00b7 BOw \u00b7 BO\u00b7FT \u00b7 Defl \u00b7 Push \u00b7 echoes (broke/held/FBO) \u2014 then the lifecycle dot">Activity \u00b7 Life</span></div>';
  var printed=false;
  m.levels.forEach(function(L, i){
    if(!printed && L.k<=px){
      html+='<div title="Current SPY price." style="display:flex;justify-content:center;align-items:center;height:14px;margin:1px 0;background:'+PAL.card+';border:1px dashed '+PAL.blue+';border-radius:6px;color:'+PAL.blue+';font-size:9px;font-weight:800">\u2014 SPY '+fmtLvl(px)+' \u2014</div>';
      printed=true;
    }
    html+=row(L);
  });
  if(!printed){ // price is below every mapped level
    html+='<div style="display:flex;justify-content:center;align-items:center;height:16px;margin:2px 0;background:'+PAL.card+';border:1px dashed '+PAL.blue+';border-radius:6px;color:'+PAL.blue+';font-size:10px;font-weight:800">\u2014 SPY '+fmtLvl(px)+' \u2014</div>';
  }
  return html;
}
