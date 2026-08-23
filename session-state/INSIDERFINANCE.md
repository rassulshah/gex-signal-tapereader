# INSIDERFINANCE — WHAT WE TAKE FROM THEM, AND WHAT IT COSTS US

**Read this alongside the Skylit sections of the skill. The panel stands on TWO books and confusing them
has caused four separate bugs in this project. If you only understand one of them you will make a fifth.**

Verified against the live page 2026-08-23 (`insiderfinance.io/gamma-exposure/SPX`, payload stamped
2026-08-21T20:04:55Z).

---

## 1. WHAT THE SITE IS

`https://www.insiderfinance.io/gamma-exposure/<TICKER>` — a Next.js **statically generated** page. We use
`SPX` (and `QQQ`). Two entirely different things live on it and we consume BOTH:

| | what it is | how we read it | trust |
|---|---|---|---|
| **the rendered header** | their own computed levels, printed as text | scraped by `hdrText()` / `hdrNum()` | THEIRS — quoted, never recomputed |
| **`__NEXT_DATA__`** | the raw option chain the page was built from | `JSON.parse(#__NEXT_DATA__)` | OURS — we recompute everything from it |

**The chain is the valuable part.** Their header gives ~9 numbers; the embedded payload gives the whole
book and lets us compute any window we like.

---

## 2. THE PAYLOAD SHAPE (verified live)

    __NEXT_DATA__.props.pageProps.initialData = {
      ticker:        "SPX"
      tickerDetails: { ticker, name, sector, industry, marketCap, exchangeShortName, ... }
      spot:          7674.1
      options:       [ 24,804 rows ]
      timestamp:     "2026-08-21T20:04:55.319588576+00:00"
      isStale:       false
    }

Each option row — **exactly these eleven fields, nothing else**:

    { strike, expireYear, expireMonth, expireDay, cp:"C"|"P",
      gamma, delta, openInterest, impliedVol, bid, ask }

**55 distinct expiries** in one payload. `gamma` is per-contract, `openInterest` is contracts.
⚠ There is **no vega and no theta**. Any vanna/charm read must come from Skylit, not here.

---

## 3. WHAT WE COMPUTE FROM IT, AND THE ONE FORMULA

    GEX = gamma × openInterest × 100 × spot² × 0.01      puts NEGATIVE

**VERIFIED to the decimal** against their own published header (all expiries, 2026-08-22):

    ours   call +263.83B   put −250.49B   net +13.34B
    theirs      $263.8B         −$250.5B        (page, live)

That reconciliation is the whole licence for recomputing rather than scraping. **If you change this
formula, re-verify against the header before shipping.**

### The windows (`windows()` in the companion)

| window | filter | what it is |
|---|---|---|
| `dte0` | nearest expiry ≥ today | the near book |
| `toFri` | today ≤ expiry ≤ Friday of this week | the week |
| `all` | everything | 55 expiries |

⚠ **ON A FRIDAY `toFri` ROLLS TO NEXT FRIDAY** (`rolled:true`), or it would be a second copy of `dte0`.
After the roll it spans today + a whole extra week — measured 2026-08-21, only **29.2%** of its gross gamma
expired that day. This is why the FRAME piles were moved to `dte0` at v11.68.

⚠⚠ **`dte0` IS NOT ALWAYS TODAY.** Their payload DROPS an expiry once it has expired. Captured after the
Friday close, the earliest expiry in the chain is **Monday**:

    payload ts   2026-08-21T20:04:55Z  (after the 16:00 ET close)
    today        20260821
    earliest     20260824              ← Friday's chain is already gone
    dte0 selects 20260824              dte0_isToday = FALSE

So on a weekend replay of Friday, the band's "today's expected move" is **Monday's** straddle. During live
RTH on a weekday the row exists and `dte0` really is today. **The label says dte0 and means "nearest live
expiry", which is not the same claim.** Nothing is wrong with the arithmetic; the wording over-promises.
Open item — see DECISIONS.md D-5.

### The sign flips by window, and all three are correct

    dte0   −$6.86B      toFri  −$16.41B      all  +$13.34B

⚠ **NEVER compare a number from one window against a number from another.** Two separate false alarms in
this project came from exactly that — a "netGEX sign bug" and a "regime contradicts FLIP" — both were me
comparing our `toFri` slice against their all-expiry total.

---

## 4. WHAT WE TAKE FROM THEIR HEADER (quoted, not recomputed)

Read by `hdrText()` / `hdrNum()`, gated by `levelSane()` (a price level must sit within 0.5×–2× of spot):

    Zero Gamma  $7647.89     → our FLIP
    Call Wall   $7900        ⚠ ALL-EXPIRY (flagged wallsAreAllExpiry)
    Put Wall    $7500        ⚠ ALL-EXPIRY
    Max Pain    $7350        ⚠ ALL-EXPIRY — and NOT what our ladder shows
    Skew / Skew Slope / Term Slope / ATM IV / Put-Call Ratio

