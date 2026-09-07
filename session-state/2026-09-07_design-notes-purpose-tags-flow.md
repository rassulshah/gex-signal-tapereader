# 2026-09-06 / 07 — DESIGN NOTES: the purpose tags · the Rec voice · absorption and the footprint · the second project

_Written by the session on 2026-09-07 while the operator stepped away ("i will come back to this. the market is about
to open in 10 minutes"). Nothing here is built. It records what was SETTLED in conversation and what is WAITING on him,
so the next context does not re-litigate it. His words are quoted; everything else is the session's._

## 1 · SETTLED — the purpose tags (every unit of every stage carries them)

- **HOD/LOD** and **DEFLECTION**. Two, final. His definition: *"the deflection creates the turning points that are within
  the hod and lod whereas the hod and the lod are the extreme turning points during the day"*. DEFLECTION covers both
  *"when a rally ends and a pullback starts as well as when a pullback ends and continuation occurs"*. No PULLBACK or
  CONTINUATION tag (he: *"I don't think we need continuation because deflection could [cover both]"*).
- No THE PROCESS tag either (he: *"doesn't the process also serve deflections and hod lod?"*). Machinery (the save, the
  day line, the guard, the review on a clock) carries BOTH tags; what kind of thing a unit is stays on the existing
  KIND axis (RULE · TEST · FEATURE · DATA · DESIGN · PROCESS · TEACH).
- The principle, his words: *"You need to link the entire application especially the data collection, analysis
  testing, learning, recommendation etc to this purpose. The whole process serves the purpose."*
- Mechanism agreed in conversation: a `serves` tag on every study, hypothesis, Learn rule, Rec row, Roadmap item and
  recorded field; shown on every row of every tab; a test fails the build when a unit lacks one; PURPOSE.md gets the
  map from the record to the decision and his DEFLECTION definition verbatim (§2). A purpose scorecard (per tag, what
  we can claim with n) and the review's "no chain → cut list" were proposed for Rec, not built.

## 2 · WAITING ON HIS ✓ — v15.75, the Rec tab (mockup sent 2026-09-06: `mockups/mockup-rec-why-gain.png`)

- His asks: *"For each recommendation, it should have a reason, a benefit. I need to know how approving will benefit
  me as a trader. Also the fonts are too small for the recommendations and need to be bigger."*
- The mockup (the real tab rendered at 760 px): every row = one bold sentence, then **WHY · WHAT YOU GAIN · SERVES**
  (tags as chips), the builder's "changes:" moved to the hover, type ~1.3× (rows 10.4 px, body 9.3 px, bigger ✓ / ✗).
  The six rewritten rows are in `../rec-rows-v1575.json` in the scratchpad of that session and in the chat history;
  the SERVES for R-2 / R-4 / R-6 changed from THE PROCESS to HOD/LOD · DEFLECTION after §1. R-5 now says v15.76 (the
  score moves one more).
- Build plan for v15.75 once ✓: the six rows (why · gain · serves) in `tools/rec-seed.py` + the three implemented
  rows tagged; `recRowHtml` and the stylesheet changed in the mockup generator first, then spliced (the v15.62 rule);
  R-10 for the face change (by operator); PURPOSE.md §2 addendum; then the tag on Analysis / Testing / Learn rows,
  the PURPOSE map and the test as the step after.

## 3 · ABSORPTION — his requirement, and where the data can come from

- *"The thing we are missing is bid ask volume to help detect absorption."* Parked design §9 of
  `design/spec-v16-dashboard-deflection.md` already said: total volume only, cannot say who absorbed.
- His sharper requirement: *"the bid ask data volume position on the candle matters. For example if heavy bid volume
  at or near the low of the candle is different than at the top of the candle."* → the input is **delta at price**
  (the footprint), not per-bar delta. Per-bar buy/sell or CVD is at best a proxy to be tested.
- Skylit (measured map, SKYLIT-FEEDS.md): the gamma feed carries no volume; Flowseeker's Live Feed carries bid/ask
  SIDE per OPTIONS print (flow at the strike, not the underlying); the Atlas chart has VOLUME-BUY / VOLUME-SELL /
  CVD-SESSION toggles, OFF in his config, endpoint UNMEASURED — he was given three steps (toggle on, F12 → Network,
  paste the URLs). Per-bar at best.
- Port-sniffing IRT's CQG feed: declined (encrypted proprietary protocol; against the data agreements; brittle).
- StoneX / CQG Continuum: CQG WebAPI can be enabled by the broker on his user — three questions for StoneX (enable
  WebAPI; separate API login so IRT is not disconnected; the host). Not asked yet.
- thinkorswim: no tape export; Schwab API has no time & sales stream; not a source for the footprint.
- **Investor/RT, the documented path (looked up 2026-09-07):** ExportData (RTX) writes tokenized CSVs on a timer;
  Volume Breakdown (VB) gives true bid/ask volume PER BAR (RTL token `VB`); VolumeScope (the footprint) has no export;
  TimeAndSales (RTX) has no logging; the Profile's Export Data is manual. → **The RTX SDK**: C++ extensions run inside
  IRT, the developer API exposes "footprint buy/sell volume data per bar or price", the SDK ships with the install
  (`C:\Program Files\LinnSoft\InvestorRT\sdk\` — `c`, `c++`, `docs`; header `sdk\c++\include\irtsdk.h`), built with
  Visual Studio 2022 Community via File → New → RTX Extension, DLL to `InvestorRT\dllx64`. ExportData itself is an
  RTX extension that writes files.
- Folder access to the SDK was requested over the bridge; the approval came from a phone notification and was not
  granted; he will approve from the desktop ("I will approve when I am in front of my computer").

## 4 · THE SECOND PROJECT — reading futures data from IRT via the SDK (his ask, 2026-09-07)

- *"I want to build an application that can read the futures data and it sounds like you can get access to this
  data via irt sdk."* Shape agreed in conversation: (1) a small C++ RTX extension = the tap, one JSON line per closed
  bar per market (time, OHLC, per-price bid volume and ask volume, delta) into `data\flow\<market>\<day>.jsonl` in
  the repo folder; (2) Python analysis on his machine like the nightly; (3) the panel stays the face; the sync task
  pushes. I write the C++ and Python; he builds the DLL in VS2022 with numbered steps.
- **The purpose of the second project — his words, being drafted:** *"I am a trader so similar to the tape reader
  project I am interested in finding hod lod and deflections to trade. … I want to get in the market where it makes
  sense so that I can profit by buying low and selling high and that is done by catching a reversal. This is done
  typically on a pullback. The hod and lod are extreme reversals that are difficult to catch but that's fine because
  the idea is that once one extremity is in we can buy or sell a pullback on its way to the other extremity. Of
  course this pullback needs to be a good one that may have elements like absorption which means I will be buying and
  selling with large traders which is always a good idea."*
- What makes a pullback good, his words: *"… scored and tested for each market. … pullbacks that may be at VWAP or
  may be sweeps of other nearby pullbacks, for example let's say you have a rally to 10 and a pullback to 8 and then
  the market comes up a little bit to 9 but then dips to just below the prior pullback at 8 by going slightly lower to
  7.80 and there is absorbing after this intraday sweep. That may be a good place to long the pullback because the
  breakdown below 8 is shallow and being absorbed. The other thing is the gamma nodes. If the pullback is tapping a
  certain type of king node or a [PIKA] stack it maybe a great opportunity given the scoring. This is why we are
  having the tape reader save data in the first place so the deflections can be scored."*
- The draft purpose sent to him (two paragraphs + the "good pullback" paragraph) is in the chat history of this
  session; not yet his. The join: the tapereader says WHERE a pullback should end (the node), the second project says
  WHETHER it is ending (absorption); the trade is where they agree. The factor families for R-5's score: gamma
  (existing) + level (VWAP · prior-pullback sweep · King type · PIKA stack) + flow (absorption at the tap), per market
  via `learning/markets.json`.
- Open question to him: which markets first (recommended ES + NQ, then GC and CL).

## 5 · SWEPT on NQ (his open item 4) — proposed as R-11, waiting on ✓

- SWEPT is ES-only because `futSessionBars` reads the courier's ES rows; NQ 1-minute bars are couriered since
  companion v1.18 (v1.15+) and read by nothing. Needs: the read per market; the face (one SWEPT line following the
  chart's instrument via FUTMODE, or a second line — mockup both); the NQ base rates (no corpus — an NQ 1-minute
  history export from IRT builds it in one night, the way `EPM26-1min.csv` built ES's).
- Question to him: does he have NQ 1-minute history in IRT to export?

## 6 · ALSO WAITING — unchanged

- The Network-tab look at CVD-SESSION / VOLUME-BUY / VOLUME-SELL (three steps given).
- The three StoneX questions (WebAPI · separate login · host) if the RTX route disappoints.
- Monday 2026-09-07 is Labor Day (no options session; the gamma book does not move); the first stamped session is
  Tuesday 2026-09-08 as the register's `since` already says. The day line keeps 9/4 until the first bar of the next
  session.
