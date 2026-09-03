# THE SKYLIT DOCTRINE AS MEASURABLE CLAIMS — and the analysis and capture designed from it

_design/SKYLIT-DOCTRINE-CLAIMS-AND-ANALYSIS-DESIGN.md · 2026-09-03 · read in full: the 11 Academy articles,
the patternpedia (8 pages), core concepts, how-to-read, best practices, pitfalls, limitations, FAQs, case
studies, plus the live pages not in the local capture — the Heatmap API data model, the historical replay
endpoint, the MCP tool catalog, Atlas and its indicators, the changelog. Judged against `design/PURPOSE.md`:
**HOD/LOD deflections first, pullback deflections second, both through gamma nodes.**_

> Skylit's doctrine is unusually testable: it states probabilities, distances, counts and sequences. Read as
> a set of claims it is a **register of hypotheses written by someone else, before our data existed** —
> the strongest pre-registration obtainable. This document turns it into that register, then designs the
> capture that can score it and the analysis that reports it.

---

## PART 1 · THE CLAIMS

Each carries its source, the **unit** it is about, and the **outcome measure** it implies. ⚠ Note how
often the doctrine's claim is about *extent and speed* (how far, how fast) rather than *hit rate*. Our
scoring is almost entirely hit rate. That is the first thing the design changes.

### The node and the tap (execution doctrine · core concepts · node lifecycle)
| # | claim | unit | measure |
|---|---|---|---|
| C1 | **Magnitude overrides pattern and polarity.** Rank by absolute value; the bigger node wins; a small −γ zone between two +γ nodes 10× its size stalls | node | stratify *everything* by magnitude |
| C2 | **The deflection zone** is ±$0.50 on SPY/QQQ, ±5 points on SPX; the King's margin is 5–10 SPX points ("5½ points away is still valid") | tap | the *definition* of a tap — compare with our 1.0 ATR band (3.1 ES pts on 09-02) |
| C3 | **Tap decay: 1st ~80 %, 2nd ~66 %, 3rd ~33 %, 4th+ spent**; the drop 2→3 is severe and non-linear | node × tap# | held rate by tap number — ⚠ our exploratory pass (n=22) said the 2nd test held MORE; H2 |
| C4 | **FRESH → TESTED → DELIVERED → DECAYING.** Delivered = rejected and moved away; a node delivered-from has weakened influence; "graveyard" = tested twice / delivered once | node state at tap | held rate by lifecycle state |
| C5 | **Real vs hedge: a node that GROWS across sessions is intent (target); one that FADES is protection (fade it).** Hedge nodes: far OTM, large, dominant, do not grow | node, day-over-day | held rate by cross-session growth sign |
| C6 | **After a rejection, the delivered-from node GROWING → a revisit is likely; SHRINKING → unlikely** | node after tap | revisit-within-N-bars by post-tap growth |
| C7 | The entry is the **direct tap**; anywhere else is noise | tap | — (a scoring rule, not a claim) |
| C8 | **Positive node → smooth, low-vol interaction; negative node → wicky, overshoots before reversing, traps the wrong side** | tap × polarity | wick depth, overshoot frequency by polarity |
| C9 | The **beach ball**: an overshoot through a positive node that fails to follow through is a stretch, not a break; trade the reversion | tap | close-back-inside rate after overshoot; MFE of the reversion |

### The King (core concepts · how-to-read · reading-heatseeker)
| # | claim | unit | measure |
|---|---|---|---|
| C10 | **The King is the largest \|exposure\|, one per session, the settlement target; pins late (EOD), drives off early** — "reaching the King too early gets pushed away" | King tap × hour | hold/reject rate at the King by session bucket; distance of close from King |
| C11 | When the King MOVES the exposure picture has rotated — pay attention | King change | price follows the new King within N bars? (`dir.kingRoll`, 437 records unread) |
| C12 | Multiple strong nodes pulling opposite ways → pinning or whipsaw | day | range width vs the two largest nodes' gap |

