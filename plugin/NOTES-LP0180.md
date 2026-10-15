# lsLiquidityProfile 0.18.0 (2026-10-14/15 night)

## Requests
- 22:11 CT, approved mockup v2 (mockup-big-resting-orders.png): the BIGGEST RESTING ORDER LINES on the price chart - the single biggest
  displayed bid within 2 ATR below price (green) and offer within 2 ATR above (red), one MBO order each, labelled "Bid 120" / "Ask 95",
  extending while it rests, ending once with a dot (filled) / X (pulled) / arrow (resting) / plain end; never repaints; hover with
  size, price, appeared, rested, traded vs cancelled, what / how / use / example and the displayed-size limit.
- 22:48 CT, approved mockup v5 (mockup-lp-warning-v5.png): the top warning's grey detail on TWO lines, one per side, never cut off.

## Where and why
Inside lsLiquidityProfile: it already draws on the PRICE pane (OVERLAY; its column is the left edge of that pane) and already holds the
chart's order-by-order feed (OrderFlowFeed.h off::Conn - one DEPTH_BY_ORDER per chart). A second plugin would open a second
DEPTH_BY_ORDER subscription on the same chart. The logic is SDK-free in BigOrderLines.h 1.0.0 (tested + replayed); the host part is
~250 lines in LiquidityProfile.cpp (loadBig / boSessionCheck / boAfter / boFlush / renderBig / renderBigHover + the feed hook,
the mouse, the status line BIG_ORDERS). lsLiquidityFlow shares the source: it only takes the version number.

## Rules (decided from his recorded order files, data\mbo\live)
- Minimum = max(market floor, the session's own top 0.1% of displayed NEW sizes so far, used from 1,000 orders on; past-only).
  Floors: ES 30, NQ 10, GC 15, HG 12, CL 15, NG 10, 6E 50 (others 20). Setting "Big order min lots" overrides per market.
- Reach = 2.0 x ATR14 of the chart's closed bars (setting 0.5-5). 1 tick of spread tolerance on the wrong side.
- Dwell 10 s (or a trade first) before a line draws; out of reach 60 s beyond 1.25 x the reach = LEFT; takeover only strictly bigger.
- A line that rested when IRT closed ends at the last heartbeat ("after <time> not known"); an order first seen after a restart is
  "since restart, start not known". The depth book showing less than the order at its price for 5 s ends a line "not seen" (the feed
  missed its exit - LP drains in calc / draw, lesson 59).
- Record: lsFlexLevels\LiquidityProfile\<MKT>-<SYM>-<spb>-bigorders-<session>.csv (S / E / H rows), settings
  lsFlexLevels\LiquidityProfile.bigorders-<MKT>.txt "BOL1,on,reach x10,min lots (0 = market),kept,font".
- Settings rows 5-9 (setParameterVersion 3 -> 4: IRT resets the saved rows once; the files keep them).

## Replay on recorded data (replay_bigorders.cpp, the shipped engine, 3-min bars from the fills)
ES 10/14 06:57-16:45: 133 lines (74 bid / 59 offer), max 20 in an hour (09:00); FILLED 10, PULLED 8, moved 1, taken over 59, left
reach 53, resting at the end 2. NQ 10/12: 105 (25 filled). GC 10/12: 78 (37 filled). HG 10/12: 24. 6E 10/12: 24. CL: too little
recorded (0 lines). Render from the real layout: render-es1014-bigorders.png (ES 08:15-10:00, 16 px a bar).

## Also fixed
- 0.17.0 attempt rows without an ADR had 4 zone-rule fields under a 5-column header (lesson 89).
- 6 stale Liquidity suites updated to 0.17.0's intended wording / rules (lesson 90).

## Tests
tests\lp-0.18.0-bigorders-tests.tar.gz: 10 suites, 674 checks, 0 failed under ASan/UBSan (-Werror on the logic suites); MinGW
syntax check against the real irtsdk.h 0 errors / 0 warnings (Profile + Flow); Win32 libs ok. MSVC on his PC: clean (23:38 CT).

## Not verified - needs his IRT
The drawing on his real charts (screenshots after the restart in 2+ markets), IRT's float row in the settings window, the live feed's
FILL side (the recorder writes what IRT hands over, so it should match the files).
