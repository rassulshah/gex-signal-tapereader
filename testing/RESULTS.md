# Regression results — all indicators

_Appended by `tools/regress.py` on every run (one block per run, newest at the bottom). Per-indicator detail and the live same-moment runs: `testing/REGRESSION.md`._

## 2026-09-17 03:29  ·  gamma daymodel daystats kingtracker  ·  panel 16.37 / companion 1.21 / GP 0.56 / DM 0.17 / DS 0.8 / KT 0.10  ·  ALL GREEN

_first recorded run of the all-indicator regression (v16.37 build)_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 65/65 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 57/57 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| daymodel | A | `test_daymodel_em.js` | 34/34 | ok |
| daymodel | A | `test_day_export.js` | 38/38 | ok |
| daymodel | A | `test_hodlod.js` | 176/176 | ok |
| daymodel | B | `test_daymodel_logic.cpp` | 23/23 | ok |
| daymodel | L | `synth-1033 (live runner)` | 10/10 | ok |
| daystats | A | `test_daystats_cond.js` | 27/27 | ok |
| daystats | A | `test_day_export.js` | 38/38 | ok |
| daystats | A | `test_hodlod.js` | 176/176 | ok |
| daystats | B | `test_daystats_logic.cpp` | 19/19 | ok |
| daystats | L | `synth-1033 (live runner)` | 9/9 | ok |
| kingtracker | A | `test_kingtracker_rows.js` | 21/21 | ok |
| kingtracker | B | `test_kingtracker_logic.cpp` | 16/16 | ok |
| kingtracker | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| kingtracker | L | `synth-1033 (live runner)` | 7/7 | ok |

_host: vm, C++: g++_

## 2026-09-17 05:39  ·  gamma daymodel daystats kingtracker  ·  panel 16.38 / companion 1.21 / GP 0.56 / DM 0.17 / DS 0.8 / KT 0.11  ·  ALL GREEN

_v16.38 / KT 0.11: the IF Magnet source on lsKingTracker_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 65/65 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 57/57 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| daymodel | A | `test_daymodel_em.js` | 34/34 | ok |
| daymodel | A | `test_day_export.js` | 38/38 | ok |
| daymodel | A | `test_hodlod.js` | 176/176 | ok |
| daymodel | B | `test_daymodel_logic.cpp` | 23/23 | ok |
| daymodel | L | `synth-1033 (live runner)` | 10/10 | ok |
| daystats | A | `test_daystats_cond.js` | 27/27 | ok |
| daystats | A | `test_day_export.js` | 38/38 | ok |
| daystats | A | `test_hodlod.js` | 176/176 | ok |
| daystats | B | `test_daystats_logic.cpp` | 19/19 | ok |
| daystats | L | `synth-1033 (live runner)` | 9/9 | ok |
| kingtracker | A | `test_kingtracker_rows.js` | 32/32 | ok |
| kingtracker | B | `test_kingtracker_logic.cpp` | 21/21 | ok |
| kingtracker | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| kingtracker | L | `synth-1033 (live runner)` | 7/7 | ok |

_host: vm, C++: g++_

## 2026-09-17 05:55  ·  gamma daymodel daystats kingtracker  ·  panel 16.38 / companion 1.21 / GP 0.57 / DM 0.18 / DS 0.9 / KT 0.12  ·  ALL GREEN

