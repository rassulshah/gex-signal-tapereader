# ES 1-MINUTE CORPUS — the evidence base for ⓪a HOD/LOD

`EPM26-1min.csv.gz` — 406,154 rows, `EPM26`, **2025-05-30 → 2026-08-23**, gzip of a 28MB CSV.
Columns: `Symbol,Date,VOL,Open,High,Low,Close,Volume`.

## ⚠⚠ THIS FILE HAS BEEN LOST ONCE ALREADY. DO NOT LET IT HAPPEN AGAIN.

Supplied by the operator 2026-08-27, committed in a cloud sandbox at `a26cdfd`, **never pushed**, and
gone when that container was reclaimed. He had to send it a second time and was right to be annoyed.
The cloud CANNOT push; only the installer run on his machine can. **Anything that only exists in a
sandbox commit does not exist.**

⚠ It is **5.1MB gzipped against a 6MB installer payload cap**, so it cannot ride the self-extracting
`.bat` alongside everything else. It reaches GitHub by being committed on his machine.

## TIMEZONE — inferred, not declared

**CT.** The Sunday session opens `17:0x` and a full day holds exactly 1380 bars = 23 hours. The
mockup's own clock reads `10:26 CT` and the panel already lives in CT. RTH = `08:30–15:00` = 391 bars.

## ⚠ THE COMPLETENESS THRESHOLD IS LOAD-BEARING — 284 vs 283

    RTH bars >= 386   ->  284 sessions   <- reproduces the mockup header "284d ES 1-min"
    RTH bars >= 391   ->  283 sessions
    bar-count distribution: 391 x283 · 386 x1 · 226 x3 · 211 x8 · 210 x1

One session is 386 bars and is the whole difference. The prior note warned that "a rebuilt study that
doesn't also land on 284 has changed the filter rather than found more data" — so the filter is
**stated** (`MIN_BARS=386` in `tools/study-hodlod.py`) rather than left to be rediscovered.

⚠ 295 rows carry a bare date with no time (midnight bars). They are outside RTH and are skipped.

## GAPS

| gap | weekdays missing |
|---|---|
| 2026-07-18 → 2026-08-14 | 20 |
| 2026-08-24 → now | 3+ |

The July gap is recoverable at 2-minute resolution from Yahoo until **2026-09-16** — see
`session-state/YAHOO-PIPELINE.md` (item 18). After that it leaves the window permanently.

## DERIVED

`BASERATES.json` — regenerate with `python3 tools/study-hodlod.py /path/to/ES_TestingData.txt`.
Every figure the ⓪a section displays comes from it, with its n. Definitions are pinned in the
script's docstring; the survival ladder is the only genuinely predictive statistic in the section.
