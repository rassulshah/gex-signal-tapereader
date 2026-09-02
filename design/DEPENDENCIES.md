# DEPENDENCIES — what this panel cannot work without

**Operator-mandated 2026-09-01, verbatim:** *"you need to make sure that when the application is
working, there is some type of test to check insider finance to ensure we are getting data from it
like the call wall, put wall, expected values, and you need to ensure that the application is
writing to the king levels to the server for irt. these tests need to be done to ensure they are
working, you need to ensure this is in some document as well because the next context needs to be
aware of these dependencies for the application to work properly. it is fundamental to the
application."*

> **`load gex` MUST READ THIS FILE.** It is registered in `skills/gex/SKILL.md` and pinned by
> `test_deps.js`, so a context that skips it fails the suite.

---

## 0 · THE ONE THING TO UNDERSTAND FIRST

**Every dependency below lives OUTSIDE the userscript, and every one of them fails SILENTLY.**
There is no exception. A companion that stopped, a chain that quietly stopped refreshing for one
symbol, a directory permission Chrome dropped on reload — each leaves a panel that looks completely
normal, renders every section, throws nothing, and is reading a book from another day.

**So no test file can tell you the system is working.** `test_deps.js` runs in Node, where there is
no browser, no companion, no market and no InsiderFinance. It proves the CHECK is correct. Only the
running panel can prove the SYSTEM is:

    __gptsDebug.deps()        the full verdict, per dependency, with ages and the three IF fields
    the `deps` dot            on the footer strip — red on a failure, amber on staleness
    the HEADER LAMPS          IRT · IF · YF · FF — one per external feed, each with a measurement

⚠ **A green suite has never once meant the dependencies are up.** Ask the panel.

### THE FOUR HEADER LAMPS — the whole outside world, in one line

| lamp | the integration | its number | fails into |
|---|---|---|---|
| **IRT** | the CSV this panel writes for IRT's FlexLevels extension | age of the last write | a stale file, not an error |
| **IF** | **InsiderFinance** — the chain, the walls, the expected move | age of the freshest symbol | an 11-day-old book that renders normally |
| **YF** | **Yahoo Finance** — `ES=F` 1-minute bars (§2) | age of the last pull | a WRONG high/low, drawn without complaint |
| **FF** | **ForexFactory** — the USD/high-impact calendar (§4) | **event count today**, not an age | a quiet Tuesday |

⚠ **FF IS COUNTED, NOT AGED.** It is delivered once a day, so an age would read "340m" on a
perfectly healthy calendar by mid-session. **`0ev` is a real, healthy answer** — the green dot is
what says the courier ran, not the number.
⚠ **Every lamp reads `depsHealth()`**, the same call the footer dot and `__gptsDebug.deps()` use, so
the three can never disagree about the same moment.

---

## 1 · INSIDERFINANCE — the call wall, the put wall, the expected move

**What it supplies.** The 0DTE and to-Friday chains for SPX / SPY / QQQ: the **call wall** (`lv.cr`),
the **put wall** (`lv.ps`), the magnet, max pain, and the **expected move** (`em.em`) — the ATM
straddle that the EM band is pinned from once per session.

**What breaks if it is stale or missing, and this is the part that matters:**

| Surface | Depends on | What a stale chain does |
|---|---|---|
| the EM band (`emBand`) | `dte0.em` | the band refuses → **the whole ladder section disappears**, because it lives inside it |
| the ladder's LEVELS | `ifLadder().rows` | draws another day's PDH / CW0 / FLIP over today |
| `dispScale` | chain spot vs our price | every SPXW strike lands at the wrong height on the rail |
| the node rail | `emPiles` clips to the band | wrong band → wrong nodes, or none |

⚠⚠ **One refusal upstream takes six surfaces with it.** That is not a hypothetical — it is v15.19,
where `emBand` returning `no EM` removed the ladder, the states, the percentages, the king lanes,
the roll arrows and the ROC column at once, with no error anywhere.

**Transport.** The panel never calls InsiderFinance. The companion userscript
(`current/gex-if-levels.user.js`, `@grant GM_xmlhttpRequest`) fetches and writes
`localStorage.gpts_if_chain_v1`; the panel reads it through `ifChain(sym)`. **A missing or outdated
companion is indistinguishable from a market with no data** unless you look at the age.

**How to check it.** `__gptsDebug.deps()` → items `if.SPX` / `if.SPY` / `if.QQQ` / `if.usable`.
Each reports its age in minutes and the three fields by name.

⚠⚠ **MEASURED FAULT, 2026-09-01:** the stored **SPY** chain was **15,328 minutes old** (payload
2026-08-21) with a **null expected move**, while SPX and QQQ were three minutes old — and the
record's own `stale` flag read **false**, because that flag is computed when the record is WRITTEN
and never re-evaluated. Nothing on the face said so.
**Rule: freshness is judged against the clock at the moment it is asked, never from a stored flag.**
⚠ Under the v15.06 SPX pin the companion stops fetching SPY, so a stale SPY is EXPECTED — which is
why the overall verdict asks "is there a usable book", not "is every symbol fresh". A check that is
red every day is a check nobody reads.

---

## 2 · YAHOO FINANCE (**YF**) — the ES 1-minute bars the ⓪a candle is measured from

**What it supplies.** Yahoo `ES=F` 1-minute bars into `localStorage.gpts_futbars_v1`, verbatim.
On a futures chart `measureBars()` serves these, so **the HOD, the LOD, the range, the open, PT, the
wick family and the candle image are all measured off them.**