_KT 0.12 Source first + greying; the font guard widened in all four plugins_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 65/65 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 57/57 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |
| daymodel | A | `test_daymodel_em.js` | 34/34 | ok |
| daymodel | A | `test_day_export.js` | 38/38 | ok |
| daymodel | A | `test_hodlod.js` | 176/176 | ok |
| daymodel | B | `test_daymodel_logic.cpp` | 23/23 | ok |
| daymodel | L | `synth-1033 (live runner)` | 10/10 | ok |
| daymodel | S | `DayModel.cpp` | ok | ok |
| daystats | A | `test_daystats_cond.js` | 27/27 | ok |
| daystats | A | `test_day_export.js` | 38/38 | ok |
| daystats | A | `test_hodlod.js` | 176/176 | ok |
| daystats | B | `test_daystats_logic.cpp` | 19/19 | ok |
| daystats | L | `synth-1033 (live runner)` | 9/9 | ok |
| daystats | S | `DayStats.cpp` | ok | ok |
| kingtracker | A | `test_kingtracker_rows.js` | 32/32 | ok |
| kingtracker | B | `test_kingtracker_logic.cpp` | 21/21 | ok |
| kingtracker | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| kingtracker | L | `synth-1033 (live runner)` | 7/7 | ok |
| kingtracker | S | `KingTracker.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 06:16  ·  gamma daymodel daystats kingtracker  ·  panel 16.39 / companion 1.21 / GP 0.57 / DM 0.18 / DS 0.9 / KT 0.13  ·  ALL GREEN

_v16.39 / KT 0.13: polarity per step, IF default, the 0.12 scramble repair_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 65/65 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 57/57 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| daymodel | A | `test_daymodel_em.js` | 34/34 | ok |
| daymodel | A | `test_day_export.js` | 38/38 | ok |
| daymodel | A | `test_hodlod.js` | 176/176 | ok |
| daymodel | B | `test_daymodel_logic.cpp` | 23/23 | ok |
| daymodel | L | `synth-1033 (live runner)` | 10/10 | ok |
| daystats | A | `test_daystats_cond.js` | 27/27 | ok |
| daystats | A | `test_day_export.js` | 38/38 | ok |
| daystats | A | `test_hodlod.js` | 176/176 | ok |
| daystats | B | `test_daystats_logic.cpp` | 19/19 | ok |
| daystats | L | `synth-1033 (live runner)` | 9/9 | ok |
| kingtracker | A | `test_kingtracker_rows.js` | 33/33 | ok |
| kingtracker | B | `test_kingtracker_logic.cpp` | 26/26 | ok |
| kingtracker | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| kingtracker | L | `synth-1033 (live runner)` | 7/7 | ok |

_host: vm, C++: g++_

## 2026-09-17 06:22  ·  gamma  ·  panel 16.39 / companion 1.21 / GP 0.58 / DM 0.18 / DS 0.9 / KT 0.13  ·  ALL GREEN

_GP 0.58: the chip sized to its content, tight pills_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 65/65 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 57/57 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 14:35  ·  kingtracker  ·  panel 16.39 / companion 1.21 / GP 0.58 / DM 0.18 / DS 0.9 / KT 0.14  ·  ALL GREEN

_KT 0.14: the status file (toggle test hook)_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| kingtracker | A | `test_kingtracker_rows.js` | 33/33 | ok |
| kingtracker | B | `test_kingtracker_logic.cpp` | 30/30 | ok |
| kingtracker | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| kingtracker | L | `synth-1033 (live runner)` | 7/7 | ok |

_host: vm, C++: g++_

## 2026-09-17 14:48  ·  kingtracker  ·  panel 16.39 / companion 1.21 / GP 0.58 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

_KT 0.15: black colour rows repaired in the dialog_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| kingtracker | A | `test_kingtracker_rows.js` | 33/33 | ok |
| kingtracker | B | `test_kingtracker_logic.cpp` | 30/30 | ok |
| kingtracker | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| kingtracker | L | `synth-1033 (live runner)` | 7/7 | ok |
| kingtracker | S | `KingTracker.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 15:53  ·  gamma daymodel daystats kingtracker  ·  panel 16.40 / companion 1.21 / GP 0.59 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

