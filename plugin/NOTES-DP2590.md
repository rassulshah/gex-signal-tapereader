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

# 2.60.0 (2026-11-10 20:57 CT) - liquidity on the level lines
Rassul: "we have dim lines right for the levels" / "how would it look if the liquidity was also there on the chart" / "i like it but dont keep the
amount, because it becomes noisy" (mockup claude.ai/artifact/Fx5CKJkC1mHzj9uyUwWTvn v3).
- While a node carries a level's badge (the largest node within 1 ATR of that level), the level's dim line is drawn BRIGHT (2 px, green = bids,
  red = offers) over those candles. No number on the chart. A stretch ends 3 minutes after the node was last seen; stretches are kept for the
  session while Investor/RT stays open (not stored: after a restart the lines start dim again).
- test_dealerprofile_2600.cpp 34 / 34 (2.59.0's 32 + the bright stretch on the level's y in the side colour with no amount; the stretch stays
  after the node leaves). ASan / UBSan clean, MinGW real-SDK syntax 0 errors, settings contract PASS.
- Not verified on his chart: the look across many candles; the mockup used $-bands from the whole recorded book, the live rule uses his top-book
  nodes (3 bids + 3 offers), so fewer stretches than the mockup are possible.
