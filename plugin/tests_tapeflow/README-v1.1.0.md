# TapeFlow v1.1.0 — corrected source handoff

## Status

**Source corrected and portable C++ tests passed. This is not a compiled or certified Investor/RT DLL.**

The input files were `pasted_content.txt` (wrapper v1.0.3) and `pasted_content_2.txt` (logic v1.0.1). The original wrapper is included for comparison. The correction preserves native Investor/RT data, standard-library-only production C++, one separate indicator pane, and tentative `?` signals.

**Pre-existing levels are NOT required.** Absorption bands are discovered from executions in the recent tape. A frozen local price band is a measured area of concentrated volume, not a user-drawn support/resistance level.

## Files

- `TapeFlow.cpp`: corrected shared RTX wrapper; do not compile it without a market definition.
- `TapeFlowLogic.h`: portable pressure, absorption, response, initiative, and baseline engine.
- `TapeFlowSupport.h`: portable snapshot validation, price conversion, journal checks, and rendering indexes.
- `TapeFlowES.cpp`, `TapeFlowNQ.cpp`, `TapeFlowCL.cpp`, `TapeFlowNG.cpp`, `TapeFlowGC.cpp`, `TapeFlowHG.cpp`, `TapeFlowEU.cpp`: separate-market launchers. Build each as its own RTX extension project using the real SDK. Do not link all seven into one DLL.
- `changes.patch`: unified source differences against the uploads.
- `run_tests.sh`, `tests/*.cpp`, `tests/validation.log`: reproducible portable tests and results.
- `tests/sdk_stub/*`: explicitly **test-only declarations**, never production headers.

## Main fixes

| Area | Original problem | Correction |
|---|---|---|
| Snapshot cursor | A second plus print count assumed unchanged ordering and silently skipped/repeated records when the consumed prefix changed. | Verify the complete consumed final-second prefix, including raw timestamp, tuple, and multiplicity. Changed/missing prefixes fail closed and resynchronize. This is conditional snapshot correctness, not an exchange-sequence guarantee. |
| Draw safety | Timer refusal allowed live and deep `RTTICKS` requests from `draw()`. | No tick queries from drawing. Calculation fallback performs only live reads; guarded backfill runs from the timer. |
| Backfill | Every staged history request replaced the live engine and could change historical signals/state. | Replay in scratch state. Preserve existing live samples and events; add only an unseen historical prefix. Import identified calibration separately. Recovery protects prior committed event keys. |
| Calibration refresh | Newly loaded history could recalculate the live session without explicit provenance. | Refresh prior-only baselines for future setups, record a versioned `BASE` diagnostic, and preserve existing episode thresholds. No live replay. |
| Invalid quotes | A later valid quote could hide missing, crossed, locked, or stale observations. | Poison that observed second; require continuous observed paths for qualification. Actual unobserved quote paths remain unknowable. |
| Warm-up and gaps | Stale quiet seconds increased warm-up; gaps retained classification/rearm metadata. | Consecutive valid observed-path warm-up; reset quote, trade, latch, initiative and balance metadata on gaps. Never reopen already committed seconds. |
| Spread | Only the final spread could conceal an earlier wide spread in the same second. | Use maximum observed spread including carry-in; require calibrated spread. Frozen episode spread thresholds survive five-minute-bin changes. |
| Baseline windows | Count-based replacements confused overlapping/disjoint partial history and could inflate calibration. | Timestamp identity `(session, contract, endT)`, deduplicated union, first committed measurement wins. Prior-only W2 records. |
| Store saves | Same row count incorrectly meant unchanged content. | Stable serialized-content fingerprint; flush-check temporary publication and backup recovery. |
| Event logging | Fixed buffer truncation and incomplete journal records could suppress retries. | Stream serialization, safe text fields, complete-record checks, incomplete-tail separation, and validated retry keys. Not a power-loss-proof database. |
| Price/quantity safety | Unchecked narrowing, silent invalid price rounding, float volume loss and overflow hazards. | Checked native-array precision conversion, normalized known tick increments, exact integer per-second volume including unknown, and overflow guards. |
| Band search | Spans over 4,000 ticks silently discarded lower prices, introducing directional bias. | Fail closed instead of truncating. Cached Dense prefix sums make band sums constant-time. |
| Configuration | Public settings allowed out-of-bounds partitions/band arrays. | Validate fixed 20-second/four-by-five-second schema, 1–8 half-width bands, counts, thresholds and timing. |
| Signal rearm | Opposing flow could count as a neutral reset; missing data could clear latches. | Initiative neutral rearm requires uncertainty bounds inside the neutral band with valid observations. Missing paths do not earn quiet/movement rearming. |
| Signal timestamp | Evaluated second label looked actionable at its start. | `Ev.t` remains the second label; `Ev.knownAt=t+1` is conservative earliest availability. Wrapper logs and positions signals at `knownAt`. Actual callback arrival may be later. |
| Rendering | Repeated backwards scans, incorrect boundary inclusion, overlapping tiny buckets, all signals at candle center. | Cached prefix index; `[start,end)` execution buckets; position signals within the candle; clip incomplete buckets; aggregate all data into fewer buckets when zoomed out. |
| Concurrency | Timer and rendering could access reallocating vectors without serialization. | Per-object recursive mutex with nonblocking timer/draw acquisition; skip a frame rather than wait during history work. SDK lifecycle still requires integration verification. |

