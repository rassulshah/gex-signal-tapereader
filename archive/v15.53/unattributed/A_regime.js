// archived at v15.53 (2026-09-03); recovered from git bbd2f1a on 2026-09-04 by tools/recover-archive.py
function A_regime(day, sym){
  sym=sym||'SPY'; var kb=A_kingBehavior(day,sym); var ce=A_combinedEdge(day,sym);
  var snaps=(day&&day.snaps&&day.snaps[sym])?day.snaps[sym]:[];
  if(kb.pts<6) return { label:'Forming', conf:'low', why:'Not enough King bars yet to classify the day ('+kb.pts+' pts).', kb:kb };
  // node dispersion: are heavy nodes concentrated at two edges (Whipsaw) or scattered (Rainbow)?
  var strikes={}; snaps.forEach(function(s){ (s.nodes||[]).forEach(function(nd){ var k=A_num(nd.k),v=Math.abs(A_num(nd.pct)||0); if(k!=null&&v>0){ strikes[k]=Math.max(strikes[k]||0,v); } }); });
  var arr=Object.keys(strikes).map(function(k){return {k:+k,v:strikes[k]};}).sort(function(a,b){return b.v-a.v;});
  var heavy=arr.filter(function(x){return x.v>=20;}); // significant nodes
  var spread=heavy.length? (Math.max.apply(null,heavy.map(function(x){return x.k;}))-Math.min.apply(null,heavy.map(function(x){return x.k;}))) : 0;
  // TREND: net drift dominates rolls (rolls mostly one way) + leading + flow polarity aligns
  var oneWayRolls = kb.rolls>0 ? Math.abs(kb.rollUp-kb.rollDn)/kb.rolls : 0;
  // King-core tightness: how narrow the band of King strikes actually visited is.
  // A tight core (few distinct strikes over a small range) = a defined range even
  // if many heavy nodes exist elsewhere on the board (Whipsaw, not Rainbow).
  var kingRange = kb.coreRange!=null? kb.coreRange : (kb.levels.length? (kb.levels[kb.levels.length-1]-kb.levels[0]) : 0);
  var tightCore = (kb.coreCount!=null?kb.coreCount:kb.distinct)<=4 && kingRange<=4;
  var trending = Math.abs(kb.netDrift)>=2 && oneWayRolls>=0.5 && kb.rolls>=2;
  // WHIPSAW: flip-flop King (balanced rolls) held to a tight core, small net drift.
  // The heavy-node count no longer gates it \u2014 an oscillating King in a tight band
  // IS a range regardless of how many significant nodes sit around it.
  var whipsaw = kb.rolls>=2 && oneWayRolls<0.6 && tightCore;
  // RAINBOW: genuinely scattered \u2014 wide King wander OR wide heavy spread AND no tight
  // core AND poor reach (the doc's "no clear range, vague positioning").
  var rainbow = !tightCore && (kingRange>=5 || spread>=10) && (kb.reachRate==null||kb.reachRate<40);
  var label, conf, why;
  if(trending){ label=(kb.netDrift<0?'Trend \u2193':'Trend \u2191'); conf=oneWayRolls>=0.7?'high':'medium';
    why='King drifted '+kb.netDrift+' via mostly one-way rolls ('+kb.rollUp+'\u2191/'+kb.rollDn+'\u2193) \u2014 leading relocation, the doc\u2019s stair-step trend. Trade pullbacks.'; }
  else if(whipsaw){ label='Whipsaw'; conf='medium';
    why='King flip-flopped ('+kb.rollUp+'\u2191/'+kb.rollDn+'\u2193) in a tight '+(kb.core&&kb.core.length?(kb.core[0]+'\u2013'+kb.core[kb.core.length-1]):(kb.levels[0]+'\u2013'+kb.levels[kb.levels.length-1]))+' core \u2014 a defined range. Fade the edges, avoid the middle.'; }
  else if(rainbow){ label='Rainbow Road'; conf='low';
    why=heavy.length+' heavy nodes scattered over ~'+spread+' strikes with no clear edges'+(kb.reachRate!=null?(' and low King reach ('+kb.reachRate+'%)'):'')+'. Vague positioning \u2014 stand aside.'; }
  else { label='Mixed'; conf='low'; why='No dominant structure: net drift '+kb.netDrift+', '+kb.rolls+' rolls, '+heavy.length+' heavy nodes. Treat as low-conviction.'; }
  return { label:label, conf:conf, why:why, kb:kb, ce:ce, heavy:heavy.length, spread:spread };
}
