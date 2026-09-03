#!/usr/bin/env python3
"""
THE PLAN AS DATA — learning/plan.json: the WHAT (design/PURPOSE.md), the HOW (design/PROCESS.md) and the
incremental plan (roadmap/ROADMAP.md), in the shape the panel's ⚙ Architecture and 🗺 Roadmap tabs render.

    "we have lost so much. i think there should be a tab for roadmap so its not lost and there is a record
     and a tab for architecture (the how) which is focused on achieving my objectives. This way the future
     contexts will not forget about it."  - operator, 2026-09-03

ONE SOURCE, THREE PLACES. The markdown documents are the prose (for people and for `load gex`); this file is
the data the panel renders; `PLAN_SEED` in the userscript is a copy that renders before the first fetch.
test_v1559.js pins all three equal: every roadmap version in ROADMAP.md must be here, every loop stage in
PROCESS.md must be here, and the seed must equal this file. Edit HERE, run this script, then the docs.

Run:  python3 tools/plan-seed.py   -> writes learning/plan.json
"""
import json, io

PLAN = {
  "schema": 1,
  "written": "2026-09-03",
  "objective": {
    "one": "Find the day's HOD and LOD early enough to trade the move between them; secondarily, find the pullback turning points that resume a trend rather than end it.",
    "mechanism": "A gamma node deflects price, and the deflection IS the turning point.",
    "twoKinds": [
      {"kind": "TREND REVERSAL", "what": "a node at the extreme turns the day", "gives": "the HOD or the LOD", "trade": "turn around"},
      {"kind": "PULLBACK REVERSAL", "what": "a node stops a counter-move", "gives": "trend CONTINUATION", "trade": "stay in"}
    ],
    "expensiveError": "Confusing the pullback deflection (stay in) with the HOD/LOD deflection (turn around).",
    "quote": "The purpose of the application is to be able to identify two key turning points which are the high of the day and low of day, in order to profit from the move from high to low or low to high. As a side objective, it is also to identify pullback turning points (aka reversals or deflections). In order to do this I am relying on gamma levels.",
    "doc": "design/PURPOSE.md"
  },
  "tabs": [
    {"tab": "Dashboard", "role": "ACT", "line": "what the tables support right now, at the tap — THE READ, every rate with its n"},
    {"tab": "Analysis", "role": "ASK AND READ", "line": "the registry: every study by subject, its status, its result with n; the TRACK field"},
    {"tab": "Testing", "role": "TRUST AND PROMOTE", "line": "the register, the gate, what the ladder renders and why, the record, the nightly, the suite"},
    {"tab": "Architecture", "role": "THE HOW", "line": "the objective and the loop, as live status — is the machinery working today"},
    {"tab": "Roadmap", "role": "THE PLAN", "line": "what shipped, what is next, in order, each with the objective it serves; your enhancement requests"},
    {"tab": "Open Items", "role": "PROJECT MANAGEMENT", "line": "your issues and questions, with the review's answer under each"},
    {"tab": "Learn", "role": "TEACH", "line": "the deflection learning doc: your screenshots, my blind calls, what the record says, the rules, and the 0–100 gauge"}
  ],
  "stages": [
    {"n": 1, "id": "RECORD", "who": "browser", "what": "every bar's book and price (snaps), every tap (defl), every scored feature (feat), what he asked to track (requests), his issues, questions and enhancement requests (items)", "writes": "recorder store · IDB", "probe": "rec"},
    {"n": 2, "id": "EXPORT", "who": "browser", "what": "Save writes the day file — the day, the book, the ES bars, the requests", "writes": "data/<day>.json", "probe": "saved"},
    {"n": 3, "id": "PUSH", "who": "his machine", "what": "push-data.bat / the installer push to GitHub — the cloud cannot push", "writes": "GitHub main", "probe": "pushed"},
    {"n": 4, "id": "NIGHTLY", "who": "cloud", "what": "verdicts per hypothesis (read ONCE at minN), refreshes the tables, copies TRACK requests and open items, writes the log", "writes": "learning/log/<day>.json · SWEEPS*.json · requests.json · items.json", "probe": "nightly"},
    {"n": 5, "id": "REVIEW", "who": "cloud (LLM)", "what": "a READ becomes a hypothesis (predict + refuteIf fixed before the data), a request becomes a study row, an issue/question/enhancement gets its answer in items.json, a refuted rule retires, a taught screenshot becomes a learning example with the record's numbers (tools/node-lookup.py) and a rule L-n, FINDINGS written", "writes": "register.json · studies.json · items.json · deflections/examples.json · FINDINGS.md", "probe": "review"},
    {"n": 6, "id": "REGISTRY", "who": "cloud", "what": "studies.json (subjects → subsections → studies, result WITH n) and register.json are what the tabs render", "writes": "learning/studies.json · register.json", "probe": "registry"},
    {"n": 7, "id": "BUILD", "who": "cloud", "what": "code + mutation-tested assertions + docs + chat history; the installer carries EVERY file the panel fetches", "writes": "installvNNNN.bat", "probe": "suite"},
    {"n": 8, "id": "INSTALL", "who": "his machine", "what": "run the .bat (it pushes), click the Tampermonkey links, reload; the panel fetches the registry, the tables, the log", "writes": "the running panel", "probe": "version"},
    {"n": 9, "id": "GATE", "who": "browser", "what": "a feature whose rate does not move between its predicted bands cannot promote and cannot render a rate", "writes": "featGated", "probe": "gate"},
    {"n": 10, "id": "DASHBOARD", "who": "browser", "what": "a rate renders only from an earned tier that cleared the gate; THE READ turns today's tape into sentences from the tables", "writes": "the face · THE READ", "probe": "dashboard"},
    {"n": 11, "id": "SCORE", "who": "next day", "what": "what THE READ said is written to the day file and scored at the close — a read that cannot be wrong cannot improve", "writes": "reads[] · readScore", "probe": "score"}
  ],
  "rules": [
    {"rule": "a % is never rendered without its n", "test": "test_analysis_tabs 5a/5b · bareP · RATE_MIN_N"},
    {"rule": "a scorer must be able to fail before its rate means anything", "test": "featGated · test_v1554 7*"},
    {"rule": "a hypothesis is written with predict + refuteIf BEFORE the data and read ONCE at minN", "test": "register ↔ seed pin · the nightly's judge"},
    {"rule": "a first read is never a verdict — it becomes a register row and is read again on unseen sessions", "test": "H7.since · the ledger line"},
    {"rule": "every level named live is made the way the corpus made it", "test": "overnightHL().full · companion v1.18 · test_v1556"},
    {"rule": "every new assertion is mutation-tested", "test": "BUILD-CHECKLIST §2"},
    {"rule": "every file the panel fetches rides the installer", "test": "test_installer_manifest.js"},
    {"rule": "after an install, probe the live panel — never assume", "test": "this tab"},
    {"rule": "the mockup's look is the panel's look — one stylesheet, one skeleton", "test": "test_v1562 1a · 2e"},
    {"rule": "a deflection is never called from the picture alone — the record is looked up; the gauge cannot flatter", "test": "tools/node-lookup.py · test_v1562 3f/3g/3l"}
  ],
  "hardening": [
    "the deflection candidate score — the L-rules as a live score per node, measured by the ledger's outcomes (the gauge's predict part)",
    "score THE READ (stage ⑪)",
    "one definitions file read by Python and the panel, pinned equal",
    "the nightly reads one READ NEXT study per night and writes it back",
    "TRACK → DRAFT study as a nightly step",
    "the face manifest: every number on the face names its study",
    "a shipped-artifact test (smoke-test the installer's payload)",
    "data-quality checks on the face (courier age, ratio drift, bar gaps)"
  ],
  "roadmap": [
    {"v": "15.55", "title": "the Analysis tab by subject · TRACK · THE READ FROM THE STATS", "serves": "HOD/LOD · PULLBACK · the loop", "status": "shipped", "date": "2026-09-03"},
    {"v": "15.56", "title": "the book's levels in the sweep read · the book table · the honest overnight (companion v1.18)", "serves": "HOD/LOD · H6", "status": "shipped", "date": "2026-09-03"},
    {"v": "15.57", "title": "EM edges · VWAP + bands · developing profile · London · HVL/magnet · the two-line rule", "serves": "HOD/LOD · PULLBACK (interior levels are pullback candidates)", "status": "shipped", "date": "2026-09-03"},
    {"v": "15.58", "title": "the READ ranks reclaimed sweeps first · the Testing tab as mocked · the installer manifest carries every fetched file", "serves": "the loop", "status": "shipped", "date": "2026-09-03"},
    {"v": "15.59", "title": "the ⚙ Architecture and 🗺 Roadmap tabs — the WHAT, the HOW and the plan inside the app, as live status", "serves": "never forgetting the objective and the machinery", "status": "shipped", "date": "2026-09-03"},
    {"v": "15.60", "title": "the 📌 Open Items tab (issues, questions) and enhancement requests on the Roadmap — one field, one store, the review answers in the file", "serves": "complete application management: requirements → design → open items", "status": "shipped", "date": "2026-09-03"},
    {"v": "15.61", "title": "the ladder floor — never fewer than 8 strikes when the tape has them: sub-threshold strikes drawn dimmed as CONTEXT rows, display only (his 'only 3 strikes' ask)", "serves": "HOD/LOD · seeing the node around the King on a concentrated book", "status": "shipped", "date": "2026-09-03"},
    {"v": "15.62", "title": "the mockups' look is the panel's look (one stylesheet, one skeleton, the scale control) · the 📚 Learn tab — the deflection learning doc, four taught examples checked against the record, eight rules, the 0–100 gauge", "serves": "PULLBACK · HOD/LOD · learning to identify and predict deflections", "status": "next", "done": "the Analysis tab's skeleton equals the mockup generator's; a screenshot he teaches with renders on the Learn tab with the record's numbers, and the gauge cannot move on taught examples alone"},
    {"v": "15.63", "title": "the deflection candidate score — the L-rules (growth into the tap, fresh, stack, roll, King distance, time of day, level confluence) as a live 0–100 per node as price approaches it, measured by the deflection ledger's CONTINUED / STALLED outcomes — the gauge's predict part", "serves": "PULLBACK · HOD/LOD · 'predict a deflection will occur once you see price is going to the node'", "status": "later", "done": "30 scored calls exist and the gauge's predict part shows a Wilson lower bound, not a hope"},
    {"v": "15.64", "title": "score THE READ (stage ⑪) · MARK / STATE / polarity hovers say 'descriptive' until measured", "serves": "HOD/LOD · the loop closes", "status": "later", "done": "a scored READ line exists in a nightly log and renders on Testing ③ with its n"},
    {"v": "15.65", "title": "the TAP record — one row per fresh tap with the node's condition, both zones, extent, wasSessionExtreme", "serves": "HOD/LOD · PULLBACK · 94 OPEN studies · H6", "status": "later", "done": "40 taps recorded and the first F-study reads from them with n"},
    {"v": "15.66", "title": "the nightly reads one READ NEXT study per night and writes it back · TRACK → DRAFT study", "serves": "the loop closes on its own", "status": "later", "done": "a study changes status without a human editing the seed"},
    {"v": "15.67", "title": "one definitions file (Python + panel) · the shipped-artifact test", "serves": "hardening", "status": "later", "done": "changing a bin in one place fails the suite until the other side follows"},
    {"v": "15.68", "title": "the face manifest — every number on the face names its study, pinned by a test", "serves": "data-driven, enforced", "status": "later", "done": "a new number on the face without a study fails the suite"},
    {"v": "15.69", "title": "the pullback outcome — RESUME to a new extreme for VWAP/value-area and node taps inside a move", "serves": "PULLBACK", "status": "later", "done": "a resume rate with n renders on a tier-3 READ line"}
  ],
  "decisionsHis": [
    "the Skylit API backfill (~15–20k credits): years of taps in an afternoon — unblocks most OPEN studies",
    "NQ 1-minute beside ES for D5 (cross-book lead)",
    "fold the four low-value DAY columns (OF BAR ×2, PTN, BODY) into hovers, or keep them"
  ],
  "constraints": [
    "one install file per build, with the Tampermonkey links pasted as text; wait ~5 min, click, reload",
    "✅ SAVE DONE naming the files saved; chat history regenerated last; every new assertion mutation-tested",
    "do not tune a parameter to make a number look good",
    "cloud push is policy-denied — the installer pushes",
    "the integrations are untouched: IRT export, the Yahoo courier, InsiderFinance"
  ]
}

if __name__ == "__main__":
    io.open("learning/plan.json", "w", encoding="utf-8").write(json.dumps(PLAN, ensure_ascii=False, indent=1))
    print("wrote learning/plan.json ·", len(PLAN["stages"]), "stages ·", len(PLAN["roadmap"]), "roadmap items")
