# SPEC (PARKED) — GAMMA LEVELS FOR THE COMMODITY & FX FUTURES

**Status: PARKED, fully researched, not built.**
Operator, 2026-09-01: *"I want you to hold this implementation detail somewhere, maybe in a roadmap
document. We will come back to it once the application with the current markets is optimal."*

> ⚠ **DO NOT START THIS BUILD** until the ES/SPX/SPY/QQQ panel is where he wants it. This file exists
> so the research is not repeated, not so the next context starts coding. Everything below marked ✅
> was **measured live on 2026-09-01**, from his own browser and his own companion store. Everything
> marked ⚠ is an unknown or a trap. **The unknowns are the valuable part of this file.**

---

## 0 · WHAT HE ASKED FOR, AND HOW THE ANSWER CHANGED

The original ask (2026-09-01) was an **ETF conversion**: take FXE, GLD, USO and UNG from
InsiderFinance, convert each to its futures contract, and add the 0DTE levels to the IRT export.

**The research changed the answer, and he approved the change.** The actual futures options exist,
are reachable free, and are the *right book* — the ETF conversion was a compromise nobody needed to
make. What killed it:

    ETF    nearest expiry available     verdict
    GLD    Sep 1 · 0DTE            ✅   the only one with a daily expiry
    USO    Sep 2 · 1DTE            ⚠    no 0DTE, and USO's ratio to CL drifts daily from roll decay
    UNG    Sep 2 · 1DTE            ⚠    same, worse; UNG publishes no zero-gamma at all
    FXE    Sep 18 · 17DTE          ❌   MONTHLY ONLY. A 17-day wall is not an intraday magnet.

Against that, the real CME books carry **Monday-through-Friday weeklies** on CL, NG and GC. The ETF
route reads the wrong book at worse expiries. **It is not a fallback worth keeping** — if the
futures path fails, the honest output is no rows, not ETF-derived rows wearing a futures label.

⚠ THE ONE THING THE ETF WORK PROVED AND IS WORTH KEEPING: if a conversion is ever needed again, the
ratio must be **measured live every session, never constanted**. Measured 2026-09-01 from his own
companion store: `GC/GLD = 11.0278`, `CL/USO = 0.64103`. USO's drifts continuously with the roll.

---

## 1 · WHERE THE DATA COMES FROM — three URLs, all Barchart, all free, no login ✅

**Neither Skylit nor InsiderFinance has any of it.** Measured on his account 2026-09-01: Skylit's
`/tv/api/gex/levels` returns HTTP 200 with **zero snapshots** for `GC1`, `GCZ26`, `CL1`, `NG1`,
`6E1` — **and for `ES1`** — while `SPY` returns 390 snapshots across 50 strikes. Skylit charts
futures but computes GEX only from equity/ETF/index chains. InsiderFinance is equity/ETF-only too.

    1  barchart.com/futures/quotes/<ROOT>*0/overview        WHICH CONTRACT   (see §2)
    2  barchart.com/futures/quotes/<CONTRACT>/options       OPEN INTEREST per strike
    3  barchart.com/futures/quotes/<CONTRACT>/volatility-greeks   GAMMA per strike

Pages 2 and 3 are the same strikes with different columns; **join on strike**.

⚠⚠ **TWO PAGE SETTINGS ARE LOAD-BEARING AND BOTH DEFAULT WRONG:**

    Options Type  →  Friday/Monday/…Weekly + "Week 1"      NOT "Monthly Options"
                     Monthly gave 09/17 — sixteen days out, the wrong book entirely.
    Moneyness     →  "Show All"                            NOT the default "Near the Money"
                     ⚠ A gamma flip computed off a TRUNCATED chain is wrong, and wrong in a way
                       that looks completely fine. This is the single most dangerous default here.

**Live proof it carries real data** — CLV26 Friday Weekly, Week 1, **3 days to expiry (09/04/26)**:

    strike    volume    open int
    88.50C         9          78
    89.00C       113         371
    90.00C       220         353
    92.50C         2       1,432

---

## 2 · ⚠⚠⚠ THE CONTRACT RESOLVER — AND THE TRAP IT CAUGHT ON ITS FIRST RUN