**Cadence and backfill.** The request is `interval=1m&range=5d`, so **every poll backfills the whole
session** — opening the panel at noon still fills from 08:30 CT. Companion **v1.17** polls every
**5 minutes inside RTH**, hourly outside.
⚠ Before v1.17 it polled hourly, chosen when these bars only fed a nightly corpus. Panel v15.08 made
them a live input and nothing about the constant looked wrong. **The reason a constant was chosen can
expire without the constant looking wrong.**

⚠ **On a CASH chart there is no backfill.** `measureBars()` falls back to `closedCandles()`, which
holds only what the panel has seen since it was opened. The courier carries no cash bars.

⚠ **WHY IT IS COURIERED AND NOT FETCHED HERE.** Measured live from the Atlas page 2026-08-27: a page
`fetch()` of `query1.finance.yahoo.com` **fails** — the panel runs `@grant none` by necessity (the
fiber harvest needs page context), so it has no privileged transport. The companion's
`GM_xmlhttpRequest` is past CORS and CSP, and it is the only reason these bars exist here.

**How to check it.** `deps()` → `fut.courier` (age, row count), or the **YF** lamp in the header.

---

## 3 · IRT — writing the king levels out

**What it does.** `irtExportNow()` builds a CSV of the resolved levels and writes it **in place**
into a directory chosen with the File System Access API, which IRT's FlexLevels extension polls once
a minute.

**Three separate things can break it, and they fail differently — which is why `deps()` reports the
BUILD and the WRITE as separate items:**

1. **the directory handle** — Chrome drops the permission on reload and `requestPermission()`
   requires a user gesture, so a call from a timer REJECTS. The symptom is a stale file, not an
   error. (v14.52: diagnosed as a data problem for a whole session; it was a permissions problem.)
2. **the build** — no levels resolved for the configured symbols, so there is nothing to write.
3. **the write** — must use `createWritable({keepExistingData:true})` plus `truncate(byteLength)`.
   ⚠ The default `keepExistingData:false` is an ATOMIC REPLACE: the contents are correct and the
   file IDENTITY changes, so IRT keeps polling a file that no longer receives updates. The symptom
   was *"it only works after I refresh"*.

**How to check it.** `deps()` → `irt.build` (rows) and `irt.export` (age, error verbatim).
`OFF` means switched off in settings — a choice, not a fault.

---

## 4 · FOREXFACTORY (**FF**) — the only thing that knows today is an event day

**What it supplies.** ForexFactory's free weekly feed, `https://nfs.faireconomy.media/ff_calendar_thisweek.json`
— no key, impact-rated — filtered to **USD + high-impact only** and cached as
`localStorage.gpts_evcal_v1` = `{day, ev:[{t,title}]}`. `evCalActive()` reads it: **FOMC-family
events stamp the WHOLE day**; everything else is active within **±90 minutes** of its release
(`EVCAL_WIN_MIN`).

**What consumes it.** The event-day state on the read. An event day changes what the panel is
willing to say, so a missing calendar does not blank a section — **it quietly removes a caveat.**

**Cadence.** Once per day. Both sides can deliver it: the panel tries a plain `fetch` (the feed is
CDN-served for browser widgets and normally sends CORS) and **companion v1.14+ couriers it via
`GM_xmlhttpRequest` into the same key** — whichever arrives first wins, because both write
`{day, ev}` and both skip the work if today's is already there.

⚠⚠ **ZERO EVENTS IS A VALID DELIVERY, AND THIS IS THE ONLY SUBTLE PART.** `{day:today, ev:[]}` means
the courier ran and there are no USD-high releases — a real answer, and the most common one. The
**only** thing separating it from "the courier never ran" is the `day` stamp, which is why the check
tests the day and not the count. Counting events would have called every quiet day a failure.

⚠ **IT RAN AS AN UNCHECKED DEPENDENCY FROM v14.38 TO v15.37** — two code comments, nothing in this
file, no `deps()` item, no lamp. It could have been dead for a month without a mark on the face.
That is exactly the failure mode §0 describes, and it survived the writing of §0.

**How to check it.** `deps()` → `cal.ff` (state, event count, the day it was delivered for), or the
**FF** lamp in the header.

---

## 5 · THE RECORDER — no replay and no corpus without it

Writes 3-minute frames into `localStorage.gpts_recorder_v7`, later exported to `data/YYYY-MM-DD.json`
and read by the replay slider and every study in `tools/`.
⚠ `recorderBlind()` is the ONE gate all nine write paths call: **replay never writes.**
**How to check it.** `deps()` → `rec.storage`, and `__gptsDebug.storage()` for the budget detail.

---

## 6 · THE CHECK ITSELF

| | |
|---|---|
| live verdict | `__gptsDebug.deps()` — run it before diagnosing anything else |
| on the face | the four header lamps (IRT · IF · YF · FF), and the `deps` dot on the footer strip |
| the check is tested by | `test_deps.js` — that it notices each failure, and does not cry wolf |
| the code | `depsHealth()`, beside `__gptsDebug.deps` |

**Adding a dependency?** Four steps, and `test_deps.js` enforces all four:
1. an item in `depsHealth()`,
2. a **lamp** in `feedLampsHtml()` — with the right KIND of number for that feed (an age for a
   polled feed, a count for a once-a-day delivery),
3. a case in `test_deps.js` proving the check notices its failure and does not cry wolf,
4. **a section in this file.**

⚠ A dependency nobody can see the state of is one nobody will check — and ForexFactory proved that
writing the warning is not the same as obeying it. It sat undocumented for twenty-seven builds
*after* §0 was written.
