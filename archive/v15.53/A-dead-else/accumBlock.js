// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function accumBlock(){
  var __asym=activeSym();   // (v10.55 PART G)
  var fs = futureStructureSummary(__asym);
  // #5 ONLY SHOW STRIKES THAT MATTER. Instead of a fixed 3-above / 3-below
  // ladder padded with Steady placeholders, surface only nodes a trader needs
  // to act on: those actively Accumulating (Building) or Dissipating (Fading),
  // plus the in-play node and the King even if Steady (structural anchors).
  var inPlayK = (fs.inPlay && fs.inPlay.k!=null) ? fs.inPlay.k : null;
  function matters(r){
    if(r.state.label==='Building' || r.state.label==='Fading') return true; // something is happening
    if(r.role==='King') return true;                                        // structural anchor
    if(inPlayK!=null && Math.abs(r.k-inPlayK)<0.001) return true;            // the in-play node
    return false;                                                            // skip idle/Steady filler
  }
  // Cap each side so a very active tape can't blow the panel out; the vertical
  // scrollbar handles overflow beyond this.
  var rowsAbove = fs.above.filter(matters).slice(0,5);
  var rowsBelow = fs.below.filter(matters).slice(0,5);
  // DISPLAY order = price ladder, highest strike at top descending to lowest.
  // Resistance (above) must render highest-first, so reverse it (774,775,776 ->
  // 776,775,774). Support (below) is already descending (773,772,771). The
  // whole panel then reads: 776 775 774 [SPY] 773 772 771.
  rowsAbove = rowsAbove.slice().sort(function(a,b){ return b.k-a.k; });
  rowsBelow = rowsBelow.slice().sort(function(a,b){ return b.k-a.k; });
  var px = (STATE[__asym]||STATE.SPY).price;
  // ===== MERGED S/R BIAS BLOCK =====
  // #4 (v10.6) Combined into ONE header: "S/R Bias". The board-tilt headline
  // (Support-heavy / Resistance-heavy N\u00d7) rides on the right of this single
  // header \u2014 the separate "\u2696 Bias" sub-header was removed.
  // BOARD = srBattle() (v10.14): a DISSIPATION-DOMINANT support-vs-resistance
  // FORCE model (validated on the user's real tape). Unlike the old
  // netPositioning (which only summed BUILDING nodes and sat green all the way
  // down a grind), srBattle weights the NEAREST level DISSIPATING heavily \u2014 so
  // the bar flips red exactly at a bear-pullback high (floor giving way) and
  // green at a bounce low (ceiling giving way). It also emits a CROSSOVER flag.
  var srb = srBattle('SPY');
  var haveSrb = !!srb;
  var supPct = haveSrb ? srb.supPct : 50;
  var resPct = haveSrb ? srb.resPct : 50;
  var srDir = !haveSrb ? 0 : (srb.dom==='support'?1:(srb.dom==='resistance'?-1:0));
  var netCol = srDir>0?PAL.longAccent:(srDir<0?PAL.shortAccent:PAL.sub);
  // (v10.27) S/R IMBALANCE standalone section REMOVED. Its net-read ("Bearish/
  // Bullish imbalance \u2014 resistance X building, support Y fading") + the tradeable
  // crossover banner are now FOLDED INTO the Node Map header (see nodeMapBlock),
  // per the Step-5 consolidation. srb/haveSrb kept above for downstream reference.
  var html='';

  // #3 (v10.24 Issue I) NODE MAP replaces the abbreviated PROJ row. The Node Map
  // is the reshaped Projected-S/R: a two-sided price-anchored dealer-positioning
  // map (strongest ★ headline + per-level Bounce/Pullback/Break-through verdicts +
  // King marker + travel-emphasis). strongestAccumulator kept for back-compat refs.
  var haveProj = true;
  // (v10.50) legacy "Deflections" section RETIRED from the live render — its history +
  // forward scoring live on in Analysis only (recordDeflections still runs every bar).
  html+=nodeMapBlock();

  return html;   // (v10.27) old two-sided ladder + PROJ block REMOVED — fully superseded by nodeMapBlock() (Step-5 identity). accumBlock is now just the Node Map wrapper.
}
