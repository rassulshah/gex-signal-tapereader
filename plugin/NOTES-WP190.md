# lsWavePOC 1.9.0 (2026-11-05 08:13-08:58 CT)

Rassul's requests, 08:13-08:46 CT:
- "draw a volume profile instead of the delta profile and it should have upto 2 hvns and the poc ... the other two hvn should be below the breakout high and if there is no breakout, it should be below the bar that broke the prior high pivot"
- "display the poc wherever it may be and then look for 2 hvn's at the break level or lower" (entry opportunities on the POC or an HVN)
- "just show the poc line"
- "now"

## Changes
- **Right of the spine:** the wave's volume profile replaces the delta profile. It is in the same place and the same size as before (38 px).
  - Plain rows are dim grey.
  - The POC row is cyan, with a P left of the spine. The POC line to the right is unchanged.
  - Up to 2 HVN rows are filled in the wave colour (green up, red down), each with an H left of the spine. HVNs get no lines.
- **HVNs** follow the existing node rule: each in its own distribution, at least 2x the median row, the biggest raw row of the distribution. The POC is never an HVN, and the POC is shown wherever it falls.
  - Up waves: the 2 biggest nodes AT or BELOW the break level.
  - Down waves: the 2 biggest nodes AT or ABOVE it.
- **Break level** (`wpc::breakLevel`):
  1. The high of the wave's BO candle (down wave: the low of its BD candle).
     - The candle must close after the wave's start pivot and no later than its end.
     - lsBreakout's own engine (`bko::Engine`, 3-bar rules) runs inside lsWavePOC on the same chart bars (`WavePOC::feedBreakouts`). It is native, with no file or plugin dependency.
  2. Otherwise, the high of the first bar of the wave that broke the prior pivot high (down wave: the low of the bar that broke the prior pivot low).
  3. Otherwise, the wave extreme, which puts no limit on the HVNs.
- **Retired from drawing:** the delta rows, the delta POC outline, the node boxes and the I / A / T letters. The delta values are still computed and recorded.
- **Status file:** the PROFILE line gives the number of HVNs drawn and the source of each break level.

## Tests (all passed)
| Test | Checks |
|---|---|
| test_wavepoc_190 (new) | 37 |
| test_wavepoc_150 (delta-letter, box and status checks moved to 1.9.0) | 49 |
| test_wavepoc_logic (P / H are spine letters, right-side rows are not liquidity bars) | 187 |
| test_wavepoc_host (mock + barVolume; cyan POC row + line per wave) | 109 |
| MinGW against the real irtsdk.h, -Dsmall=char | 0 errors, 0 warnings |

test_wavepoc_190 covers:
- the HVN rule
- the break-level order
- ES Oct 29-30 with lsBreakout's real BO / BD candles: every HVN on the right side of its break level, at least 2x the median row, never the POC
- no repaint: a finished wave's HVNs 6 bars after its end equal its HVNs at the end of the day
- the drawing: letters only P / H, no boxes
- the versions

render_dump_delta now feeds the BO / BD candles too; the Oct 30 PM render was checked by eye.

gex-build: lsBreakout's headers were added to lsWavePOC's hash list.
