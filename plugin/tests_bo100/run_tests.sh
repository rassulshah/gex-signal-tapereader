#!/bin/sh
# (1.9.0) + test_bo190_rev (Rev: rule re-check, no repaint, layout, BRV records); version pins 1.9.0
# lsBreakout 1.8.0 tests (2026-11-05 Pb overlap + max 2 + green-close pivots: test_bo180_pb; 2026-11-05 overnight Rly from 02:00 CT: test_bo170_onrly; 2026-11-04 lsBreakout1 "BO 1-bar" = Breakout1.cpp: test_bo161_onebar [1-bar pivots == independent walk on 15-min ES / NQ / GC, Pb rule, no repaint, wrapper] + MinGW of wrap_Breakout1.cpp; 1.6.0: BI removed + Pb within 1/3 ATR of the broken pivot: test_bo160_pb [bo_pb 1.3.0 parity ES / NQ / GC, the rule, no BI, no repaint]; test_bo110_pb (the 1.1.0 overlap rule) and test_bo150_bi (BI) are retired - kept in the folder, not run; test_bo120_week checks BO / BD + Rly unchanged (Pb lines skipped); 1.5.0 BI = BreakIn: test_bo150_bi [prototype parity, cancel, reference cases, no repaint, labels, records]; the older suites run biCancel = false = the 1.4.0 decisions; 1.4.0 zigzag carry across the 17:00 CT break: test_bo120_week [6] + carry expectations; 1.3.0 records + follow-through tie: test_bo130_rec, test_bo120_week [4] / [5]; 1.0.0 + Pb: test_bo110_pb; 1.1.1 Pb two rows out; 1.2.0 3-bar ZigZag: test_bo120_week, ~3 min) (Linux, from plugin/tests_bo100): parity with lsBigBidAsk (ES Oct 30 records) + the host on the mock SDK under
# ASan / UBSan, then (with the real irtsdk.h folder as $1) the MinGW syntax check of Breakout.cpp against the REAL SDK, and the render.
set -e
CXX0="g++ -std=c++17 -DBKO_RLY_ON_DEFAULT=0 -fsanitize=address,undefined -fno-sanitize-recover=all -Werror -Wall -Wextra -g"   # (1.7.0) the 1.6.0 suites: RTH-only Rly
CXX="g++ -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -Werror -Wall -Wextra -g"
$CXX0 -I.. -o /tmp/bo100_parity test_bo100_parity.cpp && /tmp/bo100_parity
$CXX0 -O1 -I.. -o /tmp/bo120_week test_bo120_week.cpp && /tmp/bo120_week
$CXX -O1 -I.. -o /tmp/bo180_pb test_bo180_pb.cpp && /tmp/bo180_pb
$CXX -O1 -I.. -o /tmp/bo1140_entry test_bo1140_entry.cpp && /tmp/bo1140_entry   # (1.14.0) fact kind E = only drawn decided Pb / Pboc / Pdoc, none hidden / provisional, no repaint (real ES 11-09 / 10-21 / 11-04)
$CXX -O1 -I.. -o /tmp/bo1160_retrace test_bo1160_retrace.cpp && /tmp/bo1160_retrace   # (1.16.0) every Pb / Pboc / Pdoc (BO / BD and Rly) pivot >= 38.2% of its wave, incl. the no-VAP fallback
$CXX -O1 -I.. -o /tmp/bo1130_pdoc test_bo1130_pdoc.cpp && /tmp/bo1130_pdoc   # (1.13.0) Pdoc + Pboc (replaces test_bo1120_pboc: 00:45 / 08:30 are Pdoc now), real ES 11-09 / 10-21 / 11-04
$CXX -O1 -I.. -o /tmp/bo190_rev test_bo190_rev.cpp && /tmp/bo190_rev   # (1.9.0) Rev   # (1.8.0) Pb: overlap the BO candle, max 2, green / red close pivots (test_bo160_pb = the 1.6.0 rule, retired: kept, not run)
$CXX -O1 -I.. -o /tmp/bo161_onebar test_bo161_onebar.cpp && /tmp/bo161_onebar
$CXX -O1 -I.. -o /tmp/bo170_onrly test_bo170_onrly.cpp && /tmp/bo170_onrly   # (1.7.0) overnight Rly
$CXX0 -IBreakout_test_support -I.. -o /tmp/bo100_host test_bo100_host.cpp && /tmp/bo100_host
$CXX0 -IBreakout_test_support -I.. -o /tmp/bo130_rec test_bo130_rec.cpp && /tmp/bo130_rec
g++ -std=c++17 -IBreakout_test_support -I.. -o /tmp/bo100_rd render_dump.cpp && /tmp/bo100_rd > /tmp/bo100_dump.txt && python3 render_check.py /tmp/bo100_dump.txt
if [ -n "$1" ] && command -v x86_64-w64-mingw32-g++ >/dev/null; then
  D=$(mktemp -d); cp "$1/irtsdk.h" wrap_Breakout.cpp "$D/"
  x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -I"$D" -I.. "$D/wrap_Breakout.cpp" > "$D/out.txt" 2>&1
  E=$(grep -c "error" "$D/out.txt" || true); W=$(grep "warning" "$D/out.txt" | grep -vc "irtsdk.h" || true)
  echo "MinGW vs real irtsdk.h: $E errors, $W warnings outside the SDK"; rm -rf "$D"; [ "$E" = "0" ] && [ "$W" = "0" ]
  D=$(mktemp -d); cp "$1/irtsdk.h" "$D/"; sed 's/Breakout.cpp/Breakout1.cpp/' wrap_Breakout.cpp > "$D/wrap_Breakout1.cpp"
  x86_64-w64-mingw32-g++ -std=c++17 -fsyntax-only -Wall -Wextra -I"$D" -I.. "$D/wrap_Breakout1.cpp" > "$D/out.txt" 2>&1
  E=$(grep -c "error" "$D/out.txt" || true); W=$(grep "warning" "$D/out.txt" | grep -vc "irtsdk.h" || true)
  echo "MinGW lsBreakout1 vs real irtsdk.h: $E errors, $W warnings outside the SDK"; rm -rf "$D"; [ "$E" = "0" ] && [ "$W" = "0" ]
fi
