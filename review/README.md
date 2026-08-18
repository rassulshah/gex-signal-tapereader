# review/

Nightly LLM review output — one `YYYY-MM-DD.json` per trading session.

Written by the scheduled "GEX nightly review" task (see `docs/LLM-NIGHTLY-BRIEF.md` and the
`gex` skill's REVIEW procedure). The cloud session CANNOT push to GitHub (network proxy blocks
github.com), so it delivers via a cascade: device bridge → Google Drive `GEX-review-inbox` →
chat. The local "GEX data push" task commits whatever lands here.

The panel reads the newest file back automatically from the raw GitHub URL and shows it in the
Analysis tab and the pre-open brief; the footer `review` pipeline stage goes green once it loads.
