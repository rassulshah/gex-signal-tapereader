# THE YAHOO PIPELINE — ITEM 18, LOCKED, NEVER BUILT, THEN LOST

_Written 2026-08-27. The operator said three times that a Yahoo solution already existed and that
**files in git were being updated to get the data**. He was right, and it took three passes to find
because I kept looking in the wrong place. **Do not rebuild it — finish it.**_

---

## 1 · THE ANSWER: ITEM 18

It is in **this repo's own history**, in `session-state/latest-resume-note.md`, and it is ours — not
another project's. Quoted verbatim from commit `72e820e` (2026-08-15):

> **18. Yahoo Finance HTF/ITF data (item 18, locked 2026-08-16)**: userscript-side fetch of
> `https://query1.finance.yahoo.com/v8/finance/chart/{SPY|QQQ|^SPX|^VIX|ES=F}?interval=..&range=..`
> (intervals 1m…1wk; 1m ≤7d, intraday ≤60d, daily unlimited; no key; chart endpoint does NOT need
> the cookie/crumb that quoteSummary does). Try plain `fetch` first, fall back to
> `GM_xmlhttpRequest` (needs `@grant GM_xmlhttpRequest` + `@connect query1.finance.yahoo.com`;
> verify unsafeWindow access still OK). New Layer-0 source `htfFeed` → cache `gpts_htf_v1` →
> `STATE.htf` → `snap.htf` in the export. Tier 1 daily/weekly at boot: prior week/month H/L/C,
> 20/50/200 DMA, daily ATR(14), gap vs ATR, position in weekly range. Tier 2 60m/15m hourly:
> 1h/4h trend + swings, 1h ATR. Joins item 14 as "nearest chart level" per node; READ may cite an
> HTF level when a node sits on it. **Cloud sandbox CANNOT reach Yahoo (blocked) — the browser
> fetches, the cloud reads the exported day file.** Degrade silently if the endpoint changes.
> User's console check pending (CORS yes/no).

**That last sentence is the whole architecture, and it is the thing I failed to find twice.**

## 2 · "FILES BEING UPDATED IN GIT" — THAT IS THE DAY FILES

Git is the **transport**, not the fetcher. There is no cron, no GitHub Action, no server:

    Tampermonkey (Skylit tab) ──fetch──▶ query1.finance.yahoo.com
              │
              ▼  STATE.htf
        snap.htf, per bar
              │
              ▼  buildDayExport()
        data/YYYY-MM-DD.json          ← THE FILES THAT GET UPDATED IN GIT
              │
              ▼  installer .bat: xcopy + commit + push (from his machine)
           GitHub repo
              │
              ▼  git clone --depth 1
        the cloud session reads it

The browser is the only participant that can reach Yahoo. It writes into the day file the panel
already exports; the installer already commits and pushes those; the cloud already clones them. **The
pipe is fully built — only the tap at the top was never fitted.**

⚠ This is also why my `WebFetch` on `query1.finance.yahoo.com` returning `ROBOTS_DISALLOWED` was never
a blocker. Item 18 anticipated it in 2026-08-16 and routed around it by design.

## 3 · IT WAS NEVER BUILT. VERIFIED FOUR WAYS

