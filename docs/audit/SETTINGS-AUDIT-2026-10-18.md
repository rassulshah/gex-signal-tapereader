# IRT indicator settings audit - 2026-10-18

Asked by Rassul at 15:13 CT: "go. make sure every indicator always has its settings verified and tested so there are not bugs anymore".
Added at 15:14 CT: "make sure it even has the right settings and you are not adding useless settings also".
His complaints: "?" or garbage values, wrong colours and defaults (BigBidAsk Divider 7 on ES), black colour swatches while the chart draws
red / green, rows cut off on the right, and group boxes that did not uncheck or grey out their items.

## 1. Root causes, checked against the SDK (irtsdk.h 15.1.6)

| # | Suspected cause | What the SDK and the files show | Verdict |
|---|---|---|---|
| 1 | Tests ran on a Linux mock and never tried the settings window | No suite modelled how IRT treats colour rows, and no test looked at row widths or group boxes. The existing mocks returned plain values, so the black-swatch bug could not show up in a test. | **Confirmed.** New mock (tests_settings/mock_irtsdk.h): a colour row ignores setIntegerValue and reads 0, the chart copy reads blank, and a window can be set to ignore what the plugin fills in. |
| 2 | Settings registered by running index and saved / read by position | IRT keeps window values BY POSITION (lesson 42). The plugin files were positional too ("7,0,0,1,1,1,-1,..." in HodLod; "BBA7\|0\|0\|..." in BigBidAsk). Controls were looked up by `pc++`, by index constants (`WIDTH_INDEX = 0`) or by bare numbers (TapeReader: `getListIndex(0)`). DeltaProfile.settings.txt was keyed by CONTRACT root (EPZ26), so every contract roll fell back to defaults. | **Confirmed.** |
| 3 | Colour byte order differs between drawing and the dialog | The SDK has ONE colour type: `typedef unsigned long COLOR`, with `MAKE_COLOR(r,g,b) = r<<16 \| g<<8 \| b`, i.e. 0x00RRGGBB. Drawing, setColorParameter, getParameterColor and setParameterColor all use it, and no plugin uses the Windows RGB() macro. The real cause was the **wrong call**: colour rows were filled with setIntegerValue and read with getIntegerValue, which leaves the swatch BLACK and reads back 0. On his PC on 10-18, BigBidAsk.settings-ES.txt held 0 for every colour. HodLod's place files held -1 for every colour, which meant a colour he picked was never saved. SessionVWAP, which uses the colour calls, stores sane colours (VWAP FF80FF). | **Not a byte-order problem; wrong API (confirmed).** KingTracker found this in v0.15, BigBidAsk fixed it in 1.16 (lesson 94), HodLod and LiquidityProfile still had it. |
| 4 | No width budget for same-line controls | The SDK has `setParameterDialogHeight(nLines)` but no window width. Measured estimate: about 6 px per character plus the control, with the window cutting off at about 415 px (BigBidAsk, 10-18). Budget: 400 px per line. | **Confirmed.** Cut-off rows: HodLod projection ~424 px, BidAskStats / BookFlow first line ~458 px, LiquidityProfile big-order line ~648 px, DealerProfile indicators text ~444 px. |
| 5 | Group master boxes not wired to their items | `enableParameter(i, bool)` and the `parmsUpdt(i)` callback exist in the real SDK. Only BigBidAsk 1.11+ used them (1.16 also greys its items). Every other plugin had none. | **Confirmed.** |

## 2. The layer (plugin\SettingsSpec.h 1.0.0, SDK-free + plugin\SettingsHost.h 1.0.0)
- Each indicator has ONE table, `<Name>Settings.h`, with one row per control: key, label, type, default plus per-market defaults, range, width, same-line flag, group master, hidden flag.
- Registration (`ss::registerAll`), filling the window (`ss::fillDialog`), reading it back (`ss::readDialog`), group masters (`ss::onUpdate`) and the per-market keyed file (`ss::Store`) all come from that table. There is no index arithmetic.
- File format: `<Name>.settings-<MKT>.txt` holds `SPEC1|plugin|version|MKT` on the first line, then key=value lines.
  - Unknown keys are ignored.
  - A missing or invalid key gets the market's default, never garbage.
  - It is re-read at most once a second and compared by content (lesson 84).
  - The old file is migrated ONCE through the table's `legacy()` reader and left in place.