Operator: *"i need you to have checks to always know the current contract."* The check earned its
keep before a line was written.

**Barchart has two continuation notations and they are NOT the same thing:**

    <ROOT>*1   = NEAREST BY DATE
    <ROOT>*0   = MOST ACTIVE      ← the only correct one

    root   *1  (nearest)      *0  (most active)      his DTN ticker     which is right
    CL     CLV26  Oct '26     CLV26  Oct '26         CLEV26             either
    NG     NGV26  Oct '26     NGV26  Oct '26         NGEV26             either
    E6     E6U26  Sep '26     E6U26  Sep '26         E6U26              either
    GC     GCU26  Sep '26 ❌  GCZ26  Dec '26 ✅      GCEZ26             ONLY *0
    HG     HGU26  Sep '26 ❌  HGZ26  Dec '26 ✅      CPEZ26             ONLY *0

⚠ **"Front month" is the obvious rule and it is WRONG for the metals.** September gold and September
copper are nearly dead; the traded month is December. `*0` matches all five of his chart symbols
exactly. `*1` would have drawn gold and copper levels on a contract he does not trade — and it would
have looked completely normal on the chart.

**The contract code comes out of the `<h1>`**: `Gold Dec '26 (GCZ26)`. Page 1 is plain server HTML,
so it is fetchable with `GM_xmlhttpRequest` directly — unlike pages 2 and 3 (see §4).

### 2b · THE FREE CROSS-CHECK — a price fingerprint, not a second scrape ✅

The companion already holds Yahoo `=F` bars. **Yahoo's `=F` tracks the ACTIVE contract**, so it is an
independent second opinion that costs no extra request:

    GC*1 (Sep, the wrong one)   4,328.0
    Yahoo GC=F                  4,375.70     ← agrees with *0 (Dec), $47 away from Sep
    CL*0                        90.82
    Yahoo CL=F                  90.68        ← agrees

⚠ **THE RULE: if the resolved contract's price disagrees with the tape beyond a tick or two, SAY SO
AND DRAW NOTHING.** A $47 gap is what a wrong contract looks like. Do not average, do not prefer one,
do not "fall back" — disclose. (Lesson v15.35: a disclosure that lies is worse than no disclosure.)

⚠ `NG=F` and `6E=F` are **NOT in `FUT_MARKETS` yet** (currently ES, NQ, GC, CL). Without them those
two markets ship with no cross-check at all. Add them before, not after.

### 2c · ⚠⚠ TWO ROLLS, NOT ONE — never collapse them

    the FUTURES contract roll   sets the CHART SYMBOL   CL Oct future trades into late September
    the OPTION EXPIRY roll      sets the LEVELS         CL Oct options DIE mid-September

Collapsing them draws a dead chain's levels on a live chart. Same class as the King track's expiry
roll (v15.14): *"that is bookkeeping, not a move."* **Every roll is announced, never absorbed.**

---

## 3 · THE MARKETS — one entry each

