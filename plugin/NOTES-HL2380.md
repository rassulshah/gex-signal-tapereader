# lsHodLod 2.38.0 - the Now line says what the liquidity at its level is doing (2026-11-20)

## Request
- Rassul 06:04 CT 2026-11-20 "ok lets do this" (mockup now-box-levels-mockup.html, the gold PROPOSED 06:02 rows): after lsLimitProfile 2.68.0
  publishes its level liquidity file, the Now line adds ONE short liquidity phrase for the level the badge names - only the states that
  PASSED the Limit Profile's scoring.

## What changed
- NowLevels.h 1.3.0: parseLiq() reads lsFlexLevels\DealerProfile\levelliq-<MKT>-<per>.txt (header LEVELLIQ,1 / MARKET / SPB / STAMP, rows
  "L|name|level_tick|B or O|state|shown|since|conf"); wrong market / bar size, garbled header, > 64 KiB, or a STAMP more than 2 min from now =
  no phrase. Only rows with shown = 1 count (lsLimitProfile 2.68.0 draws H only, so today the phrase can only be "holding").
- The phrase sits right after the badge: "[TESTING: ONH Bk / Sw] | Bids holding | Next ...", "[WATCH: ALO ↓ · ONH ↑ · Bk / Sw] | ALO bids
  leaving" (a badge naming two levels names the level). Words: building / leaving / holding (X or H) / pulled (P). Label grey; the word green
  for bids / red for offers when it helps the defence (building, holding), the attacker's colour when it fails (bids leaving / pulled = red,
  offers leaving / pulled = green). A merged row (PFL·PDL) names each level; two rows for one level: the newest shown one.
- Narrow pane: the phrase drops FIRST (layout level 3), then the Last Bk part (level 1, unchanged), then pieces off the right end.
- Host: reads the file at most every 3 s (hlSmallRead, 64 KiB cap, missing = cleared), judges freshness on every 1-s run with hlNowCt();
  status NOW_LEVELS gains ",liq <ok / stale / no file / other market ...> rows N phrase 0/1" and "liquidity phrase dropped".
- Versions: HL_VERSION = setVersion = PLUGIN_VERSION = 2.38.0 + header comment; NowLevels 1.3.0. No settings change.

## Tests
- New tests_hodlod/t_hl2380.cpp: 35 / 35 (ASan / UBSan, -Werror): every state x side with its colour, unshown and steady = none, another
  level = none, no file = the 2.37.2 line unchanged, stale 121 s / fresh 119 s, missing, garbled, other bar size, bad rows, oversized, future
  stamp, WATCH naming "ALO bids leaving", merged rows, newest row wins, narrow pane drops the phrase first, ES file never on NQ, host source,
  versions.
- t_hl2360 99 / 3, t_hl2372 40 / 4, t_hl2371 42 / 4, t_hl2370: the failures are the old version-string checks (2.37.x) and the gex-build.bat
  path checks (the container overlay has no tools folder); no behaviour check failed. Old-version checks were left as they are.
- MinGW -fsyntax-only vs the real irtsdk.h: 0 errors; only the old first_extreme_inference.cpp warning.

## Not verified
- On his IRT: needs lsLimitProfile 2.68.0 + lsHodLod 2.38.0 installed (exit IRT), then a screenshot when price tests a lit level.