- Colours go through setParameterColor / getParameterColor only.
- A control he did not touch keeps the file's value, even if IRT ignored the fill.
- An invalid entry is never saved, and the chart copy never saves.

## 3. Tests (all pass)
- `tests_settings/settings_contract.cpp`: 6,692 checks over 5 tables (HodLod, DeltaProfile, TapeReader, BidAskStats, BookFlow). It covers:
  - table checks (keys, no '?' in labels, defaults valid for all 7 markets, ranges, groups, the 400 px width budget);
  - a 7-market x 20-set round trip;
  - garbage, other-plugin, other-market, invalid and unknown keys all falling back to defaults;
  - legacy samples taken from his real 10-18 files;
  - group masters, and colours passing through unchanged;
  - every row actually read by its .cpp (no dead settings);
  - no hand-written parameter calls in a migrated .cpp;
  - the setVersion literal matching the table version.

  It builds with g++ (ASan/UBSan, -Werror) and MinGW, and gex-build runs it with cl.
- `tests_settings/settings_host_test.cpp`: 60 checks on the IRT-behaviour mock. It covers:
  - registration (ids, same-line flags, parameter version, window height);
  - a colour shown in the window equals the colour drawn, and the old integer path reproduces the black swatch;
  - a master greys its colours and re-enables them;
  - an out-of-range entry is never saved;
  - an ignored fill keeps the file's values;
  - his ES place file migrates to Bottom centre with default colours (never black);
  - his DeltaProfile per-root file maps to per-market values;
  - a second save in the same second with the same size is still seen;
  - the 2.4.1 projection row fails the width budget.
- `tests_settings/run_tests.sh` runs both, then plants a `setIntegerValue` in a copy of HodLod.cpp and checks that the contract catches it.
- Existing suites:
  - tests_hodlod/t_hl240: 45 / 45 (the hard-typed version updated to 2.4.2).
  - tests_dp217: test_dp217 47 / 47 and test_dp218 90 / 90 (the mock gained the parameter calls; the version moved to 2.18.1).
  - tests_aggbook: logic 155 / 155, BidAskStats host 15 / 15, BookFlow host 23 / 23, real-SDK syntax 0 / 0. The mock gained the parameter calls; the host test runs setup once, as IRT does; versions and row checks now read the table.
  - tests_lqp044: logic 114 / 114, host 21 / 21. The versions moved to 0.44.2 and the mock gained the colour calls. The host test's 48-byte leak is in the test itself, not the plugin.
  - The PC's cl compiled and passed the contract at 15:44 CT ("SETTINGS CONTRACT PASSED" in gex-build.log).
  - MinGW syntax check against the REAL irtsdk.h: 0 errors and no new warnings for HodLod, DeltaProfile, TapeReader, LiquidityProfile, BidAskStats and BookFlow.
- Build gate: `tools\gex-build.bat` compiles and runs settings_contract.exe before any plugin. On FAIL nothing is built, BUILD-STATUS shows "SETTINGS CONTRACT: FAILED" and the details are in `plugin\out\SETTINGS-CONTRACT.txt`.

## 4. Per indicator

Format: bugs (fix), then KEPT / ADDED / REMOVED, one line each.