⚠ **`MP*` ON OUR LADDER IS NOT THEIR MAX PAIN.** Ours is recomputed over OUR window (7712.70 on 2026-08-23);
theirs is 7350 across all expiries. The asterisk is load-bearing — v11.75 added it for this reason.

⚠ **THE SCRAPE HAS BITTEN US.** An ordered regex alternation once matched `764` inside `7646.90` and
rendered Zero Gamma as **764** beside a spot of 7674. The test passed because its fixture used
`$7,646.90` WITH a comma while the page renders it without one. **Take scrape fixtures from the real
artefact, never from what you think the page looks like.**

---

## 5. THE COMPANION (`current/gex-if-levels.user.js`)

A SECOND userscript, currently **v1.13**, that runs on the InsiderFinance tab and posts levels across to
the panel. It exists because the Atlas page cannot fetch insiderfinance.io itself.

- `levelsFor(options, spot, filterFn)` walks the chain once per window.
- `gexProf` (v1.12) — the per-strike profile the FRAME piles are drawn from:
  `[strike, callGEX $M, putGEX $M]`, puts negative.
- `gexProfCoverage` (v1.13) — the fraction of gross gamma the profile retains. A **1% tail trim** drops
  roughly **5%** of a live 780-strike chain. ⚠ The profile is NOT the whole book: do not diff it against
  `callGEX`/`putGEX` and call the gap a bug. Live coverage runs ~92–96%.
- ⚠ **The companion had NO `@updateURL` until v1.11**, so Tampermonkey could never offer it an update and
  a broken scrape survived reloads for days. Both scripts carry update URLs now — check this first if a
  fix "doesn't take".

---

## 6. HOW THE PANEL DEPENDS ON IT — every FRAME element, by source

    ① FRAME
    −G −V ⚠ regime chip .......... SKYLIT          LASTFEED / LASTVEX, SPY tape
    BREAKS / FADES ............... SKYLIT          derived from the chip
    FEEDS $214 M / PT ............ INSIDERFINANCE  toFri netGEX ÷ (spot × 1%)
    ES $1,736/ct ................. INSIDERFINANCE  dte0 straddle × contract multiplier
    T: 7718 ...................... INSIDERFINANCE  heaviest strike (Mag)
    EXP LOW / EXP HIGH ........... INSIDERFINANCE  open ± dte0 ATM straddle
    the piles on the rail ........ INSIDERFINANCE  dte0 gexProf, net-polarised
    the read sentence ............ INSIDERFINANCE  composed from the piles
    FLIP ......................... INSIDERFINANCE  their published Zero Gamma

**One chip from one book; everything else from the other.** See DECISIONS.md D-4.

**Scale chain — three scales, and mixing them is the classic error:**

    k     SPX strike           7700
    disp  chart / ES price     7700 × dispScale (1.0023) = 7717.71
    und   SPY                  × undScale (0.099773)

`rr = dispIsFut() ? dispR() : 1` converts und→chart and **is live — it drifts with basis**. The band's
anchor pins the `rr` it was captured at, because a live `rr` walked the anchor 18 points (v11.59–v11.65,
four separate causes, one of them two of my own fixes fighting).

---

## 7. FAILURE MODES — what breaks, and what it looks like

| failure | symptom on the face | how to confirm |
|---|---|---|
| tab closed / companion not running | levels stale, `ageMin` climbs | `__gptsDebug.if()` → `state.t`, `fails` |
| payload stale | `isStale:true`, `chain.stale` | `__gptsDebug.ifChain('SPX').stale` |
| page markup changes | a level scrapes wrong or vanishes | `levelSane()` rejects; `pubSrc` says `header` vs computed |
| **chain absent entirely** | ⚠ **`emPiles` returns `[]` and the sentence says "Nothing sizeable between X and Y"** | nothing on the face distinguishes it — **OPEN DEFECT, see DECISIONS.md D-6** |

⚠ **That last row is the dangerous one.** Six distinct failures inside `emPiles` all return an empty array,
and an empty array renders as a clean path. **Absence of data reads as absence of obstacles.**

---

## 8. THE ONE THING THAT IS NOT THEIRS AND MUST NOT BECOME THEIRS

Their page shows a **0DTE / Weekly / Monthly expiry mix** and a Strike Profile. We do not scrape those —
we recompute from the chain so the window is ours to control. Keep it that way: their rendered aggregates
answer a question we did not ask, and every time this project has taken a shortcut through their header
instead of the payload, it has had to be undone.
