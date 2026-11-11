# lsHodLod 2.23.0 / ReadBox 1.1.0 (2026-11-11)

## What he asked (CT)
- 11:40 "get rid of dly trend.and the values to the left and move the gamma regime were the daily trend is" - asked which values: "Dly + RTH
  opens + Bar left".
- 11:41 (screenshot of the read box, the HTF line and "0.8x ADR, Used 91%" crossed out) "also updat eht readt at the top to be more simple".

## What changed
- Header row of the HOD / LOD table: no "RTH opens 8:30 AM" / "RTH 83% done", no "Bar m:ss left", no "Dly:" trend. The gamma pill
  (+GAMMA / -GAMMA) now leads the centre group, where "Dly:" was, followed by "Hrly:" and "3m:". Right side: Exp, the range pill, the version.
- Read box (ReadBox 1.1.0): Hrly Trend / Day / Lvl / Mkt / Lmt. The first line is "Hrly Trend: Up / Down / NA" (11:45 CT "where it says HTF change it to Hrly Trend: and give it values like up or down or NA"; 2.23.1) by the Step Trend 1-bar hourly rule over the completed hours, green / red / white. The RTH
  Day line has no ADR multiple and no "Used %" ("Day: Nor 35%, -> CoB 1.33"). The overnight Day line still shows "ON 0.4x ADR".
- Flags: hl::HDR_DLY / HDR_LEFT / HDR_PILL_MID, rbx::SHOW_HTF / DAY_ADR. No settings change.

## Tests
- tests_readbox/test_readbox_110.cpp: 8 pass. MinGW real SDK: 0 errors.
  Settings contract passes.
- NOT run: the older lsHodLod host suites and tests_readbox assertions that expect the HTF line / the 5-line box (superseded).

## Not verified (needs his IRT)
- The look of the header row with the pill in the centre; nothing was rendered here.