| check | result |
|---|---|
| `htfFeed` / `gpts_htf_v1` / `.htf` / `yahoo` / `query1` in either current userscript | **zero hits** |
| `"htf"` key in any of the 8 exported day files (08-17 → 08-26) | **none, in any file** |
| `skills/gex/SKILL.md` (the repo's own skill) mentions Yahoo or HTF | **no** |
| a `.github/` directory, or any workflow yml, anywhere in 171 commits | **none** |

**And then it was lost.** Item 18 appears in every resume note from `72e820e` (2026-08-15) through
`04f6f80` / v11.23 (2026-08-20), and **is absent from every note written after that**. It did not get
cancelled or completed — a handoff dropped it, and the current v14.51 note has no trace. That is
about 24 versions ago. **This is the same failure as the lost HOD/LOD mockup and the lost
`FINDINGS.md`: work that existed only in a resume note, and a later note that did not carry it.**

## 4 · THE ONE HARD CONSTRAINT ITEM 18 DID NOT RESOLVE

Item 18 says `@grant GM_xmlhttpRequest`. **That cannot go in the main panel.** `@grant none` is
load-bearing there — the feed hooks patch `window.fetch` and `XMLHttpRequest` in page context, and
any `@grant` sandboxes the script and kills the tape. The note's own hedge — *"verify unsafeWindow
access still OK"* — is exactly this doubt, and the console check it depended on was never done.

**The project already solved this, twice.** `current/gex-if-levels.user.js` is a second userscript
that carries `GM_xmlhttpRequest` precisely because the Atlas page cannot fetch across origins, and at
**v1.14 it added the ForexFactory economic-calendar courier, writing to `gpts_evcal_v1`** — a foreign
site fetched in the companion and handed to the panel through localStorage. Yahoo is the same shape
with a different URL. **The template is written, tested and shipping.**

## 5 · WHAT THE YAHOO LIMITS ACTUALLY ARE

From item 18 itself, and confirmed independently: **1m ≤ 7 days · intraday (2m/5m/15m/60m) ≤ 60 days ·
daily unlimited.** No API key. The `chart` endpoint needs no cookie/crumb.

Applied to our corpus gaps:

| gap | recoverable | how |
|---|---|---|
| **2026-07-18 → 08-14** (20 weekdays) | **YES**, at 2-min | the ≤60d intraday tier reaches ~06-28 |
| **2026-08-24 → now** | **YES**, at 1-min | the ≤7d tier |

⚠⚠ **CLOCK ON THE JULY GAP: 2026-09-16.** Sixty days from 07-18. After that those sessions leave
Yahoo's window permanently. About three weeks from today.

⚠ **RESOLUTION IS NOT UNIFORM AND MUST BE LABELLED.** Our corpus is 1-minute vendor bars; the July
stretch would be 2-minute Yahoo bars. A HOD/LOD timestamp is only as precise as the bar that made it.
Tag source per row — same discipline as the two gamma books.

⚠ **`ES=F` IS A CONTINUOUS FRONT-MONTH QUOTE, `EPM26` IS ONE CONTRACT.** They differ by the calendar
spread across a roll — points, not ticks. Item 18 lists `ES=F`; the corpus is `EPM26`. State the
basis or keep the series separate.

---

## APPENDIX · THE OTHER YAHOO PIPELINE (a different project — reusable, not ours)

Found first and initially misreported here as the answer. It is **not** this project's solution, but
it is real, complete, and worth raiding for logic.

A separate Replit project — a Streamlit "HOD/LOD Analysis Application" — backed up to Google Drive,
folder `1Z9vWuqh95lrpWY4VfcXtWYDVy7cIMYhW`. Its `.replit` defines a **Workflow** named
`Data Scheduler` running `python scheduler.py`; `.git/config` has **no GitHub remote**, only
Replit-internal ones (`gitsafe-backup`, `subrepl-*`), which is why it is unfindable from GitHub.

| file | id |
|---|---|
| `scheduler.py` | `1Qtw6HEyLkMJnQ2locjAAHoOLty1s-9Wq` |
| `yfinance_client.py` | `1pyMA57KTLH2XezXvMTlXoo8QQ_sEbySN` |
| `utils.py` | `1Vk76xte2Et9bfsHgeODj7YoHsRgdDEZM` |

Worth stealing: `get_yfinance_ticker` builds a real front-month contract (`ESZ26.CME`-style, rolling
8 days before third Friday) instead of the continuous quote — it already solves the basis problem
above. Also `filter_bad_candles` (drops bars whose low/high sit >0.5% outside the open/close
envelope) and the intraday-completeness staleness check.

Why it stopped: `deploymentTarget = "autoscale"` cannot host a persistent scheduler — autoscale spins
down when idle and its run command is the Streamlit app only, so the `while True:` loop ran solely
while the dev workspace was open. `scheduler.log` stops **2026-05-01**. Two defects before any
restart: the hardcoded `22:00` UTC that silently captures nothing when DST flips, and
`_us_market_holidays()` running out at the end of 2026.

⚠ Drive renders `.py` markdown-escaped through the connector — not executable Python. Fetch the real
files; never retype from this page.
