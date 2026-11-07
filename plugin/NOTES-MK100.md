# MarketKey.h 1.0.0 (2026-11-06 19:10 CT)

Rassul: "have you made this mistake on other markets also?", "these are from rithmic but the backfill is from iqfeed", "fix it".

His GC / HG / CL / NG charts report the IQFeed-style roots QGC / QHG / QCL and NGX6. Several indicators only knew the CQG roots, or none:
| Indicator | Before | Now |
|---|---|---|
| CoB Signal | never ran on GC / HG / CL / NG | 1.6.2 runs (mkey::norm) |
| BO (Breakout / Breakout1) | ES hours 08:30-15:00 on GC / HG / CL / NG | 1.9.4: the market's own RTH (GC 07:20, HG 07:10, CL / NG 08:00) |
| ZZO Combo / Delta / Volume / ZigZag | no RTH hours (wrong session split for the wave normals and counts), no market params | Combo 2.2.1, Delta 1.3.3, Volume 1.2.3, ZigZag 1.2.1 |

The shared lookup accepts IQFeed (@ES, QGC ...), Rithmic (ES, GC, 6E ...), CQG (EP, GCE, CPE, EU6 ...) and contract-style roots (NGX6, GCZ6).
Test: tmk.cpp 24/24 roots; MinGW 0 errors on all seven plugins; BO tests 17/17 + 25/25; CoB Signal fib 29/29, LVP 30/30.

Consequence: ZZO wave counts on GC / HG / CL / NG change, because the RTH waves are now compared with RTH waves of the right session.
GC Nov 6: the 09:18 up wave counted 50+ with the ES hours; with GC hours its ratio (2.7x) is below the 08:33 wave's (3.2x) -> count 0.
