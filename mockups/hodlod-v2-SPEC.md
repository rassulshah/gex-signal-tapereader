# ⓪a DAY — HOD/LOD · the approved mockup, transcribed

_Source: the operator's approved v2 mockup._

⚠⚠ **CORRECTION, 2026-08-27 evening: THE ORIGINAL HTML WAS NEVER LOST.** This file was written in the
belief that "Mockup hodlod v2" had only ever been offered as a download and never saved. It is in the
repo, at the ROOT rather than in `mockups/`, and it is on GitHub:

    mockuphodlodv2.html      8,397 bytes   ← THE APPROVED DESIGN, renders clean, 0 page errors
    mockuphodlodv1.html      7,979 bytes   ← the prior draft
    mockuphodlod.html        7,003 bytes   ← n=4 proposal, the earliest
    mockuphodlod (1).html    7,117 bytes   ← the 284-session version of the same

Two earlier notes said these were gone. Both were wrong, and both were **failure pattern #4 —
concluding "absent" from a shallow look**, in this case not searching the repo root. Render
`mockuphodlodv2.html` before working from this transcription: the HTML is the artefact, this file is
a description of it.

⚠ What IS genuinely missing is the EVIDENCE, not the design — see the section at the end._

⚠ **THE NUMBERS BELOW ARE THE MOCKUP'S, NOT MEASURED BY THIS CONTEXT.** They came from a prior
session's study over the same 284-day corpus. Every one is a claim to be re-derived before it ships.

---

## LAYOUT — what the picture actually shows

Header: `⓪a DAY — HOD/LOD · 10:26 CT · 284d ES 1-min`

The section sits **above ② TREND**, and inside it the **stats table is on top and the READ box is
underneath** — which is the operator's ask, verbatim: *"give me this in the read section under the
stats."*

### Block 1 — the two extremities, actual vs expected

Two rows, `A` (actual, today) over `E` (expected, the 284-day median), so every live number is read
against its own base rate on the line beneath it.

    1ST        TOOK    BOP     WICK    W.END    WICK%   MUD
    A  LOD 09:12  42m    18m     1h00    09:30    31%     56m…
    E  — 08:51   ~21m    ~6m     ~34m   ~09:04   ~21%    ~3h19

    2ND        HL GAP          HL RNG
    A  HOD pend.  1h14…        $1,050 — 21.0pts
    E  13:22     ~3h52         ~$2,800 — 56pts (42–80)

Columns, as far as the render defines them:

| col | meaning |
|---|---|
| `1ST` / `2ND` | which extremity printed first, and its clock time. `HOD pend.` = the second one has not printed yet. |
| `TOOK` | how long the extremity took to form |
| `BOP` | time spent beyond the opening print |
| `WICK` | duration of the wick at the extreme |
| `W.END` | when the wick ended |
| `WICK%` | the wick as a share of the move |
| `MUD` | time spent in the middle, going nowhere |
| `HL GAP` | elapsed between the two extremities |
| `HL RNG` | the day's range, in dollars AND points, with the expected inter-quartile band `(42–80)` |

The `~` prefix on every `E` value marks it as a median, not a measurement of today. `…` marks a
value still running.

### Block 2 — the elapsed-time ladder

Five bars, one per holding window, each labelled with the rate that the standing extremity survives
that long. The **active** tier is filled green and marked `◂`:

    42%      54% ◂     66%      75%      84%
    30m      60m       90m      2h       3h

Read: *the longer the low has stood, the likelier it is the low of the day* — and the panel says
exactly how much likelier at each step rather than implying a cliff.

### Block 3 — THE READ (green box, under the stats)

    LOD IN — 84%  (5/5 · stood 1h14 · n=45) · MARKUP toward HOD
    ride left med +24 (p25 +11) · other extremity printed later on 43/47 of these,
    usually ~13:22 · when this tier missed, price ran med −12 past the LOD
    [OPEN ✓] [VWAP ✓] [SWP ✓] [IB60 ✓] [POS ✓]                              5/5

Grammar, in order:
1. **The verdict** — `LOD IN` / `HOD IN`, with the probability.
2. **The evidence in parentheses** — confirming chips `5/5`, how long it has stood, and **`n=45`,
   the sample behind THIS combination**, not the corpus size.
3. **The consequence** — `MARKUP toward HOD`: if the low is in, the trade is toward the other
   extremity. This is the operator's actual question — *"determine if a lod or hod is in and we are
   going to the other extremity."*
4. **The size of the ride** — median `+24`, with `p25 +11` so the 25th percentile is visible and the
   median is never mistaken for a promise.
