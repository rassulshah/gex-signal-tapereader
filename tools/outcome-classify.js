// THREE-WAY OUTCOME CLASSIFIER — the definition every node study should share.
//
// ⚠ WHY THIS EXISTS. The earlier binary ("held" = bounced without trading BREAK through) scored
// PINNING as failure. A King is where price SITS: it oscillates and crosses by small amounts, which
// is the magnet working exactly as doctrine describes. The binary collapsed three distinct behaviours
// into two buckets and produced a result (small nodes beating big ones) that contradicted a separate,
// better-controlled test. Three behaviours, three labels.
//
//   DEFLECT — came, turned, left. The level acted as a wall.
//   PIN     — came, stayed, oscillated around it. The level acted as a magnet.
//   BREAK   — came, passed through, kept going. The level failed.
//   GRAZE   — touched and nothing decisive happened inside the window.
//
// Both DEFLECT and PIN are the level WORKING; they are just different trades (fade vs range).
// Only BREAK is failure. Reporting "hold rate" without this split is what produced F7's bad number.

function classifyTouch(level, side, bars, opt){
  opt = opt || {};
  const TOL   = opt.TOL   != null ? opt.TOL   : 0.50;  // tap window
  const THRU  = opt.THRU  != null ? opt.THRU  : 0.40;  // beyond this counts as through
  const AWAY  = opt.AWAY  != null ? opt.AWAY  : 0.30;  // move away that counts as a deflection
  const PIN   = opt.PIN   != null ? opt.PIN   : 0.35;  // band that counts as "sitting on it"
  const PINF  = opt.PINF  != null ? opt.PINF  : 0.60;  // fraction of bars inside the band
  const isFloor = (side === 'below');

  let hit = -1;
  for (let i=0; i<bars.length; i++){
    if (isFloor ? (bars[i].l <= level + TOL) : (bars[i].h >= level - TOL)) { hit = i; break; }
  }
  if (hit < 0) return null;                      // never reached — not an observation at all
  const r = bars.slice(hit);
  if (!r.length) return null;

  const maxUp   = Math.max(...r.map(b=>b.h));
  const minDn   = Math.min(...r.map(b=>b.l));
  const beyond  = isFloor ? (level - minDn) : (maxUp - level);   // how far THROUGH
  const awayAmt = isFloor ? (maxUp - level)  : (level - minDn);  // how far it recoiled
  const last    = r[r.length-1];
  const endBeyond = isFloor ? (last.c != null ? last.c < level - THRU : minDn < level - THRU)
                            : (last.c != null ? last.c > level + THRU : maxUp > level + THRU);

  // PIN first: time spent inside the band is the defining feature, and it survives small crossings
  const inBand = r.filter(b => (b.l <= level + PIN) && (b.h >= level - PIN)).length;
  if (inBand / r.length >= PINF) return { kind:'PIN', hit, beyond, awayAmt, inBandFrac: inBand/r.length };

  if (beyond > THRU && endBeyond)              return { kind:'BREAK',   hit, beyond, awayAmt };
  if (beyond <= THRU && awayAmt >= AWAY)       return { kind:'DEFLECT', hit, beyond, awayAmt };
  if (beyond > THRU && !endBeyond && awayAmt >= AWAY)
                                               return { kind:'DEFLECT', hit, beyond, awayAmt, overshoot:true };
  return { kind:'GRAZE', hit, beyond, awayAmt };
}
// ⚠ `overshoot:true` is the Beach Ball from heatseeker-patterns.md — punched through a positive node,
// failed to follow through, reverted. Doctrine calls that the node ABSORBING, not failing, and calls
// chasing the break the common mistake. It is a DEFLECT with a flag, never a BREAK.
module.exports = { classifyTouch };
