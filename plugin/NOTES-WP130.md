# lsWavePOC 1.3.0 - CoB legs (2026-10-28)

## Request (Rassul)
- 17:27 CT approved: "EVERY CoB leg automatically gets a Wave POC profile", even when it fails the normal wave rule (20-bar extreme +
  bigger than the direction's average wave).

## CoB source (his locked rule, the SAME as the Cob marks)
- IRT's RTL custom indicators `cob_bull_3m_1b` / `cob_bear_3m_1b`, read natively with `RTARRAY(fEmptyArray).getCustomIndicator(name, 1, true)`
  exactly as lsCobImpulse::readFlags reads them (same names, same right-end alignment when the RTL array is shorter than the chart).
  No port of trade_def.cob_live was needed: the marks and the Wave POC CoB legs come from one source, so they match bar for bar.
  The flags of each closed bar are captured ONCE when the bar is fed (Engine::cob), never re-read for that bar.
- Status `COB_SOURCE,RTL cob_bull_3m_1b ok|NOT FOUND, cob_bear_3m_1b ok|NOT FOUND`. NOT FOUND = no new CoB legs (recorded ones still redraw).

## What changed
- A CoB leg = a zigzag leg (N-bar reversal pivots, as the waves) holding a CoB of its own direction (bull on an up leg, bear on a down
  leg) on a bar from its start pivot to its end pivot. It gets the full wave treatment: profile at the leg START (the sweep extreme),
  volume nodes >= 2x (own mini-distribution, max 5, WPOC amber), liquidity nodes 1.9x raw rows (max 5, LPOC bright), right-edge tags
  for open nodes only - all the 1.2.0 drawing rules.
- Decided when the leg's END pivot is confirmed (N bars after the extreme) - never earlier, no provisional / dashed CoB leg, recorded
  and never repainted. One profile per CoB: a later extension of the same leg does not draw the same CoB again (Engine::cobUsed).
- A leg that is a CoB leg AND a qualifying wave is drawn ONCE, kind BOTH (dedupe = the same leg start / end bars).
- Stops (same as waves): touch; a later qualifying wave (any direction - the 1.0 rule, unchanged) or a later CoB leg of the SAME
  direction (an opposite CoB leg is usually the pullback itself - it does not stop the nodes); a close beyond the leg's start.
- Setting row 3 "CoB legs" (box, default ON). Parameter version 1 -> 2 (template IB -> IBB): parmsInit carries his Reversal bars and
  Show zigzag over and sets CoB legs on (he does not need to reopen anything). A garbled box = the defaults. Off = no CoB legs, and
  recorded COB-only legs are not drawn (BOTH legs still are - they are waves).
- Record (lsFlexLevels\WavePOC\<MKT>-<spb>-nodes.csv): every line gets a kind field WAVE / COB / BOTH; the WAVE line also gets the CoB
  bar time (0 = none): `WAVE,...,vapMissing,kind,cobTime` (21 fields) | `NODE,...,ratio,kind` | `STOP,...,time,kind` | `LNODE,...,kind` |
  `LSTOP,...,kind`. Older 1.0-1.2 lines (no kind) still parse as WAVE. For the nightly: join NODE rank 0 (WPOC) with its STOP by t0,t1
  and split by kind to test whether pullbacks reach the CoB leg WPOC.
- Status: `COB_LEGS,on|off,CoB bars bull N bear N,legs drawn COB only N BOTH (also a wave) N,CoB already profiled N`; SETTINGS line + CoB legs.
- Versions 1.3.0: WP_VERSION, setVersion literal, wpc::VERSION / WPC_VERSION, wpset::PLUGIN_VERSION (wpl liquidity stays 1.1.1).

## Tests (Linux, -Wall -Wextra -Werror, ASan + UBSan)
- test_wavepoc_logic 133/0 (was 111): new testCobSynthetic - CoB leg that is NOT a wave gets a profile at its confirmation bar 27, not
  before, never provisional; WPOC = the 400-lot row; kind COB + CoB time in WAVE / NODE / STOP lines; opposite-direction CoB leg does
  not stop, same-direction does (new wave); also-a-wave = ONE profile kind BOTH; bull CoB on a down leg = nothing; setting off = nothing;
  record round trip + restart redraws the COB leg with no flags, off hides it; restart after every bar = one pass; 1.2.0 lines parse,
  unknown kind = bad line. testCobReal (Oct 22 ES): bull CoB on bar 43 -> the 13:06-13:24 up leg (41 -> 47, not a wave) gets a profile
  at bar 50; the two sell waves' stops unchanged; status COB_LEGS line.
- test_wavepoc_host 66/0 (was 49): row 3 CoB legs default on, parameter version 2, parmsInit(1) keeps N 5 / zigzag on + CoB on,
  parmsInit(2) leaves his set, garbled box = defaults; bar-by-bar with the mock RTL arrays: CoB leg appears at bar 50 only, drawn at bar
  41 with one amber WPOC; record kinds; status COB_LEGS / COB_SOURCE / SETTINGS; restart without the RTL redraws it from the record
  (status NOT FOUND), 3 WAVE lines, none twice; setting off = 2 waves; bull flag in a down leg = nothing; shorter RTL array aligned.
  Test fixes: the 1.2.0 host suite read a destroyed window host (ASan stack-use-after-scope - its 49/0 was without ASan) and never
  removed chart A (LeakSanitizer); both fixed.
- settings_contract PASS (45,949 checks, 24 tables). (settings_host_test has 1 old failure: it expects HodLod parameter version 7, the
  table is 8 since 2.6.0 - not WavePOC.)
- MinGW -fsyntax-only (NOMINMAX + windows.h + the real irtsdk.h 15.1.6): 0 errors, 28 warnings all inside irtsdk.h (same as 1.2.0).

## Not verified - needs his IRT
- The RTL arrays on his price chart (status COB_SOURCE line should say ok / ok on the 3-min charts; the RTL names are 3-min ones).
- The look with the extra CoB-leg profiles (ES + NQ) after the evening restart.