### Structure (reading-heatseeker · gatekeeper · charts-first)
| # | claim | unit | measure |
|---|---|---|---|
| C13 | **Floor = largest node BELOW spot, ceiling = largest ABOVE — not the nearest** | day | which node marked the LOD/HOD: largest-below or nearest-below? |
| C14 | **Gatekeeper** = node between spot and King; holds → stall, breaks → acceleration; **compare its value against the node behind it**; **an early-session gatekeeper rejection often marks a high-probability reversal**; count of gatekeepers = friction | gatekeeper tap | held rate by gk/behind ratio and by hour; MFE after a break |
| C15 | **Air pocket = pathway, not target**: faster, cleaner, weaker reactions inside; "absence of bars IS the signal" | path | MFE/velocity of the move across a pocket vs across nodes |
| C16 | **Never the midpoint** — support halfway between two nodes is imaginary | location | reaction rate at midpoints vs at nodes (a control) |
| C17 | **Charts first: a node at a range extreme or prior reaction level is A+; the same node mid-range is noise** | tap × structural context | held rate at session-range extremes vs interior — ⚠ this IS his HOD/LOD objective in Skylit's words |
| C18 | Speculative/decoy nodes: far OTM, no nodes between, "3× the gatekeeper and 5.5 % away" — treat with skepticism | node | reach rate by distance and intervening-gatekeeper count |

### Motion (rolling floors · air pockets & velocity · map the flow)
| # | claim | unit | measure |
|---|---|---|---|
| C19 | **Rolling floors/ceilings: one migration = noise, two consecutive = signal, three = confirmation**; rolling ≠ breakout; the vacated floor leaves an air pocket | update-to-update | price follows after 2 vs 3 migrations; the vacated strike's later hold rate |
| C20 | **Velocity (rate of change) is fuel; air pocket is space; space + fuel = acceleration; −γ + pocket + high RoC = the fastest moves** | tap | MFE by (RoC × pocket × polarity) |
| C21 | Accumulation → stronger magnet; dissipation → weakening; **a reshuffle is the market changing structure — old levels stop mattering** | node / day | held rate by 15m growth; hold rate of pre-reshuffle nodes after a reshuffle |
| C22 | Stairstepping: delivery from one FRESH node to the next FRESH node; the staircase ends where the next node isn't growing | sequence | next-fresh-node reach rate by its growth |

### Patterns (patternpedia · heatseeker-patterns)
| # | claim | unit | measure |
|---|---|---|---|
| C23 | **Rug: +γ stacked above spot, −γ below, no obvious floor → rejection WITH acceleration**; reverse rug the mirror; "teeth" when −γ sits inside the session's expected range | rejection | MFE after rejection with vs without the configuration |
| C24 | **Pika cloud** (dense +γ cluster) → trend halts, price sticks/rotates; magnitude decides whether it pins all session | cluster tap | dwell time inside the cluster; rejection rate per node in it |
| C25 | Whipsaw = cross-index conflict, fake moves; fade only the extreme ends on the most conflicted index with a FRESH node | day | — needs the trinity field |
| C26 | Trend day: fixation on a far King, stair-step, nodes away from price fade while nodes ahead grow | day | day-type classification vs realised range/efficiency (`EFF`) |
| C27 | Rainbow road: no structure → don't trade | day | realised whipsaw (wick ratio) on rainbow days |

### Confluence and regime (trinity · best practices · gamma regimes)
| # | claim | unit | measure |
|---|---|---|---|
| C28 | **Trinity: 3/3 = A+, 2/3 = size down, divergence = wait; SPX not confirming is more serious than QQQ lagging** | tap × alignment | held rate by alignment count; by which book is the outlier |
| C29 | **GEX/VEX overlap at a strike strengthens it** | node | held rate with vs without vanna at the same strike (we capture vanna) |
| C30 | **Regime: +γ → fade, mean-revert; −γ → follow, momentum; trend day = −γ + rapid accumulation + King growing rapidly + floors rolling** | tap × regime | held rate and MFE by net-gamma sign at spot |
| C31 | Power hour: 3:30 ET forced flow | session bucket | already a bucket |

