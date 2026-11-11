# lsStepTrend 1.0.0 "Step Trend" (2026-11-11)

## What he asked (CT)
- 2026-11-10 22:28 "I am also interested in a stepwise trend indicator. It looks at the hourly bar and the current zig zag rotation (going up or down), if the hourly close and high are above the prior hourly close or high, it considers this an hourly breakout and on the 3min ltf chart the level drawn is on the prior hourly bar high."
- 22:31 - 23:20 mockup rounds: "first show the 1 hour chart on top and then the 3 min chart below it with zig zags"; "those hourly highs should be coming from the top of the waves"; "they should be connected steps"; "get rid of the labels"; "did you put the zig zag wave rotation requirement" (his 60-min ZigZag: Use High / Low, Min Price Change 1 Bars); "connect the tops without going across"; "once it steps up, the prior high becomes support, so you can extend it as a subtle dashed line, not too thick".
- 23:53 "build" -> Step Trend indicator, clock hours (asked: clock hours vs hours from the session open).
- 2026-11-11 00:08 "when the rotation is up and the hourly bar closes are above prior bar highs the lines are green, and if its vice versa, do you know to make them red" (yes, mirrored). 00:11 "the bar closing higher needs to be green and vice versa" -> the breakout hour must itself be a green (up) / red (down) candle (body rule).