## Signal behavior retained

- **AW?**: absorption **watch**, not a reversal entry. High localized same-side effort, coverage, concentration, repeated partitions, fresh effort, limited adverse progress, and a subsequent three-second hold.
- **AR?**: post-watch directional response. Frozen band reclaim, qualifying pressure improvement, activity/coverage, a continuous three-second hold, and fresh response-side execution in the final second.
- **IN?**: initiative/continuation. Strong uncertainty-adjusted directional flow, context alignment, activity, and acceptance beyond a frozen preceding-tape extreme. Fresh final-second execution is mandatory.
- **x**: watch failure, price invalidation, data invalidation, or expiration. Original watch markers remain; failure is not an erased watch.
- `?` remains on directional markers. Code tests establish implementation behavior, not profitable trading signals.

No external session high/low, range boundary, manual price level, third-party service, or external model is required for these signals.

## Optimization choices

The indicator still computes on one-second trade buckets. Default fast/context/observation windows remain 30/180/20 seconds. The ordinary 3-minute chart shows six 30-second pressure buckets when pixels permit; zoomed-out views intentionally aggregate instead of overlapping bars. Use a **time-based chart**; non-time charts are rejected rather than silently treated as 3-minute bars.

Baseline quantiles use `nth_element` instead of fully sorting for every statistic. Dense band prefix sums are reused. Rendering indexes rebuild when retained history changes and are reused on unchanged redraws. Committed-event capture advances through newly added records instead of rescanning all events each poll.

The original **eight-second quiet-second late allowance is retained**. Active seconds still close when a later-second execution arrives; this allowance is not a uniform eight-second signal delay. Reducing it should follow measured CQG latency, not a guess.

One artificial rendering benchmark on this sandbox: 259,200 records, index construction about 20.9 ms; 1,200 bucket queries across 1,000 repeated redraws about 175.7 ms total. These are engineering timings for a particular run, not a guarantee of IRT responsiveness or a comparison with live markets.

## Validation performed

- Portable core: **3,487 fabricated CHECK assertions passed** with warning-enabled C++11 and AddressSanitizer/UndefinedBehaviorSanitizer.
- Support regressions: snapshot overlap, legitimate identical-print multiplicity, altered/shrunk prefix, missing overlap, invalid times/size, float feed price conversion, event keys, partial journals, escaping, and prefix indexing passed.
- Core coverage includes quotes, gaps, warm-up, configuration rejection, exact volume, serialization, window deduplication, prior-only cohorts, frozen gates, AW timing, mirrored AW/AR/IN, final-second execution requirements, FAIL/INVP/CBRK, expiration precedence, and walked-versus-jumped replay checks.
- **The 3,487 count is not 3,487 independent trading scenarios.** Many assertions check numerical reference values and each accepted fabricated input.
- All seven launchers passed Linux source syntax checks against the supplied **test-only declarations**. No real Windows RTX SDK, production ABI, live CQG feed, installation, or vendor DLL build was available.
- The input's inherited claim of 53 tests plus 300 random tapes was not independently verifiable and is not repeated as our result.