### Added from the LIVE core-concepts page (2026-09-03 review; not in the 08-13 capture)
| # | claim | unit | measure |
|---|---|---|---|
| C32 | **Rapid unwinding** — exposure vanishing makes a level that looked strong suddenly weak; rapid accumulation is a magnet that pulls hard | node | held rate by rocAtTap SIGN, not just magnitude |
| C33 | **Hedge nodes** appear around macro events (FOMC/CPI/NFP/earnings), are static or slow-unwinding; **the closer to price, the more they shape the day** | node × event flag | held rate of event-day nodes by distance |
| C34 | **A positive-gamma air pocket is mild and slow; a negative-gamma one is violent** — and a pocket is not a guarantee of transit | path × polarity | MFE and bars-to-cross by pocket polarity; transit failure rate |
| C35 | **OPEX week (third Friday): nodes carry less weight; clarity improves immediately after** | day | held rate on OPEX week vs the week after (`isOpexDay` exists) |

### What the doctrine does NOT claim (worth writing down)
- No claim that the wick itself predicts anything — consistent with our F-13 (flat).
- No claim about sweeps, IB60, the 50-SMA — consistent with F-1/F-2.
- GEX VWAP is explicitly "not a price target or a signal."

---

## PART 2 · WHAT THE LIVE DOCS ADD TO THE DATA PROBLEM