### lsHodLod 2.4.1 -> 2.4.2 (MIGRATED)
- Bug: the 5 colour rows were filled and read with the integer calls. The swatches showed black and a colour he picked was never saved (his ES / NQ files had -1 everywhere). **Fixed**: colour calls through the layer.
- Bug: the projection row was ~424 px and cut off. **Fixed**: "Up (to the HOD)" / "Down (to the LOD)" shortened to "Up (HOD)" / "Down (LOD)", now ~340 px.
- Bug: the Show boxes did not grey their colours. **Fixed**: each Show box is now the master of its colours.
- Bug: positional place file. **Fixed**: keyed `HodLod.settings-<MKT>.txt`, migrated once from `HodLod.place-<MKT>.txt`.
- KEPT: Position (the 9-spot box rule). Show table + Table background, Show projection + Up / Down colours, Show 1st-extreme mark + LOD / HOD call colours (a show box and a colour for every drawn element). Offset X / Y (see his decision list).
- ADDED: none. REMOVED: none.

### lsDeltaProfile 2.18.0 -> 2.18.1 (MIGRATED)
- Bug: "Width px (40-250)" did nothing under 110 px, because the column is drawn at least 110 px wide (2.11.2). **Fixed**: range 110-250, default 110 (drawn exactly as before).
- Bug: settings were kept per contract root (EPZ26), so every roll reset them, and an old pre-2.8 Gap column was still in the file. **Fixed**: per market, migrated once.
- Bug: positional file and index constants. **Fixed**: the layer.
- KEPT: Width, Font pt.
- REMOVED from the window: "Show delta text". Since 2.14.0 the row values are not drawn, so this box only reserved room for text that never appears. It is now a hidden row: its value is kept and still read, so the drawing is unchanged.
- ADDED: none.

### lsTapeReader 1.3.0 -> 1.3.1 (MIGRATED)
- Bug: controls were read by literal position (0, 1, 2, 3). **Fixed**: the layer.
- Bug: positional place file. **Fixed**: keyed `TapeReader.settings-<MKT>.txt`, migrated once.
- KEPT: Position, Offset X / Y, Banner background. ADDED / REMOVED: none.

### lsLiquidityProfile 0.44.0 -> 0.44.2 (targeted fixes; not migrated yet)
- Build break (found in this audit, NOT caused by it): **lsLiquidityFlow had not linked since 15:27 CT** (LNK2019: bandLoad / bandSave). The 0.44.0 band pool is defined only in the Profile build, but pump() called it in both. So the Flow pane kept running its older DLL (BUILD-STATUS "built, not installed - PENDING" came from an earlier build). **Fixed in 0.44.2**: the calls are now in the Profile build only, and the Flow behaves as before 0.44.0.
- Bug: the Banner background colour was filled and read with the integer calls (black swatch, 0 saved). **Fixed** with setParameterColor / getParameterColor. No banner file existed yet, so no black value had been saved.
- Open:
  - "Big order min lots | Ended lines kept per side (0-50) | Big order font pt (7-16)" sit on one ~648 px line and are cut off.
  - No parmsUpdt, so "Big resting orders" and "Verdict banner" do not grey their rows.
  - Three positional files (size, bigorders, banner).
  - Range text in the labels.
  - A dead read of DeltaProfile.settings.txt into dpWidth / dpGap, which nothing uses.
  - These need the full migration, which must also cover the LiquidityFlow build (same source, Font only).
- KEPT (for now): all 15 rows. Candidates are in his decision list.

### lsBidAskStats 1.13.0 -> 1.13.2 and lsBookFlow 1.3.0 -> 1.3.2 (window MIGRATED; place files keep their format)
- Bug: "Ratio bars | Position | Spike vs average" was ~458 px on one line and cut off. **Fixed**: Spike vs average moved to its own line. The row order is unchanged, so IRT's saved values stay on their rows; window height is 3 / 4 lines.
- 1.13.2 / 1.3.2: the window now comes from OrderFlowPaneSettings.h through SettingsHost.h. The index constants (RATIO_PARM, BF_RATIO ...) are gone. Same rows, same order, parameter versions 5 / 3 kept, so IRT's saved values stay on their rows. The settings contract covers both panes.
- Open: the place files are still the panes' own positional format, read by OrderFlowFeed.h (a keyed file is the next step, together with OrderFlowFeed.h).
- KEPT: all rows. Candidates are in his decision list.