5. **What happens NEXT and WHEN** — the other extremity printed later on `43/47`, usually `~13:22`.
6. **⚠ THE MISS CASE, STATED** — *"when this tier missed, price ran med −12 past the LOD."* The cost
   of being wrong is printed beside the odds of being right. This is the single best thing in the
   mockup and it must survive to the build.

**The five chips are the factor gate**: `OPEN` · `VWAP` · `SWP` (sweep/liquidity grab) · `IB60` ·
`POS` (position in range). Each ✓/✗, tallied `5/5` at the right.

⚠ **IB60 IS ONE OF THE FIVE AND THE CODE HAS NO IB60.** `sessionLevels()` computes IB30 only
(`IB_MIN_S=1800` → `ibH`/`ibL`/`ibSet`) plus PDH/PDL/PDC. IB60 is net-new work, and the operator's
question — *"did you consider combinations that include IB30 breaks and IB60 breaks"* — is asking
for the sweep to test both.

### Footer — the honesty line

    seq 48/52 coin-flip · every rate carries its n · half-split stable · descriptive — no entries/stops

Four separate disclosures, and each one is load-bearing:
- **`seq 48/52 coin-flip`** — bare sequence (which extremity printed first) is 48/52, i.e. nothing.
  The section says so on its own face rather than letting the reader assume order carries edge.
- **`every rate carries its n`** — no bare percentage anywhere.
- **`half-split stable`** — the rates held on both halves of the sample. This is the walk-forward
  claim and it is the one most in need of re-derivation.
- **`descriptive — no entries/stops`** — the standing scope boundary, unchanged.

---

## WHAT THIS COMMITS US TO

- Rates come from **284 days of ES 1-min price**, not from the panel's 3 usable gamma days. The two
  evidence bases are different sizes by two orders of magnitude and must be **scored separately and
  labelled**, never fused into one number.
- Every displayed rate needs `n` **for that exact combination**, which is what makes a 5-factor gate
  expensive: the cells thin out fast. `n=45` on a 5/5 gate out of 284 days is the shape to expect.
- The miss case is not optional garnish. It is a column of the deliverable.
- Under FEATURE ENROLLMENT this cannot ship as a display: it needs a `FEATURES` entry
  `{key,label,record,outcome,fwd,questions,rule}` so it is recorded per bar, scored nightly, and
  graduates `⚖ → 📊` only at eff n ≥ 20 with walk-forward sessions behind it.


---

## ⚠⚠ WHAT IS ACTUALLY BLOCKING THIS BUILD (2026-08-27)

**The design is safe. The data behind it is not.**

| artefact | state |
|---|---|
| `mockuphodlodv2.html` — the approved design | **PRESENT**, repo root, on GitHub |
| `data/es-1min/EPM26-1min.csv.gz` — 284 RTH sessions of ES 1-min | **GONE.** Committed in the sandbox 2026-08-27 (`a26cdfd`), never pushed, sandbox copy lost |
| "Hodlod stats2" JSON — the measured rates behind every number on the mockup | **GONE.** Offered as a download, never saved |

**EVERY FIGURE ON THAT MOCKUP CAME FROM A STUDY OVER THAT CORPUS** — the 84%, the `n=45`, the 43/47,
the median +24 and p25 +11, the −12 miss case, and the whole 42/54/66/75/84 elapsed-time ladder.
Without the corpus they cannot be re-derived, and this project's standing rule is that nothing ships
as a displayed rate without its n and its date. **Building the section against numbers we cannot
reproduce would be shipping a confident wrong answer** — the exact thing PROJECT-CONSTANTS warns
about. So the section is BLOCKED on the operator re-supplying the ES 1-min CSV, and on nothing else.

⚠ **WHY THE FILE CANNOT RIDE THE INSTALLER BACK.** The payload cap is 6MB and the corpus is ~5.2MB
gzipped from 28MB raw; it was excluded from `FILES` for that reason. The durable route is the
operator dropping it into `C:\Dev\gex-signal-tapereader\data\es-1min\` on his own machine, where
his next push carries it. Until then a cloud session cannot see it.

## THE ONE THING THE CODE ALSO LACKS

`IB60` is one of the five confirmation chips and **the codebase has zero hits for it**.
`sessionLevels()` computes `ibH`/`ibL`/`ibSet` from `IB_MIN_S=1800` — a 30-minute initial balance —
plus `pdh`/`pdl`/`pdc`. `PMH`/`PML` are also absent (item 14, still open in LOCKED-ITEMS). IB60 is
net-new work and the operator has explicitly asked for a sweep testing IB30 **and** IB60 breaks.
