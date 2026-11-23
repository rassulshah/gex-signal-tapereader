#!/bin/bash
# Turn v1.0: rebuild and run every check (research machine: needs the MBO store and Friday's event files)
set -e
W=${TURN_WORK:-/home/claude/turn/work}; P=${TURN_PLUGIN:-/home/claude/turn/plugin}; N=${TURN_NIGHTLY:-/home/claude/turn/nightly}; T=$P/tests_turn
STORE=/mnt/user-data/uploads/level-reversal-analytics/data/mbo/store
export TMPDIR=/tmp/claude-0; mkdir -p $TMPDIR
CXX="g++ -std=c++17 -Wall -Wextra -Werror"; SAN="-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=undefined"
echo "== 0 syntax against the REAL irtsdk.h, C++14 (the PC compiler's default)"
g++ -std=c++14 -fsyntax-only -Wall -Wextra -I/mnt/user-data/uploads/sdk/c++/include -I$P $T/wrap_real_sdk.cpp > $TMPDIR/realsdk.log 2>&1; echo "exit $? errors $(grep -c error $TMPDIR/realsdk.log) warnings outside the SDK $(grep warning $TMPDIR/realsdk.log | grep -vc 'irtsdk.h:')"
g++ -std=c++14 -DLSPERF_FAKE -Wall -Wextra -Werror -I$T/mock -I$P $T/compile_only.cpp -o $TMPDIR/turn_compile && echo "mock SDK compile C++14 -Werror ok"
echo "== 1 logic unit tests (ASan / UBSan)"; $CXX $SAN $T/test_turn.cpp -o $T/t_logic && $T/t_logic
echo "== 2 the nightly: seed from the reviewed reference, then two nights"
rm -rf $W/repo $W/repo20 $W/flex $W/flex20 $W/an; mkdir -p $W/repo/data/turn_pane $W/repo20/data/turn_pane $W/flex/RevRead $W/flex20/RevRead $W/an/lra
cp $N/turn_pane.py $W/an/lra/; touch $W/an/lra/__init__.py
cp /mnt/user-data/uploads/lsFlexLevels/RevRead/wymarks-*.txt /mnt/user-data/uploads/lsFlexLevels/RevRead/wy-*.txt $W/flex/RevRead/
(cd $W && python3 build_seed.py | tail -5)
cp $W/repo/data/turn_pane/seed_sweeps.csv $W/repo/data/turn_pane/norms_seed.csv $W/repo20/data/turn_pane/
(cd $W && python3 run_nightly.py $W/repo20 $W/flex20 2026-11-20 && python3 run_nightly.py $W/repo $W/flex 2026-11-23)
echo "== 3 the plugin parses every params file"; $CXX -O2 $T/replay.cpp -o $T/replay; echo "1 3 1 100 1 1 2" > $W/one.txt
for m in ES NQ CL GC HG NG EU; do $T/replay $W/one.txt --params $W/flex/LRA-TurnPane-$m.csv $m | tail -1; done
echo "== 4 golden candles through the C++"; python3 $T/golden.py $W/an $W/repo/data/turn_pane $STORE $TMPDIR/turn_golden | tail -2 | cut -c1-200
echo "== 5 the whole plugin on the mock SDK (live morning, ASan / UBSan / LSan)"
python3 $T/prep_host.py $W/hostdata /tmp/tr/plugin/tests_erpane $W/flex20 $W/flex /mnt/user-data/uploads/lsFlexLevels/RevRead > /dev/null
(cd $T && $CXX $SAN -DLSPERF_FAKE -Imock -I.. test_turn_host.cpp -o t_host && TURN_TEST_DATA=$W/hostdata TURN_DUMP=$W/dump ./t_host | grep -a "strip\|narrow\|nightly:\|new Sw\|scrolled\|small pane\|checks\|FAIL\|overlap")
echo "== 6 soak: 40 cycles plain, 8 under ASan / LSan"
(cd $T && $CXX -O2 -DLSPERF_FAKE -Imock -I.. soak.cpp -o soak_plain && TURN_TEST_DATA=$W/hostdata ./soak_plain 40 | tail -2 && $CXX $SAN -DLSPERF_FAKE -Imock -I.. soak.cpp -o soak_asan && TURN_TEST_DATA=$W/hostdata ./soak_asan 8 | tail -2)
echo "== 7 the nightly's unit tests"; if [ -f $N/test_turn_pane.py ]; then (cd $W && cp $N/test_turn_pane.py an/ && LRA_MBO_STORE=$STORE python3 an/test_turn_pane.py); fi
echo ALL DONE