### lsBigBidAsk 1.16.0 (fixed by the coordinating agent today; not migrated)
- 1.16.0 already: colour calls (lesson 94), parameter version bump (the Divider 7), groups grey their items, and a fill / read-back log in `BigBidAsk.dialog-<MKT>.txt`.
- Open:
  - positional `BBA7|...` file (toVec / fromVec);
  - the category rows are built in a loop, so their widths cannot be estimated statically until it is on the layer;
  - "Divider (0 = market default)" should show the market's real value (a per-market default in the table).
- Next: migrate on top of 1.16 and its tests_bba115/test_bba116.cpp.

### lsDealerProfile 2.17.0 (audited; not migrated)
- Bug: "Your indicators in the bar file (names, comma)" is a ~444 px text row and is cut off.
- Bug: DealerProfile.settings.txt is keyed by contract root plus chart ("EPZ26~3 Minutes*~"), so it resets at every roll, as Delta did. The file on his PC is from 10-05 and has labels = 0.
- Rule conflict: "Snapshot outline (1.x files)" is ON by default and draws a DASHED rectangle (dashRect), but his rule is solid lines only.
- HodLod also reads DealerProfile.settings.txt positionally (the banner flag, column 9), so migrating the Dealer Profile must update that reader too.
- KEPT: Dealer column px, Levels column px, Badge font, Liquidity in badge, Font size, Show buy / sell text, Fade strikes beyond 1.2 EM, Whole-book banner. Candidates are in his decision list.

### lsSessionVWAP 1.6.0 (audited; already correct, not migrated)
- Uses the colour calls correctly, every band has a show box and a colour, and every line is within the budget (widest ~366 px). No bugs found. Move it to the layer for uniformity later.

### lsTradeManager 1.3.0, lsKingTracker 0.17 (audited; not migrated)
- TradeManager: Market (Auto), Font size, Position. All lines within the budget.
- KingTracker: already uses the colour calls (v0.15), lines within the budget.
- The scanner found nothing else. "Market (Auto)" and "Clock offset" are in his decision list.

### No settings window (nothing to audit in a window)
- CVD, CobImpulse, SessionInfo, Exhaustion, ExhaustionV2, DeltaProfileHourly, MboCheck: no rows. SessionInfo only has a place file ("0").
- PullStats / StackStats / Pulling are retired from the build, so they were skipped.
- Right-settings gap: CVD and CobImpulse draw lines and marks with no show box or colour. Whether to add them is in his decision list.

## 4b. Status after the second pass (16:00 - 17:30 CT): per indicator

| Indicator | Version | On the layer | File | Bugs fixed in this audit | Tests |
|---|---|---|---|---|---|
| HodLod | 2.4.3 | yes | keyed HodLod.settings-<MKT>.txt (from place file) | black colour swatches / colours never saved; cut-off projection row; Show boxes now grey their colours; Dealer Profile banner read from the keyed file (no positional column) | t_hl240 45/45, contract |
| DeltaProfile | 2.18.2 | yes | keyed per MARKET (was per contract: reset at every roll) | dead Width range 40-109; "Show delta text" did nothing (now hidden); migration reads the chart's own contract row first | dp217 47/47, dp218 90/90, contract |
| TapeReader | 1.3.1 | yes | keyed (from place file) | controls read by literal position | contract, syntax |
| BidAskStats | 1.13.2 | window yes | pane place file (format kept) | cut-off first row; index constants | aggbook 155 + 15 host, contract |
| BookFlow | 1.3.2 | window yes | pane place file (format kept) | cut-off first row; index constants | aggbook 23 host, contract |
| LiquidityProfile + LiquidityFlow | 0.45.0 | yes (2 tables) | size / bigorders / banner files (strict formats kept) | banner colour black swatch / 0 saved; ~648 px row cut off; no group wiring (masters now grey their rows); Font 6-9 did nothing; chart copy could save the size row; **Flow had not linked since 15:27** (fixed 0.44.2) | lqp044 114 + 21 host, contract |
| DealerProfile | 2.17.1 | yes (new TEXT row type for the indicators list) | keyed per MARKET DealerProfile.settings-<MKT>.txt (old file keyed by contract root: reset at every roll); migrated once from the chart's own root row; per-chart (~) rows of 10-05 never matched before, so they are skipped (nothing he sees changes) | indicators row ~444 px cut off (label shortened) | 2110 identical to baseline (37 pass / 19 pre-existing failures of a stale suite), 2170_host 12/12 (version check now compares the constant), 290 identical (60/3 pre-existing), contract with his real file rows |
| TradeManager | 1.3.1 | yes | keyed (from place file; place file still written for the drag code) | **Font never reached the chart** (read only while the window was open) | t130 13/13, contract |
| SessionVWAP | 1.6.0 | no | its own keyed-style file | none found (correct colour API, groups not needed) | - |
| BigBidAsk | 1.16.0 | no | positional BBA7 file | colour API / version bump / groups fixed by the coordinating agent in 1.16 | its tests_bba115 |
| KingTracker | 0.17 | no | none (reads the window in callbacks) | design predates the per-market file rule (Skylit King, not an LRA pane) | - |
| CVD, CobImpulse, SessionInfo, Exhaustion, ExhaustionV2, DeltaProfileHourly, MboCheck | - | n/a | - | no settings window (nothing to migrate) | - |

