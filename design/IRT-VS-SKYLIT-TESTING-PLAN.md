# IRT vs SKYLIT — the indicator consistency test plan

_Created 2026-09-15. Standing rule (operator, 2026-09-07): **"We should match with Skylit always in order to have
a source of truth to compare against."** This plan operationalises that — it checks, several times a day, that the
IRT indicators still say what Skylit says, and flags drift the moment it appears._

---

## 0. THE PIPELINE — three snapshot points, two gates

```
   SKYLIT ATLAS  ──(panel reads DOM tape + gex/levels feed)──▶  GammaProfile.csv  ──(plugins parse+draw)──▶  IRT
   (source of truth)            GATE A: extraction                 (the contract)        GATE B: render        (chart)
```

Every discrepancy lives in exactly one gate, and naming which one is half the fix:
- **GATE A — Skylit vs CSV.** Did the panel read Skylit correctly and write the right numbers? A gap here is a
  PANEL bug (`gammaProfileBuild`, the tape reader, a scale/sign conversion).
- **GATE B — CSV vs IRT render.** Did the plugin parse the CSV and draw it faithfully? A gap here is a PLUGIN bug
  (parse, scale, colour, layout).

Check both. If Skylit and IRT disagree, read the CSV to localise the gate before touching any code.

---

## 1. THE GOLDEN RULE — SAME MOMENT, or it is not a comparison

⚠ **A stale CSV against a live tape is not a test — it is noise.** On 2026-09-15 after hours the CSV (written
23:58) said SPX King 7595 while Atlas showed ~7620; that is the King rolling in the thin overnight book between the
two snapshots, not a bug. The books move; only simultaneous snapshots are comparable.

Procedure for every checkpoint:
1. On the Atlas tab, click the panel's **⟳ now** (IRT FlexLevels export) to force a fresh write — or confirm the
   180s timer just fired (`ASOF` within ~60s of now).
2. Immediately read all three: Atlas tape/chart, the freshly-written `GammaProfile.csv`, the IRT render.
3. Diff field by field. `ASOF` (CT sec-of-day) and the Atlas clock must be within ~60s or discard the round.

---

## 2. THE SCALE / WINDOW / BOOK RULES — the project's #1 error, encoded

Comparing a number from one book / window / scale against another has produced four separate phantom bugs in this
project. Before comparing ANY value, pin all four:

- **BOOK** — SPX (SPXW tape), SPY, QQQ, NDX. The ES chart's King is the SPXW book; SPY is its own book. Never
  compare SPX to SPY strikes.
- **WINDOW** — the expiry set. The panel self-fetch pins **0DTE / current / exp_count=1** (F-22). Atlas must be on
  the same window: **RTH · READ AS %King · VELOCITY All · LOW NODES never Hide** (SKYLIT-FEEDS.md). If Atlas is on
  a different projection window or the **Derived** toggle differs, the node universe differs — reconcile the setting
  first, do not report a bug.
- **DERIVED vs RAW** — the Atlas top bar shows `Derived 3/3` / a `Derived` chip. The panel reads the DOM tape
  (`tapeMapLive('SPXW')`), which is the rendered ladder. Confirm Atlas is showing the same layer.
- **SCALE** — SPX strike (~7595) vs ES price (~7599, via `esOfSpx`, ratio ~1.00058) vs SPY (~761, ×rr ~10). The CSV
  carries BOTH: `KINGNOW,ES,SPX,<esPx>,<strike>,<pct>` gives the strike AND the ES price. Compare strike-to-strike
  or ES-to-ES, never mixed. The gamma `KING` row and `STRIKE` rows are ES price; `KINGNOW`/`KINGTRACK` carry both.

---

## 3. WHAT TO COMPARE — by indicator, with tolerance

For each, Skylit value ↔ CSV row ↔ IRT render. Tolerance is the max gap that still counts as MATCH.

### 3.1 Gamma profile (lsGammaProfile) — the histogram + level rail
| field | Skylit (SPXW tape) | CSV row | tolerance |
|---|---|---|---|
| King strike | the boxed/King-marked strike | `KING` (ES) + `KINGNOW,ES,SPX,…,<strike>` | exact strike |
| King sign & % | the King cell sign & % | `STRIKE,<es>,<pct>,1,1` | sign exact, % exact |
| Top-5 nodes | the 5 largest \|%\| strikes | `STRIKE` rows with `rank` 1–5 | strike set exact, %±1 |
| Put wall / Call wall | PW0 / CW0 | (rail levels) | exact strike |
| Flip | FLIP0 | (rail) | exact strike |

### 3.2 King tracker + King chart (lsKingTracker, panel King chart) — RTH ONLY
| field | Skylit | CSV row | tolerance |
|---|---|---|---|
| SPX King level | SPXW King strike → ES | `KINGNOW,ES,SPX,<esPx>,<strike>` | strike exact; ES ±1 tick |
| SPY King level | SPY King strike → ES | `KINGNOW,ES,SPY,<esPx>,<strike>` | strike exact; ES ±1 tick |
| QQQ / NDX King | QQQ / NDX King | `KINGNOW,NQ,…` | strike exact |
| Roll count (RTH) | count Atlas King moves 08:30–15:00 | `KINGTRACK` rows in RTH | ±1 (dwell/near-tie judgment) |

⚠ The King chart is RTH-only by design (v16.22). Do not count after-hours flips on either side.

### 3.3 Day-model candle (lsDayModel)
| field | Skylit (ES chart) | CSV row | tolerance |
|---|---|---|---|
| Actual HOD / LOD | session high / low + clock | `DAYHOD` / `DAYLOD` | ±1 tick, ±1 min |
| MUD | (derive: 2nd extreme − open) | `DAYMUD` | ±0.5 pt |
| Swept levels | PDH/PDL·ONH/ONL·PWH/PWL·PFH/PFL ticks | `SWEPT` rows | strike exact, status R/B/T |
| Expected candle | (model, no Skylit equivalent) | `DAYEXP` + `EXPMODEL` | internal: `EXPMODEL` basis sane for the clock |

