# lsMorrisRSI 0.1.1 (2026-11-20) - lighter 70 / 30 lines, no 50 line

Request (CT, 2026-11-20): 05:46 screenshot of the pane "Morris RSI v0.1.0 (20, 3): 14.1" - "make the red and green lines dashed and slightly thinner and get rid of the gray dashed line".

What changed
- MorrisRsiLogic.h: the 70 (red) and 30 (green) lines are drawn as short explicit dashes - DASH_ON 4 px on, DASH_OFF 4 px off, 1 px wide - each dash its own solid segment, so the look does not depend on IRT's pen-style handling (0.1.0 used one full-width 1 px P_DASH line). The grey 50 line is removed.
- MorrisRSI.cpp: lines drawn with a solid pen (dashes come from the draw list); version 0.1.1 in header, MRS_VERSION, description and setVersion.
- Not changed: the RSI curve (width 2, blue / green / red), Dv lines and tags, title, records, status.

Tests
- tests_morrisrsi/t_morrisrsi.cpp 28/28 (ASan / UBSan, -Werror): new checks - no grey / no 50 line; 70 / 30 drawn as 100+ dashes across the pane in red / green; every dash 1 px wide, <= 4 px long, with gaps; RSI curve still width 2 blue / red / green; a too-small pane draws nothing; title "Morris RSI v0.1.1".
- MinGW -fsyntax-only against the real irtsdk.h: 0 errors / warnings in MorrisRSI.cpp / MorrisRsiLogic.h (the same 28 SDK-only RT_LONG diagnostics as 0.1.0).

Risks / not verified
- Not seen on his chart yet: needs IRT closed (auto build / install) and reopened.
- A slightly higher draw count (~2 x pane width / 8 short segments per redraw); trivial cost.
