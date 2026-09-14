# lsKingTracker — the King Tracker IRT plugin (design + status)

**Roadmap #4.** A SEPARATE RTX indicator (`plugin/KingTracker.cpp` → `lsKingTracker.dll`), kept apart from the
other three plugins so its dialog / parameter numbering can never destabilise them. All four read the same
`GammaProfile.csv`; each ignores the others' rows. **Source READY v0.1 (2026-09-14) — compile & calibrate next.**

## What it draws — the operator's ask
> "a new indicator called king tracker which tracks the king movements throughout the day … stepped lines that
> move throughout the day … for the SPX and … the spy king … as well as for qqq and ndx on the nq futures chart."

Each book's King as a **TIME-indexed stepped line**: the King holds a flat level, then **steps** to a new price
when it rolls to a new strike. Price-aligned (each step at the King's own futures price) and time-positioned
(each step at the bar of the roll). Two lines per chart:
- **ES chart:** SPX King (magenta, matches the gamma King) · SPY King (cyan).
- **NQ chart:** QQQ King (green) · NDX King (amber).

The chart's family (ES/NQ) is auto-detected from the root symbol (`getRootSymbol`); a **Chart family** setting
(Auto/ES/NQ) overrides it. Only the family's two books draw.

## The data — where the journeys come from
The panel keeps a per-day store **`KTRK`** (SEPARATE from `KINGDAY`, which tracks the SPY/QQQ strike for the
"Moved N× today" panel text). Every 30s, `ktrkSample()` reads each of the four books' current King in that
book's **futures price space** via `futDerBookKing(futSym, sources)`:
- ES1 → SPX `['SPXW','SPX']`, SPY `['SPY']` · NQ1 → QQQ `['QQQ']`, NDX `['NDXP','NDX']`.
Both ES1 and NQ1 derived payloads self-fetch every 60s while the export is on (`selfFetchFut`), so all four
populate regardless of which chart is active. A **step is recorded only on a CONFIRMED King-STRIKE change**
(2 consecutive samples on the same new strike — a one-poll flicker can't add a step). Persisted per trading
day; resets on a new day. `ktrkSample()` also runs once inside every `gammaProfileBuild()` so an export never
lags the 30s timer.

## CSV rows (King-tracker; other plugins ignore them)
```
KINGTRACK,<fam ES|NQ>,<book SPX|SPY|QQQ|NDX>,<secOfDay>,<futPrice>,<strike>   one confirmed step
KINGNOW,<fam>,<book>,<futPrice>,<strike>,<pct>                                live right-edge level
```
The plugin builds each book's polyline from its `KINGTRACK` steps, then a synthetic final point at the right
edge using `KINGNOW` (so the live end sits at the current King price / stays aligned with the gamma magenta
King even as basis drifts). It draws each hold flat then the vertical step; optional roll-point dots and a
right-edge `SPX 7700` label.

## How the clock→bar mapping works (the one thing to calibrate live)
The roll clock is CT sec-of-day (the panel's `ctNowSecOfDay`). The plugin maps it to a chart bar with:
```
RTDATE now=currentDate(); getLocaltime(now,&tm);           // today's date, machine-local
tm.hour/min/sec = roll clock (+ Clock-offset setting);     // build "today at HH:MM:SS"
RTDATE rd=makeRTDATE(&tm);
RTARRAYI dates(barDateTime); dates.getBarNumber(rd,&bar,0,kDATE_GE);   // first bar >= that time
```
with a manual `dates[i] >= rd` scan fallback if `getBarNumber` fails. `PNT.set(bar,price)` then gives x from the
bar and y from the price (INSTRUMENT_SCALE). **⚠ If IRT's chart timezone ≠ CT, steps land at the wrong X — use
the "Clock offset (min)" setting to shift them; tune once against a known roll.**

## Settings (14 params, explicit `pc++`, `setParameterVersion(1)`)
Chart family (Auto/ES/NQ) · Line width · per-book show + colour (SPX/SPY/QQQ/NDX) · Right-edge labels · Roll-point
dots · **Clock offset (min)** · Font size.

## Build / install
Close Investor/RT → double-click `plugin/compile-kingtracker.bat` (or `build-kingtracker.bat` from an x64 VS
prompt) → `lsKingTracker.dll` → `%USERPROFILE%\InvestorRT\dllx64` → reopen IRT → add the RTX indicator to the
ES chart (SPX+SPY) and to the NQ chart (QQQ+NDX).

## Flags
`POST_DRAWING | OVERLAY | NO_UI | INSTRUMENT_SCALE` — price-aligned, draws on the instrument pane.

## Known / to verify
- Clock→bar timezone offset (above) — the primary calibration.
- `RTARRAYI(barDateTime)` is assumed to expose the chart's bar dates directly; the manual-scan fallback covers a
  non-populated `getBarNumber` search, but confirm steps sit at the right times before trusting them.
- The KINGNOW level should coincide with the gamma-profile magenta King on the ES chart at the open.