### 3.4 Day-stats strip (lsDayStats) + THE READ
| field | source | CSV row | tolerance |
|---|---|---|---|
| Actual 1ST/2ND, took, gap, range | ES chart | `DAYSA` | ±1 tick / ±1 min |
| Expected row | the model | `DAYSE` | internal consistency (DAYSE ↔ DAYEXP) |
| READ (HOD IN / LOD IN %) | panel READ box (Dashboard) | `READ` row | panel box % == plugin line % exact |

The READ is the one cross-check that is IRT-internal: the plugin's READ line must equal the panel's own READ box
for the same bar (same HLTAB lookup). If they differ, the CSV `READ` row or the plugin parse is wrong.

---

## 4. CHECKPOINT SCHEDULE — CT, RTH

Five snapshots a day, chosen at the session's structural moments:

| # | CT time | why |
|---|---|---|
| T1 | **09:05** | opening range settled (~35 min in); first King, walls, opening range, EXPMODEL should be `open30` |
| T2 | **10:30** | mid-morning; King rolls, node migration, first deflections |
| T3 | **12:00** | midday; HOD/LOD developing, EXPMODEL `open60`, READ maturing |
| T4 | **14:00** | power hour; King, deflections, HOD/LOD near-final |
| T5 | **15:05** | after the close; full-day reconciliation — final HOD/LOD/MUD, swept set, RTH roll count, day stats |

T5 is the anchor (everything is final and comparable). T1–T4 catch intraday drift.

---

## 5. THE COMPARISON PROCEDURE (a session with Atlas + device + IRT access)

1. Confirm the setup posture on Atlas: RTH · READ AS %King · VELOCITY All · LOW NODES not Hidden · the same
   Derived/window the panel uses. If it drifted, note it and fix the setting — do not log a bug against a
   mismatched setting.
2. Force a fresh export (⟳ now) and stage `%USERPROFILE%\InvestorRT\rtx\lsFlexLevels\GammaProfile.csv`.
3. Read Atlas (tape ladders + chart) and, where needed, the IRT render (a masked-desktop screenshot of the ES/NQ
   panes; grant Chrome too if the Atlas read must come from the desktop).
4. Diff every field in §3 against its tolerance. Classify each: **MATCH · DRIFT (within 2× tol) · MISMATCH**.
5. Append the round to the log (§6). Any MISMATCH: localise the gate (read the CSV — is it A or B?), open a fix.

---

## 6. LOGGING — `testing/irt-vs-skylit/<YYYY-MM-DD>.md`

One file per day, one block per checkpoint. Minimal, greppable:

```
## T5 15:05 CT — ASOF 54120 (Atlas 15:04:30) — posture OK
gamma King      | SPXW 7620 (+93%) | CSV 7620/+93 | IRT 7620 | MATCH
SPY King        | 761              | CSV 761/7626.55 | IRT ~7626 | MATCH
RTH rolls SPX   | Atlas ~3         | CSV 3 | IRT 3 | MATCH
HOD/LOD         | 7719.75/7595.25  | CSV same | IRT same | MATCH
READ            | box LOD IN 84%   | CSV READ LOD/…/84/IN | IRT line 84% | MATCH
scorecard: 14/14 MATCH, 0 DRIFT, 0 MISMATCH
```

Keep a running scorecard at the top of each day's file, and a rolling `testing/irt-vs-skylit/SCORECARD.md` (last
10 sessions, per-indicator match rate). A field that DRIFTs two sessions running, or MISMATCHes once, becomes an
open item with its gate named.

---

## 7. OPEN ITEMS TO RECONCILE FIRST (before trusting the green)

1. **King strike, same-moment.** The 2026-09-15 stale CSV (7595) vs live Atlas (~7620) must be re-checked at a
   single RTH moment with a fresh ⟳-now write. Expected: CSV `KINGNOW` strike == Atlas SPXW boxed King exactly.
2. **The Derived/window posture.** Confirm the panel's `exp_count=1` 0DTE book equals what Atlas draws with the
   operator's standard settings; if Atlas's `Derived` layer differs from the DOM tape, document which the panel
   should mirror (SKYLIT-FEEDS.md, F-22).
3. **READ line == READ box.** First RTH with v16.22 + lsDayStats 0.5: the plugin's READ line % must equal the
   panel's READ box % for the same bar.
4. **Fresh CSV carries the new rows.** Confirm a live v16.22 write includes `READ`, `EXPMODEL`, and a bodied
   `DAYEXP` (close≠open) — the after-hours CSV on 09-15 predated them.

---

## 8. HOW IT RUNS (cadence mechanism)

The comparison needs live Atlas (browser) + the CSV (device) + IRT (desktop) — all session-bound. Two ways to run
it several times a day:
- **Attended, in-session:** while a session holds the Atlas tab + device + IRT access, self-schedule wake-ups at
  the T1–T5 CT times, run §5, append to the log. Best fidelity (all three sources).
- **Device-bound scheduled task:** a task with `requires_local_device` fires at each checkpoint; it stages the CSV
  and screenshots IRT via the device, and reads Atlas via the device's browser. Durable across sessions; set up
  once the attended flow is proven. (Gate A alone — Skylit vs CSV — the panel could also self-log, since it holds
  both Skylit's data and the CSV it writes; that is the most robust automation for the extraction gate.)

Start attended (this session, next RTH), prove the field list and tolerances against real ticks, then harden into
the scheduled task.