Run portable validation on Linux with:

```bash
./run_tests.sh
```

No Python or shell script is used by the production extension. Those tools are only for tests/build review.

## Important migration changes

New calibration files use `<market>-base-v110-<session>.csv` and W2 timestamped records. New event journals use `<market>-events-v110-<sanitized-contract>-<date>.csv`.

**Old baseline and event files are not overwritten or deleted.** Legacy windows have no trustworthy exact identity, so they cannot safely be used as deduplicated calibration. Rebuild from native IRT history or collect fresh sessions. The indicator may show `CALIBRATING` until sufficient identified, eligible prior-session windows exist. Sixty windows can be roughly four sessions only with nearly complete usable sampling; coverage/path filters can require longer.

FRONT-ROLL normalization remains explicit and enabled by default. Front contract selection uses total execution volume in eligible recorded windows, not thin-window average rate. It is **not** proof of the exchange's actual front contract or full-session volume. Optional `cfg.frontRoll=false` selects same-contract history, requiring separate calibration after a roll.

## Investor/RT integration gates — do not skip

1. Compile one launcher with the actual vendor `irtsdk.h`, SDK libraries, supported compiler, export settings, architecture, and CRT. Include both production headers. Never add `tests/sdk_stub` to the production include path.
2. Verify `RTDATE` subtraction units, RTTICKS lifetime/order, raw timestamp precision, whether overlapping queries include the entire last second, and the element types of native tick arrays. The verified-prefix cursor deliberately fails closed if these assumptions fail. Obtain a documented trade sequence/ID if available rather than inventing one.
3. Verify chart-bar timestamps are closing timestamps for the chosen 3-minute chart. Current bucket alignment inherits that assumption; if IRT uses opening timestamps, adjust the adapter, not the engine.
4. Verify stable chart/contract context in timer callbacks and native redraw scheduling. The wrapper does not invent an undocumented invalidate API. If one extension object is dispatched across hosts, implement the vendor-supported host registry before multi-chart use. Initially test one active attachment/contract per market extension.
5. Set/verify the intended timezone. This source retains **local civil seconds and a 17:00 session policy**. It does not contain a full exchange calendar or absolute-time/civil-time split. DST fall-back/nonmonotonic input fails closed; it cannot reconstruct the repeated hour. Weekend/deep-backfill scheduling assumes CME-style Central time and maintenance 16:00–17:00. Change/validate this for a different clock/venue/calendar.
6. The feed supplies quotes at trades, **not** every quote update, displayed depth, order additions/cancellations, or MBO. Absorption is an execution/observed-price inference; it cannot prove hidden replenishment or actual continuous liquidity defense. Quote validity at a print is not proof of zero feed latency or an exchange aggressor flag.
7. Compare live recording and replay on ES/NQ first, then CL/NG/GC/HG/EU separately: counts, side coverage, unknown volume, warm-up, baseline versions, event availability, simultaneous charts, disconnect/reconnect, roll, and skipped/changed snapshots. Calibrate parameters with out-of-sample data and realistic fees/slippage. No parameter optimization for profitable expectancy has been performed.
8. File publication improves recoverability but does not guarantee power-loss durability or cross-process coordination. Per-object mutexes do not make multiple DLL/process writers to the same market files safe. Use one writer per market until a vendor-supported shared manager/file-lock design is verified.

## Instruction to Claude / the developer

> Review these three production source files and compile the appropriate market launcher using the actual Investor/RT RTX SDK. Preserve level-free tape-derived absorption, AW/AR/IN distinctions, tentative markers, frozen episodes, and identified prior-only calibration. Do not include the test SDK shim in a production build. Verify the integration gates above and adapt only the documented SDK layer where required. Run portable regressions, then native replay/live comparison. Report real compiler errors and any unavailable data instead of substituting undocumented APIs or declaring the system production-tested.
