# Large-lot thresholds (LMT R / LMT F / MKT footers and tiers) - the ONE method
Rassul 2026-10-15 14:52-15:00 and 2026-10-16 (LP 0.38.0, BigOrderLines 1.5.0, BigFlow 0.38.0).

1. ISOLATE large orders first: resting orders (LMT) at or above the market's start size; big market orders (MKT) at or above the session's 99th percentile (never under 10 lots for the buckets).
   Start floors (lots), LMT: ES 30, NQ 10, GC 15, HG 12, CL 15, NG 10, 6E 50 (the chart's own adaptive start is used when higher; ES ~67).
2. DISTRIBUTE them on the log scale: 48 buckets (10 lots x 1.15^k); mean m and SD of ln(size). Footer: mu = e^m, 1SD = e^(m+sd), 2SD = e^(m+2sd), 3SD = e^(m+3sd); the top tier "2.7" = e^(m+2.66sd) capped at 3SD.
3. POOL the current session with the 4 earlier sessions (5 sessions, one pooled distribution; not an average of per-session values).
4. Three channels, kept in files: all hours, RTH (8:30-15:00 CT), overnight. The channel of the current period is used once it has 30+ large orders; until then all hours; until then the session's own live stats / "learning".
5. Same method for LMT and MKT. Files in lsFlexLevels: LiquidityProfile.biglots-<MKT>[-RTH|-ON].txt (LMT), LiquidityProfile.mktlots-<MKT>[-RTH|-ON].txt (MKT), BLD1 lines, one per session.
6. The nightly study rows (LRA-LmtNorms / LRA-MktNorms) no longer override the footer. ES reference: mu 139 / 1SD 269 / 2SD 520 / 3SD 1006.
Note: period channels start filling from 0.38.0 forward (earlier sessions were stored all-hours only), so RTH / overnight use all hours until they reach 30 orders.
