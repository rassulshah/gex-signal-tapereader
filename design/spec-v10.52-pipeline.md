# v10.52 — end-to-end PIPELINE INDICATOR + automatic review read-back

Base v10.51.2. Version → 10.52 (3 spots). Invariants: one render(), final `})();`, v10.js==current/ (md5),
no key renames, descriptive-only, all enrollment intact.

## Why
There is no way to see whether the day's data actually reached the nightly review, or whether the review
came back. Two real breaks were found: (1) v10.50's footer redesign DROPPED the old `saved ✓ / not saved`
text, and (2) the panel can only receive a review via a console call (`__gptsDebug.setReview`), so a review
that exists on GitHub never appears on its own.

## A. Restore + extend the save state
`saveState()` → {code:'saved'|'download'|'none', t, name} from REPO_LAST_SAVE + localStorage
`gpts_last_export` compared against the CT trading date (NOT the UTC date — this already caused a false
"not exported today" reading on 2026-08-17).
- `saved`   = written to the repo folder today
- `download`= exported but only as a browser download → it is NOT in the repo, the review will not see it
- `none`    = not exported yet today

## B. Remote pipeline checks (public raw URLs, CORS-open, cached)
`pipeCheck()` — at most once per 10 min (cache in `gpts_pipe_v1`), skip when tab hidden:
- `dataOnGit`   : HEAD/GET `RAW_BASE/data/<CT-today>.json` → 200 = pushed
- `reviewOnGit` : GET `RAW_BASE/review/<lastTradingDay>.json` → 200 = review exists; parse + hand to
  `ANALYSIS_REVIEW` (this is the AUTO READ-BACK — replaces the manual `setReview` console call).
RAW_BASE = `https://raw.githubusercontent.com/rassulshah/gex-signal-tapereader/main`.
Fail soft: any error leaves the stage `unknown` (grey), never throws, never blocks render.

## C. The indicator (footer, replaces the 3 dots)
One compact strip, each stage a coloured dot + tiny label, each with a question-first hover:
`● rec  ● saved  ● pushed  ● review`
- rec    : green when a bar was recorded in the last ~6 min, amber otherwise. "Is it recording? ..."
- saved  : green `saved`, amber `dl` (download-only — won't reach the review), red `none`. Hover names the
           time and the exact meaning of `dl`.
- pushed : green when today's data file is on GitHub, grey unknown, red 404. Hover: "Has today's data
           reached GitHub, where the nightly review reads it?"
- review : green when last session's review file exists (and was loaded), amber when missing. Hover:
           "Did the nightly review run and come back?" + the review's headline if loaded.
Keep 💾 and 📁 buttons. Version stays at the right. Must stay ONE line at 250px.

## D. Review read-back surfaces
When `ANALYSIS_REVIEW` is populated (from B), the Analysis tab ⑥ shows it instead of "awaiting review",
and the pre-open brief may cite one line from it. No new layout — just wire the existing consumers.

## Tests
test_pipeline_indicator.js: saveState() classification incl. the CT-vs-UTC date bug (a UTC-tomorrow clock
must still read today's CT export as saved); pipeCheck caching + fail-soft on network error; the four
stage colours; indicator renders one line and contains all four stage labels. Full suite: only the 4
known-stale may fail.