**The historical heatmap endpoint.** `GET /v1/historical?at=<RFC3339>` returns the full per-strike book —
`strike`, `value`, and Skylit's own **`nodeType`** (`king`, `gatekeeper`, `pika`, `barney`, `significant`,
`normal`) — at **1-minute resolution, up to 365 days back**. 5 credits per request; new accounts are seeded
with 5,000; live is 1 credit; 600 requests/minute. Also exposed as the MCP tool `heat_historical_heatmap`.
⚠ **This is the answer to the project's oldest constraint.** Every gamma hypothesis has waited on "eleven
sessions." The 284-session ES file spans 2025-06-02 → 2026-08-21; 365 days back from today reaches to
September 2025 — **roughly 240 of those sessions could be given their gamma book.** Not every minute: for
the deflection question we need the book at ~10–15 instants per session (the open, each extreme's print
time, each pullback's turn). That is **~15,000–20,000 credits for the whole corpus**, one-off. The API is
*limited beta*; whether his account has a key is the first question in Part 5.

**Skylit's `nodeType` is an oracle for our detectors.** `skRoles` (King/Gate/Rug), `gatekeeper()`,
`clusterDetect` (pika) and `rugDetect` were built from the prose. One live call per bar tells us whether
they agree with Skylit's own classification. That is a study in itself (S-D6 below).

**Atlas has "Projections (beta): potential forward price-gravity zones" and "Scroll as Replay."** Both are
Skylit's own answers to questions we have been building; worth knowing before we build more.

---

## PART 3 · THE CAPTURE — one TAP record, in the doctrine's own vocabulary

Today the event-level ledger (`day.defl`, one row per fresh tap) carries: strike, direction, distance,
bars, chips, and a 10-bar continuation label. **It cannot score C3–C30 because it does not record the
things the claims are about.** The design: every tap writes ONE record with the fields below, and every
study in Part 4 is a query over those records. Nothing is inferred later that could have been recorded.

```
TAP  (one row per entry of price into a node's zone, per node, per session)
  identity     day · sym · book(SPXW|SPY|QQQ) · strike · tapT · tapBar
  node         value($) · pct(%King) · pol(+/−) · skylitType(if API) · ourRole(King|Gate|Flr|Ceil|Mag|Cluster|Rug…)
               isKing · isLargestBelow/Above (C13) · gkRatio = value / node-behind (C14) · gkCount between spot and King
  lifecycle    tapNo (C3) · state FRESH|TESTED|DELIVERED|DECAYING (C4) · deliveredFromK (last node price left, "delivered from")
  growth       m15 · sinceOpen · dayOverDay sign & size (C5) · rocAtTap (C20) · barsSinceReshuffle (C21)
  structure    posr (C17) · atSessionExtreme (within 1 ATR of running HOD/LOD) · atPriorDayLevel · ibEdge
               midpointFrac (C16) · pocketFar (air pocket on the far side, C15) · nextNodeGap
  configuration rug|reverseRug (C23) · pikaCloud n (C24) · vannaAtStrike (C29) · regimeSign (C30) · dayType (C26/27)
  confluence   triAlign 0-3 · triOutlier book (C28)
  zone         doctrineZone hit (±0.50 / ±5) · atrBandHit (1.0/1.5) — BOTH, always (C2)
  time         sessionBucket · minsSinceOpen · powerHour (C31)
  outcomes     held3m (close back inside) · overshoot (C9) · wickDepth (C8)
               mfe5/mfe10/mfe20 · mae5/10/20 (C15, C20, C23 are about EXTENT)
               followThrough (next N bars fail to extend) · revisitWithin20 (C6) · nodeGrowthAfter15 (C6)
               wasSessionExtreme (labelled at the CLOSE: did this tap print the day's HOD/LOD)   ← the objective
```

Two rules. **Every field is recorded at the tap, from the book as it stood** — never re-derived from a
later snapshot (the v15.00 lesson: a 10:00 high measured against a 16:00 King). **`wasSessionExtreme` is
labelled at the close and nowhere else** — it is the one field that makes the HOD/LOD objective and the
pullback objective the same table with one column of difference.

What this replaces: `day.defl`'s ad-hoc shape, and the per-bar `node`/`reaction`/`defl_ant` feature records
that were three views of one event.

---

## PART 4 · THE ANALYSIS — the studies, by his two objectives, each tied to its claim

Every study: the question (his words or Skylit's), the claim it tests, the unit, the measure, and the
corpus it needs. **S-A = the day's extremes. S-B = the pullback turn. S-D = the nodes. S-C = direction.**

**A · IS THE EXTREME IN — and which node made it?**
| id | study | claim | measure |
|---|---|---|---|
| S-A1 | Which node printed the HOD/LOD: the King, the largest-below/above, or the nearest? | C10, C13 | share of extremes by node role, per book |
| S-A2 | King taps by hour: drive-off before 11, pin after 2? | C10 | hold/reject at the King by session bucket; close-to-King distance |
| S-A3 | Does an early gatekeeper rejection mark the extreme more often than a late one? | C14 | `wasSessionExtreme` rate of gatekeeper taps by hour |
| S-A4 | Is the extreme inside the doctrine zone (±5 SPX) or the ATR band — and which definition is tighter? | C2 | both, side by side |
| S-A5 | Does the extreme print at a session-range edge more than the interior? (Skylit's charts-first, our posr) | C17 | already in `HLTAB`; the node-conditioned version is **H5** |
| S-A6 | Which book's King owns the extremes — SPX, SPY or QQQ? | C28 | share by book |

**B · WILL THIS PULLBACK TURN HERE?**
| id | study | claim | measure |
|---|---|---|---|
| S-B1 | **Tap decay** — Skylit's 80/66/33 against our taps | C3 | held rate by tapNo; **H2 registered** |
| S-B2 | Lifecycle: does a FRESH node hold more than a TESTED one, and a DELIVERED one least? | C4 | held rate by state |
| S-B3 | Real vs hedge: does a node that grew day-over-day hold more than one that faded? | C5 | held rate by dayOverDay sign |
| S-B4 | Growth into the tap: 15-minute accumulation vs flat vs decaying | C21 | held rate by m15; `rollsupport` found growth matters, pairing doesn't |
| S-B5 | A node **born during the pullback** vs an established one | C21 | held rate by age at tap |
| S-B6 | **Rug / reverse rug: is the rejection faster and deeper?** | C23 | MFE10 with vs without the configuration |
| S-B7 | Beach ball: overshoot then close back inside — how often, and what follows? | C9 | overshoot rate; MFE of the reversion |
| S-B8 | Polarity: wickier at −γ? | C8 | wickDepth by pol; held rate by pol (**H3 null**) |
| S-B9 | Gatekeeper ratio: does value-vs-node-behind predict hold vs break? | C14 | held rate by gkRatio bands |
| S-B10 | Air pocket on the far side: is the deflection larger? | C15, C20 | MFE10 by pocketFar × rocAtTap × pol |
| S-B11 | Pika cloud: does it pin, and does magnitude decide? | C24 | dwell; held rate by cluster mass |
| S-B12 | Trinity at the tap: 3/3 vs 2/3 vs split — and is the SPX outlier the bad one? | C28 | held rate by triAlign and triOutlier |
| S-B13 | GEX/VEX overlap at the tapped strike | C29 | held rate with vs without vanna |
| S-B14 | Regime: +γ fade vs −γ follow | C30 | held rate AND MFE by regimeSign |
| S-B15 | After a rejection, does the node's growth predict the revisit? | C6 | revisitWithin20 by nodeGrowthAfter15 |
| S-B16 | The midpoint control: reactions at midpoints vs at nodes | C16 | the control every other row is judged against |
| S-B17 | The rejection wick | — | **H4 null**, flat on 970 rows |

**C · WHICH WAY AFTER THE TURN?**
| id | study | claim | measure |
|---|---|---|---|
| S-C1 | King roll: does price move toward the new King within N bars? | C11 | `dir.kingRoll`'s 437 records, read for the first time |
| S-C2 | Rolling floor: two migrations vs three — does price follow within the session? | C19 | follow rate by migration count |
| S-C3 | Stairstep: after delivery, is the next fresh, growing node reached? | C22 | reach rate by next node's growth |
| S-C4 | Day type (trend / range / whipsaw / rainbow) vs realised range and efficiency | C26, C27 | `EFF` by classified day type |
| S-C5 | GREEN/RED, the open's location, the value area — already measured | — | 77 % one-liner; opens/VA no better than sham |

**D · THE NODES THEMSELVES**
| id | study | claim | measure |
|---|---|---|---|
| S-D1 | How often the crown moves; how long it dwells — measured | C11 | `kingmoves`, `kingdwell` |
| S-D2 | Succession: motion vs stasis — measured | C22 | three passes |
| S-D3 | Hedge bleed: nodes that shrank day-over-day — how often do they vanish, and do they ever hold? | C5 | survival by dayOverDay |
| S-D4 | Reshuffle: do pre-reshuffle nodes stop holding? | C21 | held rate by barsSinceReshuffle |
| S-D5 | Decoy nodes: far, isolated, oversized — reach rate | C18 | reach by distance × gkCount |
| S-D6 | **Our roles vs Skylit's `nodeType`** — do `skRoles`/`gatekeeper`/`clusterDetect`/`rugDetect` agree with the oracle? | — | agreement rate; needs one live API call per bar |

**E · REVIEW** — unchanged: the nightly's verdicts on the register.

**Presentation.** Each study is a card: question · claim it tests (C-number) · corpus (sessions, and whether
the book was present) · measure · result with n and interval, or "thin — not read" · verdict · the script.
Refused studies stay visible. The doctrine's number, where it gives one, is printed beside ours.

---

## PART 5 · RECOMMENDATIONS, IN ORDER — and the decisions that are his

1. **Build the TAP record (v15.55).** It is the capture correction everything else reads; without it
   S-B2–S-B15 are unrecordable. One writer (`recordTap`), one store (`taps` in IndexedDB, exported daily),
   `wasSessionExtreme` labelled at the close.
2. **Register the doctrine's numbers as hypotheses before the tap corpus exists**: C3 (80/66/33), C10
   (drive-off early / pin late), C14 (early gatekeeper rejection → extreme), C23 (rug → larger MFE),
   C6 (growth after delivery → revisit), C28 (3/3 > 2/3; SPX outlier worst), C30 (−γ → larger MFE).
   They are Skylit's claims, dated; scoring them on our taps is the cleanest test this project can run.
3. **Add EXTENT outcomes** (MFE/MAE at 5/10/20 bars, revisit, follow-through) to every scorer. Half the
   doctrine is about how far and how fast, and hit rate cannot see it.
4. **⚠ DECISION — the historical API.** If he has (or can get) an API key: a one-off backfill of the gamma
   book at ~12 instants per session for the 240 sessions the ES file and the 365-day window share, ≈ 15–20 k
   credits. It turns the price-only prior into a price-and-gamma corpus and unblocks H5, S-A1, S-A6 and
   S-B1–B14 at once — years of accumulation in an afternoon. Without it, the plan is unchanged and slower.
5. **Validate our detectors against `nodeType`** (S-D6) before trusting any role-conditioned result.
6. **Keep the two corpora separate on every card** — price-only vs price-and-gamma — as now.

**Decisions for him:** (a) API access — yes/no, and the credit budget; (b) which of S-A1–S-D6 go first
(my order: S-A1, S-B1, S-B6, S-B4, S-B12 — they are closest to how he trades); (c) the tap zone — do we
adopt Skylit's ±0.50/±5 as the primary definition and carry the ATR band beside it, or the reverse.