New BUILD-LESSONS: 101 (LiquidityFlow link break). Pre-existing, NOT caused by this audit: test_dealerprofile_2110 (19 failures) and test_dealerprofile_290 (3 failures) are stale suites written for 2.12 / 2.9 - same failures before and after; they need their own refresh.

## 5. His decision list (not changed)
1. **Offset X / Y** in HodLod, TapeReader, BidAskStats, BookFlow, and LiquidityProfile's label offset. On 10-07 you chose the 9-spot Position instead of pixel offsets, but HodLod 2.0.6 added offsets later. Remove them from the window? (Saved offsets would stay in the files and keep working.)
2. **BookFlow "Record order feed"**: your rule is to always keep recording. Remove the box so recording is always on?
3. **BidAskStats / BookFlow "Ratio bars" and "Spike vs average"**: internal tuning that the per-market data should set. Hide them and let the nightly set them per market?
4. **DealerProfile**:
   - "Snapshot outline (1.x files)" draws dashed rectangles: remove it (off)?
   - "Clock offset (min)" and "Your indicators in the bar file" are internal: hide them?
   - "Market (Auto)": hide it?
5. **LiquidityProfile "Ended lines kept per side (0-50)"**: internal, hide it?
6. **DeltaProfile Font default is 9 pt** and **TradeManager Font default is 8 pt**, both under your 10 pt minimum. Raise them to 10?
7. **CVD / CobImpulse**: add show boxes and colours for the drawn line and marks?

## 6. Real-dialog check (pending)
Pending for the next attended session (tools\settings-check\README.md): HodLod 2.4.3, DeltaProfile 2.18.2, TapeReader 1.3.1, LiquidityProfile + LiquidityFlow 0.45.0, BidAskStats 1.13.2, BookFlow 1.3.2, DealerProfile 2.17.1 and TradeManager 1.3.1. Expected windows: `settings_contract.exe <plugin> --print ES`.

## 7. Left to do
Migrate to the layer: BigBidAsk (on top of 1.16 and its tests_bba115), keyed place files for BidAskStats / BookFlow (with OrderFlowFeed.h), LiquidityProfile's three files into one keyed file, SessionVWAP, KingTracker. Earlier list for reference: LiquidityProfile + LiquidityFlow, BigBidAsk (on top of 1.16), keyed place files for BidAskStats / BookFlow (with OrderFlowFeed.h), DealerProfile (and HodLod's banner reader), SessionVWAP, TradeManager, KingTracker. For each one: add its .cpp to SETSRC in gex-build.bat and add an Entry to settings_contract.cpp.
