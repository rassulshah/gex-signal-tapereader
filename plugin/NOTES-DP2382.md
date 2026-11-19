# lsDeltaProfile 2.38.2 — memory growth: measured, not yet fixed

**Request (Rassul, 2026-11-19 13:07 CT):** "ok.. when will you fix delta profile" / "can you fix it now?"

## What the live files show (permem2-DeltaProfile-20261119.csv, PerfGuard 1.2.0)
- own_calc_mb is always 0. All of the growth happens inside draw(), and own_live rises by the sum of own_draw_mb.
- About 170–180 KB stays allocated per draw on ES and NQ, and about 120–150 KB on CL, GC and HG. The rate is the same at 07:30 and at 13:05, so it doesn't build up over the session.
- In the first minute after a restart it is about 300 KB per draw (13:00: 10.65 MB over 35 draws). In the minutes after that it is about 180 KB.
- Process private bytes rise with it (11:52→12:25: priv +1,480 MB, own +1,365 MB), so this is a real leak, not a counting error.
- ES has 136 signals and NQ has 298, yet both keep the same amount per draw. So the growth doesn't scale with signals, kept circles or profile rows.

## What the host test shows (mock SDK, same 2.38.1 code)
- 5,000 draws on two charts at real ES and NQ prices, with the real lsFlexLevels files and today's real ES and NQ session records (298 signals). The test scrolls every 7 draws, zooms every 11, resizes the pane every 13 and adds a new bar every 50 draws. It ran both with throttling and with a forced rebuild on every draw. It also ran with symbol switching and with draws 100 ms apart.
  - The heap stays flat after warm-up: within ±100 KB over 5,000 draws, and the movement comes from the 100 added bars. That is about 0.02 KB per draw, against 170 KB per draw on the PC.
- So none of the DeltaProfile code paths the mock can run is leaking. The leak lives in something the mock can't model: the real SDK or IRT side of draw().

## Leading suspect (not proven)
- Each draw() builds SDK bar arrays: 18 per normal draw (6 RTARRAYI, 5 of them barDateTime, and 12 RTARRAY) and 34 on a rebuild draw (11 + 22 + 1 RTARRAYP). In the real SDK these classes hold a `tempArray` / `bFreeTempArray`. The code is in the SDK .lib that is linked into our DLL, so its allocations go through our counted operator new.
- 170 KB / 18 ≈ 9.4 KB per array, and 300 KB / 34 ≈ 8.9 KB. A chart of about 2,300 bars at 4 bytes per value fits both. The ratio between normal and rebuild draws also matches the first-minute ratio.
- calc() builds no arrays, and own_calc is 0. DeltaProfile's whole engine (buildNative) and render both run in draw().

## What 2.38.2 adds (nothing drawn changes)
- `OWN_HEAP` status line: the own-heap bytes kept per draw by each stage of draw() (layout, build, render, mst, self, aware, status), summed for the DLL. It refreshes every 10 s.
- `SDK_PROBE` status line: once a minute, draw() builds and destroys one RTARRAYI(barDateTime), one RTARRAY(barHigh) and one RTARRAYP(barVolumeProfile), and records the own-heap bytes each one leaves behind. This directly confirms or rules out the leading suspect. The cost is 3 arrays a minute.
- Version 2.38.2 is set in DLT_VERSION, setVersion("2.38.2") and DeltaProfileSettings.h PLUGIN_VERSION. No settings change, no include change, and the gex-build header list is unchanged.

## Tests (scratchpad dpleak/plugin/t)
- Draw list byte-identical: 3,000 draws, 279,113 drawn items, 2.38.1 vs 2.38.2. The only text excluded is the title, which carries the version. 2.38.1 run twice gives an identical list.
- 5,000-draw own-heap runs: flat, as above.
- ASan + UBSan + LeakSanitizer over 1,500 forced draws and 800 throttled draws: no reports, exit 0.
- MinGW syntax check against the real irtsdk.h (Windows path: PerfGuard's own-heap operator new compiled in): clean.

## What to read after the restart (DeltaProfile.status-ES-180.txt and the others)
- `SDK_PROBE ... RTARRAYI(barDateTime) N`. If N is about bars × 4 (≈ 9,000) or more, every SDK array leaks. The fix is then to read the bars once per closed bar (or in calc) and keep them in the chart state. That would take draw from 18–34 array builds down to about 1.
- `OWN_HEAP ... build X render Y ...`. This shows which stage keeps the 170 KB.
- permem2: own_draw_mb ÷ draw_n should still be about 170 KB/draw (2.38.2 doesn't change it).