### 1. GC — Gold → `GCEZ26`
- Resolver `GC*0` → **GCZ26 (Dec '26)** ✅ · ⚠ `GC*1` gives GCU26 (Sep) — **wrong**
- Cross-check: Yahoo `GC=F` 4,375.70 agrees with Dec ✅ (already in `FUT_MARKETS`)
- Gamma ✅ / Open Int ✅ free
- ⚠ **UNVERIFIED: gold's near-dated WEEKLY liquidity.** The Z26 *monthly* is 84 DTE, so 0–1DTE has to
  come off weeklies whose OI was never priced. **Check this first when the build starts.**
- DTN `GCE` + `Z26` · tick 0.10 · multiplier 100 oz

### 2. CL — Crude Oil WTI → `CLEV26`   ⭐ START HERE
- Resolver `CL*0` → **CLV26 (Oct '26)**, last 90.82 ✅
- Expiries ✅ Monthly, European-Style, Average Price, Calendar Spreads, **Mon/Tue/Wed/Thu/Fri weeklies**
- Chain ✅ measured live at 3DTE with real traded OI (see §1), strikes on 0.25
- Cross-check ✅ Yahoo `CL=F` 90.68 vs 90.79 page
- DTN `CLE` + `V26` · tick 0.01 · multiplier 1,000 bbl
- **Best-covered market of the set. Prove the whole pipeline on CL alone before extending.**

### 3. NG — Natural Gas → `NGEV26`
- Resolver `NG*0` → **NGV26 (Oct '26)** ✅
- Expiries ✅ Monthly, European-Style, Friday Weekly European-Style, **Mon/Tue/Wed/Thu/Fri weeklies**
- Gamma ✅ / Open Int ✅
- ⚠ `NG=F` missing from `FUT_MARKETS` — no cross-check until added
- DTN `NGE` + `V26` · tick 0.001 · multiplier 10,000 MMBtu

### 4. E6 — Euro FX → `E6U26`
- ⚠⚠ **Barchart's root is `E6`, NOT `6E`.** `6EU26` returns a 404 — an easy way to build the entire
  feature against a dead symbol and never see data.
- Resolver `E6*0` → **E6U26 (Sep '26)** ✅
- ⚠⚠ **NO USABLE 0DTE.** The weeklies are LISTED but NOT TRADED — the Oct 2 Friday weekly on E6Z26
  returned **N/A in every field**. The real book is the front serial: **3 DTE (09/04) as measured**.
  Listed ≠ traded, and the dropdown will happily show you an empty product.
- ⚠ `6E=F` missing from `FUT_MARKETS`
- ⚠ DTN root `E6` has **no `E` suffix**, unlike GCE/CLE/NGE/CPE — see §3b
- Tick 0.00005, five decimals · multiplier 125,000 EUR

### 5. HG — Copper → `CPEZ26`   (operator has it on his ticker list; inclusion never confirmed)
- Resolver `HG*0` → **HGZ26 (Dec '26)**, last 6.5385 ✅ · ⚠ `HG*1` gives HGU26 (Sep) — **wrong**
- ⚠⚠ **THE DTN ROOT IS `CPE`, NOT `HGE`.** Barchart and CME both say `HG`; his chart says `CPE`.
- ⚠ **UNVERIFIED:** weeklies, and gamma/OI coverage. Cheap to check.
- Tick 0.0005 · multiplier 25,000 lb

### 3b · ⚠⚠ THE ROOT MAP IS A HAND-WRITTEN TABLE, NEVER DERIVED

    exchange root   DTN root      note
    CL              CLE
    NG              NGE
    GC              GCE
    HG              CPE           ⚠ NOT HGE — different letters entirely
    E6              E6            ⚠ no 'E' suffix at all

Four of five take a trailing `E`; copper changes letters and euro takes nothing. **Any clever rule
derived from the exchange symbol breaks on copper and euro.** Write the table. This is the same
failure this project keeps finding: a value used outside the assumption it was created under.

---

## 4 · ⚠⚠⚠ THE DELIVERY OBSTACLE — the chain is NOT in the raw HTML

**Measured 2026-09-01:** fetched `CLV26/volatility-greeks` server-side, exactly as the companion
fetches everything. **466,227 bytes of HTML, and `"strike":` appears ZERO times.** (`"gamma":` and
`"openInterest":` appear once each — column definitions, not data.) The tables are loaded by a
client-side call after the page renders.

**So `GM_xmlhttpRequest` on those URLs returns a page with no data in it.** Page 1 (§2) is fine;
pages 2 and 3 are not.

Three ways round, **operator picked A** (2026-09-01):

**A · A SECOND USERSCRIPT THAT RUNS ON `barchart.com`.** ← RECOMMENDED AND CHOSEN
Reads the table already rendered on screen, hands it to the panel with `GM_setValue` (Tampermonkey
shares that across tabs; `localStorage` cannot — it is per-origin). Same posture the companion
already has with InsiderFinance: read what is displayed. No credentials, no private endpoints.
**Cost: one Barchart tab open, once a day.** ⚠ And once a day is genuinely enough — see §5.

**B · Call Barchart's internal JSON API.** Works, but it is a private endpoint behind a cookie
token — a real step beyond reading a public page, and it breaks the day they rotate it. **Rejected.**

**C · Manual CSV download.** Free account, the `download` button, five files a day into a watched
folder. Unambiguous but it is a daily chore that stops happening by Thursday. **Rejected.**

⚠ **The free tier throttles.** Confirmed empirically — a signup wall dropped over the page after
roughly eight to ten views, twice. A free Barchart account raises the limit and appears to enable
the download button. §5 explains why this never binds.

---

## 5 · ⚠⚠ THE DESIGN INSIGHT THAT MAKES ALL OF THIS CHEAP

**OPEN INTEREST IS PUBLISHED ONCE A DAY BY THE EXCHANGE. THERE IS NOTHING TO POLL.**

This is true for everyone, including the paid vendors — it is exactly why MenthorQ's futures levels
are **EOD** while their stock/ETF levels are intraday. The intraday movement in the SPX walls comes
from estimating same-day flow on top of yesterday's OI, not from fresh OI.

Consequences, all good:
- One pull per contract shortly after the open is not a compromise, it is the **correct** cadence.
- 5 contracts × 2 pages × 1 pull ≈ 10 page views/day — inside even the un-registered free limit.
- A level set fixed for the session is close to the practical ceiling for these products, free or
  paid. ⚠ **Do not build a poller.** It buys nothing and trips the throttle.

---

## 6 · THE COMPUTATION

    per strike:   GEX = gamma × open interest × contract multiplier
    CALL WALL  =  heaviest CALL-gamma strike
    PUT WALL   =  heaviest PUT-gamma strike
    FLIP       =  where cumulative net gamma crosses zero

Same computation the panel already runs on the SPX ladder — one quantity, one source.

⚠ **THE MULTIPLIERS DIFFER WILDLY** — GC 100 oz, CL 1,000 bbl, NG 10,000 MMBtu, 6E 125,000 EUR,
HG 25,000 lb. This does **not** move a wall (argmax is scale-invariant) but it means the dollar GEX
figures are **NOT comparable across markets**. Label them per-market, or a $ number will quietly
imply gold and natural gas sit on one scale.

---

## 7 · WHAT THE BUILD LOOKS LIKE, WHEN IT HAPPENS

    barchart reader userscript   NEW · runs on barchart.com · reads rendered tables · GM_setValue
    companion                    + NG=F and 6E=F in FUT_MARKETS (cross-check)
                                 + <ROOT>*0 overview fetch (plain HTML, GM_xmlhttpRequest is fine)
    panel                        + resolver + price fingerprint + roll disclosure
                                 + the hand-written DTN root table (§3b)
                                 + GEX computation (§6) and CW/PW/FLIP rows in the IRT CSV
                                 + a BC lamp beside IRT · IF · YF · FF
                                 + a deps item, a test case, and a DEPENDENCIES.md section
                                   (the four-step rule that file now enforces)

**Order: CL alone, end to end, first.** Then NG, GC, E6, and HG last (least verified).

### ⚠ THE UNKNOWNS, LISTED SO THEY ARE NOT DISCOVERED LATE

1. **Gold's near-dated weekly OI** — never priced. GC may be monthly-only in practice, like the euro.
2. **Copper entirely** — weeklies and gamma/OI coverage unverified.
3. **Whether the operator wants copper at all** — asked twice, never answered.
4. **Whether a free Barchart account changes the rendering** — the reader may not need one.
5. **How the reader userscript signals "the page is showing the wrong expiry"** — it reads whatever
   the tab happens to display, so the two settings in §1 are a correctness dependency on a HUMAN.
   ⚠ That is the weakest link in design A and it needs an explicit guard: the reader must publish
   the expiry and moneyness it actually saw, and the panel must REFUSE a chain that is not the
   expiry it asked for. Do not trust the tab.

---

## SOURCES

- Barchart contract resolver, chains and greeks — https://www.barchart.com/futures/quotes/CLV26/volatility-greeks
- MenthorQ futures gamma (CME/NYMEX/CBOT/COMEX, EOD) — https://menthorq.com/quantitative-model/gamma-levels-on-futures/
- MenthorQ Bookmap integration (the EOD-for-futures statement) — https://menthorq.com/guide/bookmap-integration/
- Barchart OnDemand `getFuturesOptions` (paid API alternative) — https://www.barchart.com/ondemand/api/getFuturesOptions
- CME volume & open interest — https://www.cmegroup.com/market-data/browse-data/exchange-volume.html