_v16.40 / GP 0.59: the SPY book + the Atlas pool; Rank = Atlas merge_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 78/78 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 63/63 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |
| daymodel | A | `test_daymodel_em.js` | 34/34 | ok |
| daymodel | A | `test_day_export.js` | 38/38 | ok |
| daymodel | A | `test_hodlod.js` | 176/176 | ok |
| daymodel | B | `test_daymodel_logic.cpp` | 23/23 | ok |
| daymodel | L | `synth-1033 (live runner)` | 10/10 | ok |
| daymodel | S | `DayModel.cpp` | ok | ok |
| daystats | A | `test_daystats_cond.js` | 27/27 | ok |
| daystats | A | `test_day_export.js` | 38/38 | ok |
| daystats | A | `test_hodlod.js` | 176/176 | ok |
| daystats | B | `test_daystats_logic.cpp` | 19/19 | ok |
| daystats | L | `synth-1033 (live runner)` | 9/9 | ok |
| daystats | S | `DayStats.cpp` | ok | ok |
| kingtracker | A | `test_kingtracker_rows.js` | 33/33 | ok |
| kingtracker | B | `test_kingtracker_logic.cpp` | 30/30 | ok |
| kingtracker | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| kingtracker | L | `synth-1033 (live runner)` | 7/7 | ok |
| kingtracker | S | `KingTracker.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 16:19  ·  gamma  ·  panel 16.40 / companion 1.21 / GP 0.59 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

_first real 16.40 export as fixtures (the pool on both files)_

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 78/78 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 63/63 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |

_host: vm, C++: g++_

## 2026-09-17 16:31  ·  gamma  ·  panel 16.40 / companion 1.21 / GP 0.60 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 78/78 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 67/67 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 16:47  ·  gamma  ·  panel 16.40 / companion 1.21 / GP 0.61 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 78/78 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 72/72 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 17:15  ·  gamma  ·  panel 16.40 / companion 1.21 / GP 0.62 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 78/78 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 75/75 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 17:36  ·  gamma  ·  panel 16.40 / companion 1.21 / GP 0.63 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 78/78 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 78/78 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 17:37  ·  gamma  ·  panel 16.40 / companion 1.21 / GP 0.63 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 78/78 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 78/78 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 20:53  ·  gamma  ·  panel 16.41 / companion 1.21 / GP 0.63 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 89/89 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 78/78 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |

_host: vm, C++: g++_

## 2026-09-17 20:56  ·  gamma  ·  panel 16.41 / companion 1.21 / GP 0.64 / DM 0.18 / DS 0.9 / KT 0.15  ·  1 suite(s) FAILED

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 89/89 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | rc=1 | **FAIL** |

_host: vm, C++: g++_

## 2026-09-17 20:56  ·  gamma  ·  panel 16.41 / companion 1.21 / GP 0.64 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 89/89 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 20:58  ·  gamma daymodel daystats kingtracker  ·  panel 16.41 / companion 1.21 / GP 0.64 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 89/89 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| daymodel | A | `test_daymodel_em.js` | 34/34 | ok |
| daymodel | A | `test_day_export.js` | 38/38 | ok |
| daymodel | A | `test_hodlod.js` | 176/176 | ok |
| daymodel | B | `test_daymodel_logic.cpp` | 23/23 | ok |
| daymodel | L | `synth-1033 (live runner)` | 10/10 | ok |
| daystats | A | `test_daystats_cond.js` | 27/27 | ok |
| daystats | A | `test_day_export.js` | 38/38 | ok |
| daystats | A | `test_hodlod.js` | 176/176 | ok |
| daystats | B | `test_daystats_logic.cpp` | 19/19 | ok |
| daystats | L | `synth-1033 (live runner)` | 9/9 | ok |
| kingtracker | A | `test_kingtracker_rows.js` | 33/33 | ok |
| kingtracker | B | `test_kingtracker_logic.cpp` | 30/30 | ok |
| kingtracker | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| kingtracker | L | `synth-1033 (live runner)` | 7/7 | ok |

_host: vm, C++: g++_

## 2026-09-17 22:06  ·  gamma  ·  panel 16.41 / companion 1.21 / GP 0.65 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 89/89 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 22:07  ·  gamma  ·  panel 16.41 / companion 1.21 / GP 0.66 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 89/89 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 22:16  ·  gamma  ·  panel 16.41 / companion 1.21 / GP 0.67 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 89/89 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-17 22:37  ·  gamma  ·  panel 16.42 / companion 1.21 / GP 0.67 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 89/89 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |

_host: vm, C++: g++_

## 2026-09-17 22:48  ·  gamma  ·  panel 16.43 / companion 1.21 / GP 0.67 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 103/103 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |

_host: vm, C++: g++_

## 2026-09-18 00:13  ·  gamma  ·  panel 16.44 / companion 1.21 / GP 0.67 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 105/105 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |

_host: vm, C++: g++_

## 2026-09-18 00:27  ·  gamma  ·  panel 16.44 / companion 1.21 / GP 0.68 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 105/105 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_

## 2026-09-18 01:43  ·  gamma  ·  panel 16.44 / companion 1.21 / GP 0.69 / DM 0.18 / DS 0.9 / KT 0.15  ·  ALL GREEN

| indicator | gate | suite | result | status |
|---|---|---|---|---|
| gamma | A | `test_gammaprofile_build.js` | 105/105 | ok |
| gamma | A | `test_if_extras.js` | 24/24 | ok |
| gamma | B | `test_gammaprofile_logic.cpp` | 85/85 | ok |
| gamma | B | `test_contractoffset_logic.cpp` | 15/15 | ok |
| gamma | L | `fixtureA-0947 (live runner)` | 18/18 | ok |
| gamma | L | `fixtureIF-1020 (live runner)` | 19/19 | ok |
| gamma | L | `GammaProfile-1325 (live runner)` | 18/18 | ok |
| gamma | L | `GammaProfile-1426 (live runner)` | 18/18 | ok |
| gamma | L | `1118-pool (live runner)` | 20/20 | ok |
| gamma | L | `1118-pool-SPY (live runner)` | 21/21 | ok |
| gamma | S | `GammaProfile.cpp` | ok | ok |

_host: vm, C++: g++_
