#!/bin/bash
# lsTurn 1.2.0: rebuild and run every check (research machine: needs the MBO store and Friday's event files; E = the work tree with
# plugin/ (this version), base110/ (1.1.0), optE/ (the mockup-only variant he approved), mock/, data/ev_ES.txt, data/eff_ref_ES.csv)
set -e
E=/home/claude/eff; W=/home/claude/turn/work; P=$E/plugin; T=$P/tests_turn; H=$W/hostdata
STORE=/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store
export TMPDIR=/tmp/claude-0; mkdir -p $TMPDIR $E/out/v12
CXX="g++ -std=c++17 -Wall -Wextra -Werror"; SAN="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined"
echo "== 0 syntax against the REAL irtsdk.h"
for std in c++14 c++17; do g++ -std=$std -fsyntax-only -Wall -Wextra -I/mnt/user-data/uploads/sdk/c++/include -I$P $T/wrap_real_sdk.cpp > $TMPDIR/realsdk.log 2>&1; echo "$std exit $? errors $(grep -c error $TMPDIR/realsdk.log) warnings outside the SDK $(grep warning $TMPDIR/realsdk.log | grep -vc 'irtsdk.h:')"; done
g++ -std=c++14 -DLSPERF_FAKE -Wall -Wextra -Werror -I$T/mock -I$P $T/compile_only.cpp -o $TMPDIR/turn_compile && echo "mock SDK compile C++14 -Werror ok"
echo "== 1 logic unit tests (ASan / UBSan), C++17 and C++14"; $CXX $SAN $T/test_turn.cpp -o $T/t_logic && $T/t_logic | tail -1; g++ -std=c++14 -Wall -Wextra -Werror -O2 $T/test_turn.cpp -o $TMPDIR/t_logic14 && $TMPDIR/t_logic14 | tail -1
echo "== 2 golden candles through the C++"; $CXX -O2 $T/replay.cpp -o $T/replay; python3 $T/golden.py $W/an $W/repo/data/turn_pane $STORE $TMPDIR/turn_golden | tail -2 | head -1
echo "== 3 the whole plugin on the mock SDK (the saved full setting, the Effort-only default, one piece, garbled files; ASan / UBSan / LSan)"
(cd $T && $CXX $SAN -DLSPERF_FAKE -Imock -I.. test_turn_host.cpp -o t_host && TURN_TEST_DATA=$H ./t_host 2>&1 | grep -a "JUST EFFORT\|checks\|FAIL\|overlap\|LeakSanitizer\|ERROR")
echo "== 4 soak: 40 cycles plain, 8 under ASan / LSan (every other cycle Effort only, the others the full setting)"
(cd $T && $CXX -O2 -DLSPERF_FAKE -Imock -I.. soak.cpp -o soak_plain && TURN_TEST_DATA=$H ./soak_plain 40 | tail -2 && $CXX $SAN -DLSPERF_FAKE -Imock -I.. soak.cpp -o soak_asan && TURN_TEST_DATA=$H ./soak_asan 8 2>&1 | grep -a "checks\|rss after\|LeakSanitizer\|ERROR")
echo "== 5 Friday's ES morning 8:24 - 10:33 AM (44 candles): the default = the approved mockup; = the independent calculation at five zooms; restart; the full setting = 1.1.0"
cd $E; printf 'TURNSHOW|1\nstop|1\nexhaustion|1\nanswer|1\nread|1\neffort_from|0.5\nEND\n' > data/display-full.txt
g++ -std=c++17 -O2 -DLSPERF_FAKE -w -I$T/mock -I$P mock/effmock.cpp -o mock/effmock_new; g++ -std=c++17 -O2 -DLSPERF_FAKE -w -I$T/mock -Ibase110 mock/effmock.cpp -o mock/effmock_110; g++ -std=c++17 -O2 -DLSPERF_FAKE -w -I$T/mock -IoptE mock/effmock.cpp -o mock/effmock_e
./mock/effmock_e data/ev_ES.txt $H out/e 22 170 34 0 > /dev/null; ./mock/effmock_new data/ev_ES.txt $H out/v12/def 22 170 34 0 > /dev/null
cmp out/v12/def_draws.txt out/e_draws.txt && cmp out/v12/def_mouse.txt out/e_mouse.txt && echo "1.2.0 default = the mockup he approved, draw for draw (no mouse, and the mouse on 10:06 AM)"
for z in 60 22 12 6 3; do EFF_RESTART=1 ./mock/effmock_new data/ev_ES.txt $H out/v12/def_$z $z 170 | grep -a restart | sed "s/^/default $z px: /"; done | sort | uniq -c -f3; python3 mock/only.py
for z in 60 22 12 6 3; do ./mock/effmock_110 data/ev_ES.txt $H out/v12/v110_$z $z 170 21 0 > /dev/null; EFF_SHOW=data/display-full.txt ./mock/effmock_new data/ev_ES.txt $H out/v12/full_$z $z 170 21 0 > /dev/null; done; python3 mock/full_vs_110.py
for cfg in "300 22" "1000 6" "3000 3"; do set -- $cfg; EFF_PERF=$1 ./mock/effmock_new data/ev_ES.txt $H out/v12/perf $2 170 | grep perf; done
echo ALL DONE
