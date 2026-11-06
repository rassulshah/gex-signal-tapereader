# lsCobSignal 1.5.1 (2026-11-06 12:13 CT)

Rassul (12:11 CT, HG 3-min screenshot): "why is there no cob signal", then "why isnt' the cob there ?"

## Cause
His IRT feed is CQG, so a chart's ROOT symbol is EP / ENQ / CLE / GCE / CPE / NGE / EU6, not ES / NQ / CL / GC / HG / NG / EU.

- lsCobSignal 1.0.0 - 1.5.0 passed the root straight to svl::rthOf().
- svl::rthOf() did not know the CQG roots, so identify() returned false.
- Result: the indicator never ran on any of his charts. It drew no CoB and wrote no status, record or live file. lsFlexLevels had no CobSignal* file at all.
- The tests never caught it because they set root "ES". The mock SDK's own default ("EP") was the real case.

## Fix
- `cobs::mktKey` now maps the CQG roots: EP→ES, ENQ→NQ, CLE→CL, GCE→GC, CPE→HG, NGE→NG, EU6→EU (and 6E→EU). This is the same table lsBigBidAsk and lsDealerHedge use.
- Nothing else changed.

## Tests
| Test | Result |
|---|---|
| test_cobsignal_host (+ check: each CQG root runs as its market and writes its status file) | 35 passed |
| test_cobsignal_fib | 29 passed |
| test_cobsignal_lvp | 30 passed |
| MinGW against the real irtsdk.h | 0 errors, 0 warnings |
