# lsLimitProfile / lsDealerProfile 2.59.0 (2026-11-10) - the node profile

## Requests (Rassul, 2026-11-10 CT)
- 18:46 "start working on the limit order profile ... fit for identifying support and resistance ... i also dont want it always updating every second"
- 18:49-18:56 "the labels should be inside the node and the profile should be facing price"; "the rank can be on the outside"; "rank without background"; "get rid of the ratio since we already have % king"
- 19:13-19:16 "keep the level badge left justified so i can easily see the level and then the liquidity"; level name "use upto 1 ATR"
- 20:08 "just do all of it"; 20:44 "it will only be on the profile right, not on the chart"; 20:51 "earlier mockup" (claude.ai/artifact/6BVGfyxKXGJY6qVbSs29XU v8)

## What changed (drawing only; the levels still come from lsBookFlow's native top book, kind G: 3 bids + 3 offers, its filters and ranks)
| Element | 2.58.0 | 2.59.0 |
|---|---|---|
| Row | rank box, thin bar, lots after it | NODE anchored at the column's outer edge, pointing at price, length = % of the king |
| Text | after the bar | INSIDE the node from its tip: [level badge] lots  pct%  time in the top book  [S20 / ? / EXIT] |
| Rank | boxed, at the row front | outside, in front of the node, no box (K = the largest, 2-6) |
| Level name | none on these rows | a key / volume / gamma level within 1 ATR (3-min) names the LARGEST node only (white badge, no border) |
| Update | every draw (the feed posts once a second) | once a minute; at once when price is within 2 ticks of a node or the book goes cold |

## Study behind it (lra.limit_levels 1.1.0, 7 markets, 39 sessions)
22,345 touches of a resting band + 1,100 first sweeps of key / MenthorQ levels: size, rank, time rested, on-a-level, pulled / held on approach
and the hit behaviour (held / refilled / absorbed / pulled / eaten) do NOT predict the turn (50-57% everywhere; judged fairly from the price 60 s
later: 46-50%). Largest band in reach ON the level: 75% vs 67% close back (small, held out 73% vs 71%). So: a MAP, no signal is drawn.

## Tests
- test_dealerprofile_2590.cpp (the real DealerProfile.cpp on the mocked host): 32 / 32 - versions (cpp, settings, LimitProfile wrapper); node texts;
  every node anchored at the outer edge with its text ending inside; rank outside, no box; once-a-minute hold, touch, cold book; level badge on the
  largest node within 1 ATR only; the 2.54-2.57 checks (native top book, ENT / EXIT tags, params) unchanged. Clean under ASan / UBSan (no errors).
- MinGW syntax check of LimitProfile.cpp against the real irtsdk.h: 0 errors (only the old warnings).
- Render of the test scene: limit_profile_2590_render.png.

## Not verified - needs his Investor/RT
1. The look on his wide monitor with real fonts (text width inside short nodes).
2. "time in the top book" starts when the indicator first sees the level (restart = starts again).
3. The level badge uses the levels of the Key Levels column; with no ATR yet (first bars) no badge is drawn.

# 2.62.0 (2026-11-10 21:41 CT) - Limit Profile only
Rassul: "what dealer profile, we only made changes to the limit profile right?" / "retire that and only call it the limit profile".
- lsDealerProfile is retired: its build line is commented out in gex-build (the Limit Profile line still builds DealerProfile.cpp, its source),
  it is off the BUILD-STATUS list, and tools\retire-dealerprofile-2026-11-10.bat moves the installed lsDealerProfile.dll to
  InvestorRT\_retired-2026-11-10 (not deleted; undo = move it back).
- Built as lsLimitProfile the column header reads "LP v2.62.0  LIQUIDITY" (was "DP v...").
- Unchanged on purpose: the source file names (DealerProfile.cpp / DealerProfileSettings.h) and the lsFlexLevels\DealerProfile files, so his
  settings and records carry over.
- test_dealerprofile_2620.cpp 37 / 37 with -DDP_LIMIT_NAME=1 (header LP) and without; settings contract PASS; MinGW real-SDK syntax 0 errors.
- Not verified: whether any chart still holds the OLD lsDealerProfile indicator - such a chart shows it missing after the move; add "Limit Profile" there.

# 2.63.0 (2026-11-10 22:16 CT) - the Limit Profile reads the order feed itself
Rassul (his ES chart had no nodes: lsBookFlow was not on it): "it needs to read the feed directly".
- LimitFeed.h 1.0.0: one DEPTH_BY_ORDER per chart (kept in the chart's state, deleted when the chart / indicator goes), opened with a crash-guard
  file, read from the draw (<= 5,000 events or ~8 ms a call), never re-entered after a fault, re-opened after 2 min of silence while the chart
  trades; fed to tbk::Engine (TopBook.h, the engine lsBookFlow runs), evaluated once a second into the same facts as SignalBoard kind G.
- DealerProfile.cpp: the own feed is used as soon as it has its first event; lsBookFlow's kind G only before that / if the feed cannot open.
  Status lines OWN_FEED (state, events, via, faults, reconnects, capped slices, the TopBook line) and TOP_BOOK_G "OWN FEED".
- test_dealerprofile_2630.cpp 44 / 44 (with and without -DDP_LIMIT_NAME): mocked DEPTH_BY_ORDER - opened once, young level not shown, after
  2 min the nodes 505 (K) / 305 come from the own feed with no kind G, one feed object for 170 draws, cancelled level goes, chart removed =
  the feed object deleted. ASan / UBSan clean; MinGW real-SDK syntax 0 errors (LimitFeed.h undefines the Windows `near` / `far` macros before
  TopBook.h - DealerProfile.cpp only undefined `far`); settings contract PASS.
- NOT verified - needs his Investor/RT: the real feed (a second MBO subscription next to SR / Zone Defense on the same chart), CPU at the NQ
  open (no timer path: events are read on draws only), the 2-minute warm-up after every restart (the book starts empty).
