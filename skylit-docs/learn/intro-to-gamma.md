# Intro to Gamma: Pika, Barney & the Absolute Value Rule (skylit.ai/learn/intro-to-gamma) — captured 2026-08-13

## Nodes (COLOR = SIGN)
- PIKA node = YELLOW = POSITIVE gamma. Dealers net long gamma. Hedge is CONTRARIAN: buy dips, sell rips. => FRICTION (brakes).
  Result: mean reversion, suppressed vol, range/chop, pinning. Tight ranges, frequent retests, low-vol drift.
- BARNEY node = PURPLE = NEGATIVE gamma. Dealers net short gamma. Hedge is PRO-CYCLICAL: buy rallies, sell declines. => FUEL (nitrous).
  Result: vol amplification, acceleration, overshoots, gaps through "support", air pockets, fast directional moves.

| Condition | Dealer hedge | Market behavior |
| +GEX Pika/yellow | buys dips, sells rips | dampening, mean-revert, range, chop |
| -GEX Barney/purple | buys rallies, sells declines | amplification, acceleration, overshoots |

## THE ABSOLUTE VALUE RULE (critical for detectors)
MAGNITUDE matters more than COLOR/SIGN. $2B yellow node dominates $200M purple node.
When reading map: look at LARGEST |nodes| FIRST, assess sign SECOND. The bigger node wins.
A -gamma zone between two +gamma nodes 10x its size will STALL, not break — friction overpowers fuel.
=> King = largest ABSOLUTE exposure (confirms our v10.29 fix).

## Regime transition = best setups
Price crossing from large +gamma zone into -gamma = friction disappears, fuel takes over. Knowing that polarity-cross point
in advance = edge on entry/target.
