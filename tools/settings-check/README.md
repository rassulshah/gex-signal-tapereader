# settings-check - the real settings window after every build (2026-10-18)

The Linux tests (plugin\tests_settings) prove the tables, the files and the SDK calls against a mock. Only IRT itself shows what the
window really looks like (row widths, swatch colours, whether IRT kept the values the plugin filled in). So after every build that
changed an indicator's settings, an ATTENDED Claude session does this check.

## When
- After Rassul restarted IRT and BUILD-STATUS says `installed` for the changed plugin.
- Only with his permission (computer-use on his PC). Never during trading hours unless he asks. Never touch orders or the trading platform.
- Unattended / nightly builds cannot do it: they add a line `dialog check pending: <Plugin> <version>` to docs\handoff\LATEST.md
  (level-reversal-analytics) so the next attended session does it.

## Steps (Claude)
1. Expected window: run on the PC (or Linux with the staged sources)
   `plugin\out\settings_contract.exe C:\Dev\gex-signal-tapereader\plugin --print ES` (any market: ES NQ CL GC HG NG EU).
   It prints every line of each migrated indicator's window with its label, its default for that market, the estimated width, and the
   hidden rows kept in the file.
2. Ask Rassul to allow computer-use. His working apps are on the WIDE monitor (Samsung C49HG9x), not the BenQ.
3. For each changed indicator: open a chart of a market whose `lsFlexLevels\<Name>.settings-<MKT>.txt` you have read, open the
   indicator's settings window, screenshot it.
4. Compare, row by row:
   - every row of the table is there, in the same order, on the same lines; nothing cut off on the right (the last control's right edge
     visible inside the window);
   - each value equals the FILE's value (not just the default): positions, numbers, check boxes, and each colour swatch matches the hex
     in the file (e.g. `proj_up=22C55E` = green). A black swatch where the file has a colour = the colour API is not used (lesson 97);
   - a group master unchecked shows its items unchecked and greyed;
   - no '?' / garbage / 0 / 7 where the table has a real default.
5. Change one control (e.g. a colour), press Apply, re-read the file: only that key changed.
6. Write the result to docs\audit\SETTINGS-AUDIT-<date>.md ("dialog check" section) and remove the `dialog check pending` line.
   A mismatch = a BUILD-LESSONS line + a contract / host test that reproduces it, then the fix.

## Notes
- If IRT ignores values written into the window by parmsLoad (suspected 10-18; lsBigBidAsk 1.16 logs filled vs read back in
  `lsFlexLevels\BigBidAsk.dialog-<MKT>.txt`), the layer still saves correctly (an untouched control keeps the file's value), but the
  window would show stale values - record which plugin / row, so the fill can be retried from parmsUpdt.
- The width estimate (6 px a character + the control, 400 px a line) is a model; correct ss::BUDGET_PX / labelPx in SettingsSpec.h if
  the screenshots disagree, and say so in BUILD-LESSONS.
