# THE KING STUDY — why a King holds price, and why it breaks

**His ask, 2026-09-10/11 (verbatim intent):** build "a very robust way of identifying the cause of why the
king held or why the king broke," by scoring a King tap against many factors and letting the record say
which factors actually separate a hold from a break. **Order he set:** first ship the King deflection
*view* (the chart + the ledger + the honest n-gated shell — v15.96, THE KING CHART). Then *enrich* it,
factor by factor, as the tap record accrues the data. This file is the factor backlog so none is lost.

## THE PICTURE (v15.96, THE KING CHART — shipping first)
A 3-minute ES candle chart in the node-ladder section (a view toggle: chart ⇄ the grid). On it:
- **SPX King and SPY King as STEP lines** — from `kingDay('SPXW').moves` and `kingDay('SPY').moves`
  (each `{t,k}` is a step; the line holds a strike then jumps when the King rolls), drawn in ES scale.
- **Deflect / break markers** at each King tap — from the node ledger's King touches (`nodeLedger`,
  the per-node `touches` / `deflect` / `through`, and `day.defl`, one row per fresh tap). ▲ held · ✕ broke.
- **The deflection ledger beside it** — time · which King · held/broke · points.
- Later, a toggle to add the tracked levels and CW0 / PW0 the same way.

## THE FACTORS TO SCORE A TAP BY (the enrichment — his list, extensible)
Each becomes a dimension the tap record splits hold-rate on; a factor earns a rendered rate only at
**n ≥ 15 with a Wilson low**, never a number without its n. Candidate factors (his words + the L-rules):
1. **King ROLL direction** — did the King roll UP or DOWN into / before the tap, and how many strikes
   (rolling 1 = noise, 2 = signal, 3 = confirmation). Does a King that rolled toward price hold harder?
2. **King GROWTH vs DECLINE** — was the King's $ exposure rising (growing into the tap) or fading? (the
   recorder's d15 / m15.)
3. **PIKA / BARNEY STACK support** — are additional high-gamma nodes (each ≥ ~50% of the King) stacked on
   adjacent strikes behind the King, acting as one wall? A supported King vs a lone King.
4. **POLARITY** — +γ King (brake — holds smooth) vs −γ King (accelerator — overshoots, wicky break).
5. **LEVEL CONFLUENCE** — is the King at/near a tracked level (PDH/PDL · ONH/ONL · VAH/VAL/POC · PFH/PFL ·
   CW0/PW0 · weekly)? Confluence with a liquidity level vs a bare gamma King.
6. **TREND vs the 50 MA** — is price above or below the 50-MA (and the trend state)? A King tapped WITH the
   trend vs against it.
7. **TIME OF DAY / THE HOUR** — the open, mid-day, power hour, late day; the specific hour bucket.
8. **ABOVE / BELOW THE OPEN** — is the tap happening with price above or below today's RTH open.
9. **BOOK CONFLUENCE — QQQ + SPY (and SPX)** — do the SPX / SPY / QQQ Kings agree at that price (all
   pointing to the same level)? **His emphasis: confluence of at least one of QQQ/SPY with the King
   "matters a lot."** Two or three books crowning the same price vs one book alone.
10. **DISTANCE / MARGIN** — how far price was from the King at the tap (the doctrine 5–10 pt margin — a
    rejection short of the King still counts).
11. **...more** — open by design; add factors as he names them, each as its own column the record fills.

## THE OUTPUT (built out as data accrues)
- **Hold rate by factor** (the table) — one row per factor, each a Wilson-bounded rate with its n, or
  `recording · n=0` until it fills. The single existing measurement: a tap holds **≈ half** (PROVISIONAL,
  n=4 sessions 08-27·28·31, 09-03 — thin, may not hold); FINDINGS F-21 says accumulating vs dissipating
  did NOT separate holds (PROVISIONAL).
- **A confidence read on the LIVE tap** — the v15.96+ deflection-candidate score: the factor lean 0–100,
  labeled **the factor lean, not a measured probability**, earning a real hit rate only once ≥ 30 taps of
  that shape are scored. Doctrine is the prior; the tape is the arbiter.
- **Insights** — sourced statements (`doctrine` = Academy prior; `provisional`/`measured` = our record with
  n + date + status). The doctrine set: +γ brake / −γ accelerator; the 5–10 pt margin; fresh & growing
  holds more; rolling floor up = bullish / ceiling down = bearish.

## DOCTRINE GATE (per PROJECT-CONSTANTS / SOURCE-OF-TRUTH)
Every factor is checked against the Academy before it is scored: FOLLOWING (doctrine covers it),
DEVIATING (the data says otherwise — legitimate, with numbers/dates), or NO DOCTRINE (get his agreement).
A factor's rate is descriptive until measured; suppressing a measured finding because it fails a
definition is how the system stops learning — detect it, label the confidence honestly, record the outcome.
