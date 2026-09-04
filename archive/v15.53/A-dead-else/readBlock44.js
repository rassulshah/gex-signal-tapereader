// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function readBlock44(sym){
  sym=sym||'SPY';
  var S=STATE[sym]||{}; var px=S.price; if(px==null) return '';
  var m=nodeMapModel(sym); if(!m||!m.ok) return '';
  var sp=null; try{ sp=spineOf(sym); }catch(eSp){}
  var d=(sp&&sp.dir)||(function(){ try{ return directionGrade(sym); }catch(e){ return {dir:'SIDE',grade:'C',disp:'C',inputs:{}}; } })();
  var L=(sp&&sp.inPlay); if(L==null){ try{ L=inPlayZone(sym); }catch(eL){ L=null; } }
  var dr; try{ dr=driftRead(sym); }catch(eDr){ dr={dir:0,verdict:'NONE'}; }
  var dirWord=d.dir||'SIDE';
  var dirNum=dirWord==='UP'?1:(dirWord==='DN'?-1:0);
  // POTENTIAL target = next node in the direction, capped at the King (from the frame).
  var tgtK=null, tgtRole='';
  try{
    if(L){ var fr=tradeFrame(sym,L,dirNum||nodeHoldDir(L,px));
      if(fr && fr.tgt!=null){ tgtK=fr.tgt;
        var tl=(m.levels||[]).filter(function(x){return Math.abs(x.k-tgtK)<0.001;})[0]; if(tl) tgtRole=zoneRole(tl); } }
  }catch(eFr){}
  var legR=(sp&&sp.leg)||null;
  if(!legR){ try{ legR=legEngine(sym); }catch(eLg){ legR=null; } }
  READ_SYM=sym;
  var v=read3Beat(dirWord, L, px, m.flr, m.ceil, dr, tgtK, tgtRole, d.relation||null, legR, d.capped||null);
  // (v11.0 audit G8) THE READ IS ON THE RECORD. The sentence the user actually saw (and the leg
  // voice id) was never recorded, so the brief's contradiction #1 (READ vs direction) was
  // vacuous. LAST_READ is picked up by the `dir` feature record on the closed bar.
  try{ LAST_READ[sym]={ t:Date.now(), verdict:v.verdict, sentence:String(v.sentence||'').slice(0,240), voiceId:v.voiceId||null,
                        legDir:legR?legR.dir:'none', legPhase:legR?legR.phase:'none', dirSrc:legR?(legR.dirSrc||'sma'):null,
                        map:String(v.structure||'').slice(0,240), structureId:v.structureId||null }; }catch(eLR){}
  var vcol={BULLISH:PAL.longAccent,BEARISH:PAL.shortAccent,SIDEWAYS:PAL.sub}[v.verdict]||PAL.sub;
  // Direction grade chip — letter only, NO ⚖ (the tier + regime live in the hover).
  var g=d.grade||'C', gdisp=d.disp||g;
  var gcol=g==='A'?PAL.longAccent:(g==='B'?PAL.blue:PAL.amber);
  var gbg =g==='A'?'rgba(46,194,126,.15)':(g==='B'?'rgba(74,144,217,.15)':'rgba(242,180,90,.15)');
  var di=d.inputs||{};
  // (v10.51) QUESTION-FIRST direction hover: the hierarchy, stated in one breath, then
  // this bar's actual values. The grade is a confidence read on the TREND, not a blend.
  var dTip=((DRIFT_LIVE
    ? ('Which way, and how sure? SMA-50 sets the trend (15/20 closes); GEX/VEX drift confirms it or diverges from it. '+
       'Trend UP + drift UP = confirmed. Trend UP + drift DOWN = divergence, caution. No trend = drift gives a tentative lean, never better than C. ')
    : ('Which way, and how sure? SMA-50 sets the trend (15/20 closes): a confirmed trend is a B, a confirmed roll adds a point, mid-range and chop cap at C, no trend = SIDE at C. '+
       'GEX/VEX drift is in SHADOW mode — recorded every bar and reviewed nightly, not shown and not voting until it clears the promotion bar. '))+
    'Now: trend '+(d.trendState||'?')+' ('+((di.trend&&di.trend.up)||0)+' up / '+((di.trend&&di.trend.dn)||0)+' dn of '+((di.trend&&di.trend.win)||0)+') '+
    '· drift '+((di.drift&&di.drift.verdict)||'?')+' → '+(d.relation||'?')+' → '+(dirWord==='UP'?'Up':(dirWord==='DN'?'Down':'Sideways'))+' '+gdisp+
    '. Score '+(d.score!=null?d.score:'?')+' (live cut points A≥'+DIR_WEIGHTS.gradeA+', B≥'+DIR_WEIGHTS.gradeB+', else C)'+(d.capped?('; hard-capped to C by '+d.capped):'')+
    '. '+((function(){ var R=di.roll; if(!R || R.dir==='none') return 'No leg engine reading (no confirmed trend). ';
      var w=(R.dir==='dn')?'lower':'higher';
      var s='Roll: '+R.count+' pullback node'+(R.count===1?'':'s')+' '+w+(R.confirmed?' — confirmed (+1)':(R.signal?' — signal':''))+
            (R.weakening?(' · PB rolled AGAINST the trend — weakening ('+(R.vote||-1)+')'):(R.vote>0?'':''))+
            (R.pb!=null?(' · PB '+fmtLvl(R.pb)):'')+(R.magnet!=null?(' · magnet '+fmtLvl(R.magnet)):'')+
            '. Multi-session rolling '+(R.session&&R.session.ready?('votes '+R.session.vote):('records only — '+((R.session&&R.session.note)||'needs 3 sessions')))+'. ';
      return s; })())+
    'Structure '+((di.structAsym&&di.structAsym.bias)||'?')+' and King roll are RECORDED, not voted, until measured. '+
    ((d.tier==='⚖')?('Hand-set \u2696 \u2014 becomes \uD83D\uDCCA only when this panel has promoted it past the bar AND measured eff n\u2265'+RULE_UNLOCK_N+' locally.'):'Measured \uD83D\uDCCA on local evidence.')).replace(/"/g,'');
  // Session badge — dim normally; HIGHLIGHT only power hour / OPEX.
  var sb=sessionBucket();
  var hot=(sb.bucket==='power'||sb.opex);
  var sTip=('What part of the day? '+sb.bucket+(sb.opex?' · OPEX (3rd Friday)':'')+
    '. The same structure means different things at 08:35 and 14:45'+(sb.capOdds?' — power-hour/open-drive flows are mechanical, so no odds claim is made now.':'.')).replace(/"/g,'');
  var sBadge='<span title="'+sTip+'" style="font-size:8px;font-weight:700;color:'+(hot?PAL.gold:PAL.sub)+';border:1px solid '+(hot?PAL.gold:PAL.line)+';border-radius:8px;padding:0 4px">'+(SESSION_ABBR[sb.bucket]||sb.bucket)+(sb.opex?' opex':'')+'</span>';
  // Model-heat — show ONLY when cold (the tool grading itself poorly on this tape).
  var heat=modelHeat(sym); var hBadge='';
  if(heat.state==='cold'){
    hBadge='<span title="Is the model reading this tape well? The last '+heat.n+' resolved direction/node grades were '+heat.rate+'% right (below 40% = cold). Descriptive self-report, never a forecast." style="font-size:8px;font-weight:700;color:'+PAL.shortAccent+';border:1px solid '+PAL.shortAccent+';border-radius:8px;padding:0 4px">model cold</span>';
  }
  var rTip=('Which way is the tape leaning, and where can it go? '+v.verdict+' — '+v.sentence+
    ' · Structure: every meaningful node within '+PB_REACH+' strikes (SPY strikes and the SPXW lanes) is read for accumulation — Acm = %King up ≥'+MAP_ACM+'% over 15m, Dec = down ≥'+(-MAP_DEC)+'% or ≥'+MAP_DROP+'% off its session peak or dropped out of the book; a Dec node with an Acm neighbour on the same side = the ceiling/floor rolling. Descriptive, never an instruction.').replace(/"/g,'');
  // (v11.0.1) the bar a pullback node lands: the read takes the ⚑ style (red down / green up)
  var evt=!!v.event; var evCol=evt?((legR&&legR.dir==='dn')?PAL.shortAccent:PAL.longAccent):PAL.gold;
  var evBg=evt?((legR&&legR.dir==='dn')?'rgba(240,97,109,.07)':'rgba(63,185,80,.07)'):PAL.card;
  if(evt){ try{ fireAlert('pbNode', String(legR.pbDetected.k), evCol); }catch(eA){} }
  var bodyHtml=(v.sentenceHtml||v.sentence);
  if(evt){ var cut=bodyHtml.indexOf('. Deflection expected'); if(cut>0) bodyHtml='<b style="color:'+evCol+'">⚑ '+bodyHtml.slice(0,cut+1)+'</b>'+bodyHtml.slice(cut+1); else bodyHtml='<b style="color:'+evCol+'">⚑</b> '+bodyHtml; }
  return '<div style="border-left:2px solid '+evCol+';background:'+evBg+';border-radius:0 8px 8px 0;padding:3px 7px;margin:2px 0 5px">'+
    '<div style="display:flex;align-items:center;gap:5px;font-size:11px;font-weight:800;white-space:nowrap;overflow:hidden;margin-bottom:1px">'+
      '<span title="'+dTip+'" style="color:'+vcol+'">'+v.arrow+' '+v.verdict+'</span>'+
      '<span style="color:'+PAL.line+'">·</span>'+sBadge+hBadge+
      // (v11.1.1) the grade sits at the RIGHT edge, level with the Next Stop grade above it
      '<span title="'+dTip+'" style="margin-left:auto;color:'+gcol+';background:'+gbg+';padding:0 5px;border-radius:3px;font-size:10px">'+gdisp+'</span>'+
    '</div>'+
    '<div title="'+rTip+'" style="font-size:9.5px;line-height:1.35;color:'+PAL.ink+';display:-webkit-box;-webkit-line-clamp:4;-webkit-box-orient:vertical;overflow:hidden">'+bodyHtml+'</div>'+
  '</div>';
  // (v11.0.1) the separate ⚑ banner and the "Map:" line are folded INTO the read above.
}
