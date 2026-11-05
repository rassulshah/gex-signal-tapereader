# lsBookFlow 1.36.0 (2026-11-04 20:39 CT)

Rassul: "this is overcomplicated.. how do i make it basic". He chose "option A" on the bf_basic mockup.

## Changes (drawing only; the bars are unchanged)
- **Header:** "Defending?  B 0.4x  · S 0.5x". The ratio is the same one as in 1.35.0.
  - Removed: the lots (+76 / +77) and the Live / time word.
  - Kept: "Feed silent" still shows when the feed stops.
  - `bf::ratioCell`, `LINE_RATIO_ONLY`.
- **Turn marks** (triangle + ratio over the bars): not drawn (`BF_DRAW_TURN_MARKS = false`). They are still decided, written to the turns file and shown in the status.
- **Level squares** under the bars: not drawn, and the axis no longer keeps room for them (`BF_DRAW_SQUARES = false`). They are still decided, recorded to the levelbox file and reloaded.
- **Level box** (LEVEL | Defenders | Behind | Attackers) right of the header: unchanged.

## Tests (all passed)
| Test | Result |
|---|---|
| tests_bf136/test_bf136 (new: header, flags, versions) | 11 |
| tests_bf134 host (squares decided, not drawn, no axis room; restart reload) | 32 |
| tests_bf135 host (WPOC) | 17 |
| MinGW against the real irtsdk.h | 0 errors |

tests_bf131/test_bf133 (the 1.33.0 header format) is retired.