## What it does
On the price chart (a period that divides the hour; his is 3 min). No text, no hovers, no settings.
- HOUR: clock hours of the chart's own candles (IRT stamps a candle with its END; the candle belongs to the hour its START is in). Closed when a later candle exists.
- STEP UP (green): the hour closes above the prior hour's high, is a green candle, and the hourly ZigZag (High / Low, 1 bar) is rotating up (or has no rotation yet). Level = the prior hour's high, starting at the candle that made it (the wave top). STEP DOWN (red) is the mirror on the prior hour's low.
- LINES: from a top straight up (down) to the next level, then flat to the next top (3 px, the new step's colour). The newest level is dashed to the right edge. Once the next step the same way exists, the old level runs on thin (1 px) and dashed in a dimmer green / red until the first candle after the new step's start closes back through it. A dot marks each level's start candle.
- Never repaints: a step is added when its hour closes; a support line only gets its end, once. (The newest level's dashed line becomes the thin support line when the next step arrives - the same level, a lighter style.)
- Status file per chart: lsFlexLevels\StepTrend\<MKT>-<seconds>.status.txt (version, rule, candle / hour / step counts, the last 8 steps).

## Files
plugin: StepTrend.cpp (host), StepTrendLogic.h (stp::Engine + layout, no SDK), StepTrendParams.h (rule 0, rot 1, body 1 for every market; by hand), tests_step/ (t_step.cpp, fx_mockup.txt, ref.js, render_dump.cpp, render.py, wrap_real_sdk.cpp, mock/irtsdk.h).
tools: gex-build.next.bat (the `call :one StepTrend lsStepTrend ...` line + the status list).
LRA: analytics/lra/step_trend_score.py + analytics/test_step_trend_score.py + test_step_trend_fx.txt; parts.json part "STEP TREND"; setup_scorecard line.

## Tests (all pass)
- tests_step/t_step.cpp, ASan / UBSan, -Werror: 130 checks. PARITY: the engine gives exactly the steps of the mockup's own code (ref.js) on the 7 real chart days (ES Nov 10 day + evening, NQ / GC / CL Nov 9 and 10), both rules, rotation on and off = 28 cases incl. start candles and support ends. BODY rule. NO REPAINT candle by candle + a restart after any candle. FORMING (a step only when the hour is complete). ties, session gap, out-of-order candle, rotation. LAYOUT. HOST on the mock SDK: candle by candle = the engine; drawing only flats and risers, green / red, widths 3 / 1, inside the pane, clipped to the visible candles; status file; changed history; two charts on one DLL; symbol change; a 7-minute chart (nothing drawn, said in the status file); unreadable candles; no tick; one candle; no visible range; a host with no slot; 7,600 candles (re-feed of the last 5,000). VERSIONS.
- MinGW against the REAL irtsdk.h (-Dsmall=char): 0 errors, 0 warnings outside the SDK.
- render check: render_dump + render.py = the plugin's own draw list on the ES and CL days, compared with the mockup (same steps, same shape).
- analytics/test_step_trend_score.py: 6 tests (walk() == the mockup's steps on the same fixture, no repaint, body rule, follow / retest, nightly files, judge needs both halves).

## First nightly run (stored 3-min bars, all sessions, 36 - 56 days per market)
Follow (2 ATR further before 2 ATR back, 60 min from the hour's close): ES 49% (419) vs any hour 48%; NQ 53% vs 51%; CL 53% vs 52%; GC 48% vs 52%; HG 48% vs 52%; NG 46% vs 50%; 6E 52% vs 53%. The level was retested after 60 - 66% of steps and held on 34 - 42% of the retests. No challenger passed. Status LEARNING everywhere: the steps are a trend map, not a proven signal.

## Risks / not verified (needs his IRT)
- The look on the real chart: line widths, the hand-drawn dashes of the newest level, the thin P_DASH support line, the dot size, colours against his background.
- That IRT's hour grouping of his 60-min chart equals clock hours of the 3-min candles (it should: both are clock aligned).
- IQFeed backfill vs Rithmic live candles can differ by a tick: after a restart a level can sit one tick away from where it was drawn live.
- Steps older than the last 5,000 candles are not drawn.

## 1.1.0 (2026-11-11) - the support (retest) lines end
Rassul 07:21 CT "the retest lines should not extend indefinitely . they should stop once they are tested or once another step up is made or a step down is made".
- An old level's thin dashed line now ends at the FIRST of: (a) tested = the first candle after the new step's start that trades back to the level (low <= level for support, high >= level for resistance); (b) / (c) another step is made, up or down = the line stops where that step starts (its wave top / bottom). A test that happened before that step's hour closed keeps its place.
- 1.0.0 ended the line only when a candle CLOSED back through the level, so an untested level ran to the right edge for the rest of the chart.
- Still running to the right edge: only the level before the newest step, while it is untested and no further step exists.
- When another step is confirmed, a line that was running to the right edge is cut back to where that step starts (the steps themselves are drawn back to their wave tops the same way).
- Tests: t_step.cpp 134 checks (the rule written out candle by candle on the 28 mockup cases: support lines, ended, with a later step; unit cases a / b / c; an ended line is never drawn to the right edge), ASan / UBSan; MinGW real SDK 0 errors 0 warnings; render check on the ES day. lra.step_trend_score 1.0.1 walk() follows the same rule (6 tests).

## 1.2.0 (2026-11-11) - rotation follows every hour; works on the 60-minute chart
- 07:57 CT, with NQ screenshots: "the green step didnt draw when the nq bade a higher green bar". Cause (my rule): the hourly rotation was a
  3-pivot ZigZag that needed a completed swing low before it turned up, so it stayed "down" through a green hour that closed above the prior
  hour's high, and the step was skipped. Now the 1-bar rule of his chart's ZigZag (Use High / Low, 1 bar): an hour with a higher high and not
  a lower low = up; a lower low and not a higher high = down; both (outside hour) = the candle's colour; neither (inside hour) = unchanged.
- 08:04 CT: "i also want a version of this indicator to draw on the hourly charts or can you update an existing indicator on the hourly ?"
  The same lsStepTrend now accepts a 60-minute chart (periodOk up to 3600 s): each candle is an hour, the same levels, each drawn from the
  hour candle that made it. Nothing else to install.
- Nightly scorer lra.step_trend_score 1.0.2 uses the same rotation rule (its first score, with the old rule, is replaced tonight).
- Speed check (tests_perf/check_draw_io.py): growth with the candles on screen fails only when the big scene costs 0.5 ms+ a draw
  (Step Trend 1.2.0 was failed for "10x growth" at 0.03 ms).
- Tests: tests_step/t_step 134 pass (ASan / UBSan, -Werror): the NQ case, the 60-minute chart = the 3-minute chart's steps in every fixture
  day, parity with the mockup (21 cases); MinGW real-SDK syntax check 0 warnings; speed check pass; python 6 pass.
- Not verified (needs his IRT): the look on his 60-minute chart (line thickness and the dots at hour-candle width).

## 1.3.0 (2026-11-11 15:08-15:12 CT) - the 60-minute chart
Rassul: "only draw the dashed line after the stp up. when a candle then pullbs back and touches it, without breaking the step up cancles low, its can be
called a T which means that the hbo is tested. also you missed a candle that stepped up ... you put the step on the wrong red candle next to it even
though only green candles that close higher are suppose to have the step"; "it should go under the low of the candle. only when it is touched or exceeded".
- Cause of the "wrong candle": the step was anchored to the candle whose high was broken (the wave top, right on a 3-minute chart) - on the hourly
  chart that is the candle BEFORE the breakout candle. Hourly mode (stp::Engine::hourly, spb 3600) anchors the step, dot and dashed line to the
  breakout candle; level = the prior hour's high / low as before.
- T (stp::Mark): the first later candle whose low touches / goes through the level without trading below the breakout candle's low; a candle that
  trades below that low first ends the line with no T; the next step ends it too. Under the low (green) / over the high (red, step down).
- 3-minute and other charts: unchanged (layout hash = 1.2.0 on 6,000 ES 3-min bars).
- Tests: tests_step/t_step130.cpp 10 pass (680 real ES hours Oct-Nov), t_step 134 pass, MinGW 0 errors. BO / ReadBox tests pass with the shared header.
- Mockup https://claude.ai/artifact/GPHeTaUZ75ZHtELN5dYFTU . Not verified on his chart.
