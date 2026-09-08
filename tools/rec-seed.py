#!/usr/bin/env python3
"""
THE REVIEW'S RECOMMENDATIONS (v15.70) — the rows the review writes on the 💡 Rec tab, by id (R-n). The nightly writes the
machine's rows (RN-…) and applies his ✓ / ✗ (tools/nightly/recommend.py); this seed never touches those. Run it after
editing; it merges over learning/recommendations.json by id, so his decisions survive a regeneration.

    python3 tools/rec-seed.py    -> learning/recommendations.json (merged)

A row: id · kind (RULE · TEST · FEATURE · DATA · DESIGN · PROCESS · TEACH) · text (the recommendation, one sentence) ·
changes (what it changes on the face or in the loop) · evidence (with n and date where it is a number) · by 'review' ·
asOf. Set status 'implemented' + version here when a build ships it; 'withdrawn' + why when the review retracts it.
Operator, 2026-09-04: "based on the entire process and what you have learned you need to make recommendations and get
my approval to implement."
"""
import io, json, os, sys

ROWS = [
    dict(id='R-1', kind='FEATURE', by='review', asOf='2026-09-04',
         text='Record every READ the face shows — the HOD/LOD line, the direction call, the King verdict — as a claim with its inputs at the moment it is shown, and score it at the close (stage ⑪).',
         changes='directional prediction and the reads get a chain; the Learn gauge’s predict part comes alive',
         evidence='no read has ever been scored; the live lodhod scorer read 100% on 362 rows and could not fail (F-11)'),
    dict(id='R-2', kind='TEST', by='review', asOf='2026-09-04',
         text='When an Analysis row reads at n ≥ 15 with its Wilson low clear of the base, draft a register entry from it automatically — predict, refuteIf, minN 40, since = the next session — so it is tested only on sessions it has never seen.',
         changes='a count becomes a test without a session; the register fills while you sleep',
         evidence='PROCESS rule 3 (written before the data, read once at minN); 21 Analysis rows are mapped to their numbers'),
    dict(id='R-3', kind='DATA', by='review', asOf='2026-09-04',
         text='Add the clock as a class in the pattern table (time of the tap: first hour · midday · last hour).',
         changes='every rate can be split by the time of day; the F-19 confound (an early tap has more session ahead to be undercut) becomes measurable',
         evidence='F-19 names it; L7 (the taught legs cluster in the first hour and 12:00–13:15) is “not measured” until it exists'),
    dict(id='R-4', kind='DESIGN', by='review', asOf='2026-09-04',
         text='One knowledge file: the Learn tab’s rules (L-n, with the record’s verdict) and rules.json (what the face renders) become the same file, so the face can only say what the Learn tab knows.',
         changes='the dashboard draws from the knowledge base and nothing else; a contradicted rule cannot stay on the face',
         evidence='two files today; “it is from the learning that you can know something” (2026-09-04)'),
    dict(id='R-5', kind='FEATURE', by='review', asOf='2026-09-04',
         text='The candidate score (v15.82) built from tested rules — each factor’s weight is its out-of-sample rate — instead of hand-set weights.',
         changes='the pre-tap read shows a tested number, not a hope',
         evidence='no tested rule exists yet; the score waits for R-2 and the first clears'),
    dict(id='R-6', kind='PROCESS', by='review', asOf='2026-09-04',
         text='The review as a scheduled cloud session bound to your computer (runs the nightly’s log through the review and writes back over the bridge).',
         changes='the one stage that still waits for a session runs on a clock',
         evidence='stage ⑤ is the only manual step left after v15.68'),
    # (v15.71) asked and approved by him in the chat rather than on the tab — recorded here so the face change has its Rec row
    dict(id='R-7', kind='FEATURE', by='operator', asOf='2026-09-04',
         text='The save runs itself: after the close (15:01 CT and later, retried every 10 minutes until the file is confirmed in the repo folder) and, outside market hours, any earlier day captured but never written — the 💾 as the override; the footer’s 💾 chip says saved · pending · DUE.',
         changes='no click at the close; a missed day is written the next morning before the open; a save that cannot happen says so on the face',
         evidence='his ask 2026-09-04 (“automatically have the application trigger the save button instead of me clicking it … after market hours”); the record: the last seven day files exported at 15:01–15:03 CT by the old auto-export, which stopped trying at 16:00 and downloaded silently when the grant was missing',
         status='implemented', version='15.71', why='approved in the chat 2026-09-04 (“besides this i approve .. build”); shipped in v15.71'),
    # (v15.72) his three asks on the face, approved on the mockup in the chat — recorded so the face change has its Rec row
    dict(id='R-8', kind='FEATURE', by='operator', asOf='2026-09-04',
         text='The AFTER HOURS chip moves to the bottom of the panel; the King cards take the whole row and grow; the ladder font grows with its columns; the amber sliver inside the pattern blocks (a dead .g3pb rule’s border) goes.',
         changes='the King cards and the ladder are readable at a glance; nothing else on the face changes',
         evidence='his words 2026-09-04: “the after hours message to the left is bad choice, it is taking up too much space … more space for the king badges which you can make bigger. as well as the size of the font in the node ladder” · “there is yellow in the rectangle right before the purple”; measured: the cards sat at 462 of 649 px beside the chip',
         status='implemented', version='15.72', why='approved on the mockup 2026-09-04 (“yes .. build”); shipped in v15.72'),
    # (v15.73) the day line — his ask, approved on the mockup ("i like it.")
    dict(id='R-9', kind='FEATURE', by='operator', asOf='2026-09-05',
         text='A line at the bottom of the panel that says what the process did with the day: saved · analysis · testing · learning · rec, each with its evidence and a colour; red names the cause when a stage is overdue.',
         changes='the loop is visible on the dashboard without opening a tab; a stalled stage says so and why',
         evidence='his words 2026-09-05: “there needs to be some message at the bottom that tells me … 9/4 - data saved, analysis complete, testing complete, recommendations made. something descriptive”; every fact is already fetched by the panel',
         status='implemented', version='15.73', why='approved on the mockup 2026-09-05 (“i like it.”); shipped in v15.73'),
    dict(id='R-12', kind='FEATURE', by='operator', asOf='2026-09-07',
         text='The closed state: when the market has not opened today the dashboard stands on the last recorded session parked at its close — every section as it was — badged CLOSED, and returns to LIVE by itself with today’s first book.',
         changes='the session signal is the gamma payload’s own minute series, not the clock; the stale-day guard no longer evicts a replay on a holiday; the day line reads the last session, not “recording · 0 bars”',
         evidence='his words 2026-09-07 (Labor Day, at the panel): “the application doesn’t seem to support a frozen state from when it was open so i can really work on it … there is no node ladder”; measured on his panel: every feed arriving, the clock past 08:30, 0 bars, an empty face, a drag on Friday’s strip handed straight back to LIVE',
         status='implemented', version='15.75', why='approved in the chat 2026-09-07 (“ok.. fix”); shipped in v15.75'),
    # (v15.76) the IRT export carries the rest of Skylit's top-5 — his ask, the "which five" question asked and answered
    dict(id='R-13', kind='FEATURE', by='operator', asOf='2026-09-07',
         text='The IRT FlexLevels file carries the SPX top-5 gamma levels as white lines beside the Kings, CW0, PW0 and FLIP0: Skylit’s NODES=5 draws the five largest nodes by |%King| with the King always #1, so the King’s slot is the gold SPXW KING line already exported and the four that follow are G2–G5 — white, thin, solid, on EPU26 and SPY, held for the session like the Kings when the tape blinks.',
         changes='four white lines on his ES chart at the 2nd–5th largest SPX nodes, ranked by size not sign, from the same tape the SPXW King row reads; no G1 (one level, one line); IRT_LAST.gWhy names the four',
         evidence='his words 2026-09-07: “i want to update the irt export so it exports the top 5 levels for the spx also. G1 - G5. all should be white. in addition to this, it already exports the kings, cw0, pw0 and the flip”; asked which five, he chose “Mirror Skylit: G2–G5”; the setting map (SKYLIT-FEEDS.md, measured): NODES = 1/3/5/10/15/20 top-N — NODES=1 is the King alone',
         status='implemented', version='15.76', why='asked in the chat 2026-09-07; the one open question (is G1 the King?) answered “Mirror Skylit: G2–G5”; shipped in v15.76'),
    # (v15.77) the E row on top of the HOD line, per weekday — his ask on his own tool's strip, mockup 2 chosen
    dict(id='R-14', kind='FEATURE', by='operator', asOf='2026-09-08',
         text='The expected row sits on top of the HOD line: the weekday’s recent colour, the first extreme, then HOD/LOD · took · BOP · wick · W.End · wick% · MUD · the other extreme · HL gap · HL rng — every value a trimmed mean over the sessions of the SAME WEEKDAY (Fridays against past Fridays), with its n beside the E.',
         changes='one row above the HOD line; every expected field switches from all 284 sessions to the shown day’s weekday (55–60 sessions); the read’s timing prose names the same basis; the hold rates stay pooled; BASERATES.json carries byWeekday and the baked base byDow',
         evidence='his words 2026-09-08: “I want to see the expected row on top of the HOD at the top … I dont need Rly · Done · PB · Num · Ret · Risk · Ext · Tgt · Rwd · Dur · Time” · “use mockup 2, because it compares friday with fridays in the past and mondays with past mondays … this is a type of seasonality”; measured on the corpus: Fridays reach the first extreme in ~19m, Tuesdays ~46m; Thursdays range ~70 pts, Tuesdays ~54 (n=55–60 per weekday)',
         status='implemented', version='15.77', why='mockup 2 chosen in the chat 2026-09-08; shipped in v15.77'),
    # (v15.78) the day candle beside the ladder, with the sweep labels — his ask on a screenshot of the black space
    dict(id='R-15', kind='FEATURE', by='operator', asOf='2026-09-08',
         text='The developing day candle fills the space to the right of the node ladder, at the ladder’s height: the HOD and LOD with their clocks and the leg after each, the open and close ticks, MUD and its dollars, the PT line, the shape spine, the reversal names ON the wick to the left of the bar and the SWEPT line’s labels THROUGH it to the right, in the SWEPT line’s colours.',
         changes='one new element beside the ladder on a wide panel (measured after layout: under 110 px of free width nothing changes); dayCandleSvg takes a frame size; the sweep labels read the SWEPT line’s own events',
         evidence='his words 2026-09-08: “do you see the black space on the right of the app … I want the daily candle that is developing to be displayed there. you have the code for this and the labels also already” · on the mockup: “you have to add the sweep labels. after doing that, build”',
         status='implemented', version='15.78', why='mockup approved in the chat 2026-09-08 with one addition (the sweep labels); shipped in v15.78'),
    # (v15.79) the E row moved below the ladder and made bigger; the candle's sweeps are the shown day's
    dict(id='R-16', kind='FEATURE', by='operator', asOf='2026-09-08',
         text='The E row sits below the ladder and the day candle, above the SET line and the replay strip, at a readable size (values 10.5 px, labels 8.6 px) and spread across the whole row; the candle’s swept levels and the SWEPT line read the SHOWN day’s bars, so a parked Friday candle wears Friday’s sweeps.',
         changes='the row moves from the top to under the ladder block; bigger type, space-between; sweepEventsShown routes the candle and the SWEPT line to the replayed / parked day; futSessionBars can be anchored on a day and sorts its day keys numerically (from the 10th of a month “today” would have been the 9th)',
         evidence='his words 2026-09-08: “move the hod expected stats above the replay below the node ladder and the daily candle and make the font slightly bigger because it is very small and cant read it” · “make the font bigger and add some spacing so it takes up the row” · “on the daily candle you should indicate the levels that the hod and lod swept”',
         status='implemented', version='15.79', why='mockup chosen in the chat 2026-09-08 (“build”); the candle’s sweeps asked for mid-build; shipped in v15.79'),
    dict(id='R-17', kind='FEATURE', by='operator', asOf='2026-09-08',
         text='The day candle draws only the swept KEY levels, each at its own price on the price axis with the minute it swept; the reclaim of the open (↩ W.END · BOP) and MUD sit beside the open tick; the candle, the A row and GREEN/RED measure the shown day’s ES 1-minute bars in a replay or the closed state; the MUD dollars are converted once; ONH/ONL are the full Globex night; the prior session skips a holiday key; the King, the EM edges, PDC and the IB leave the key-level set.',
         changes='the right of the bar becomes a price axis (tick + name + minute, pushed apart on collision); the reversal names go; MUD moves left of the bar with the reclaim line above it; measureBars serves the courier’s ES bars for the shown day in a replay (closedCandles keeps the frames); mudUsd = |second extreme − open| × rr × $50; overnightHL joins the prior key’s evening (≥ 17:00) to the session’s morning, FULL when both halves are there; futSessionBars(1) walks over keys with no RTH bars unless `calendar`; LEVEL_TIER tier 1 = his list (+ PWH PWL WPOC by name); EMH/EML, PDC, IBH/IBL leave sweepLevelsToday',
         evidence='his words 2026-09-08: “the levels that are swept are the only ones that should be indicated. EMH and EML are not levels and they should be aligned based on the y axis which should be the price axis so the candle should show where it swept the level … a MUD of 0m doesn’t make sense. double check the values they are incorrect” · “the king is not a key level. the key levels are PDH, PDL, ONL, ONH, WH, WL, Prior day POC, VAH, VAL, Weekly Poc” · “I dont want IBL IBH PDC. you can keep CW0 and PW0. and POC is the prior day poc. VAH and VAL is also prior day VAH and prior day VAL” · “since the market is open you should show today”; measured: the parked Friday candle read the recorder’s frames (open 767.39 at 09:46, MUD 0m) against the ES bars (7742, 08:36, MUD 106m); $15,168 under MUD for a $1,512 leg; ONH/ONL never once the full night since v15.56; the Tuesday after Labor Day walked back onto a key with 0 RTH bars',
         status='implemented', version='15.80', why='his key-level list, in his words; three mockups in the chat 2026-09-08 (Friday, the key levels only, today from his own courier rows); shipped in v15.80'),
    dict(id='R-18', kind='FEATURE', by='operator', asOf='2026-09-08',
         text='The IRT export no longer writes the QQQ King converted onto the ES symbol; QQQ converts for NQ only.',
         changes='irtBuildCsv section 4 (the v14.75 rail-bearing row on EPU26, dashed, tilde-tagged) is removed; ENQU26 keeps its QQQ King; the rail still draws its ~ QQQ bearing on the face; xqWhy records the decision',
         evidence='his words 2026-09-08: “currently the irt export includes qqq converted for ES, remove this. qqq should only be converted for nq”',
         status='implemented', version='15.80', why='an unambiguous instruction mid-build; shipped in v15.80'),
    dict(id='R-19', kind='FEATURE', by='operator', asOf='2026-09-08',
         text='Every line the IRT export writes is solid — no dashed or dotted lines.',
         changes='irtCsvRow writes PENSTYLE 0 for every row (the 0DTE trio was dotted, the SPY King dashed); colour and width still tell the rows apart',
         evidence='his words 2026-09-08: “in the export for irt, i dont want dashed or dotted lines, make all solid lines”',
         status='implemented', version='15.80', why='an unambiguous instruction mid-build; shipped in v15.80'),
    dict(id='R-20', kind='FEATURE', by='operator', asOf='2026-09-08',
         text='The NQ symbol in the IRT export carries G2–G5 too — the QQQ book’s next four by |%King| after the King, converted by the NQ ratio, like the ES rows.',
         changes='irtQqqTop reads the same QQQ array the QQQ KING row reads (the face’s ladder first, the tape second), drops the exported King’s strike, ranks by size not sign, ties to the lower strike; four rows G2..G5 on ENQU26, white, width 1, solid, held day-scoped under GQ so a blind tick never deletes them; IRT_LAST.gqWhy',
         evidence='his words 2026-09-08: “Can you add additional levels for NQ as well, similar to how you have it for ES, so NQ would include G2-G5 also”',
         status='implemented', version='15.80', why='the ES rule mirrored on the QQQ book; shipped in v15.80'),
    dict(id='R-21', kind='FEATURE', by='operator', asOf='2026-09-08',
         text='The A row (today’s actuals) sits under the E row in the same columns, the two rows one aligned grid tagged E · TUE / A · TUE; MUD reads MU (up to a HOD, green) or MD (down to a LOD, red); the E row’s first badge is the expectation for the day (GREEN DAY / RED DAY, or DAY ? before the call); HOD cells green, LOD cells red.',
         changes='hlARowHtml built in secDay, emitted in secLoc right after the E row; hlMudLabel; the E row’s recent-six badge moves into the expectation chip’s hover (gdRead: the opening-range break confirmed by the first leg, 76% on 225 of 282 sessions); .hi / .lo colours on the extreme cells and MU / MD; the candle’s MU / MD wears the same colour; the two rows are one 13-column grid (.g3eag, display:contents rows) so every column lines up, tags E · TUE / A · TUE with the n, the basis and the date in their hovers, DAY ? / 1ST ? placeholders keep thirteen cells, eagFit falls back to wrapping rows below ~900 px',
         evidence='his words 2026-09-08: “Under the Expected row add the Today (Actual) row.” · “MUD should be dynamic, MU or MD” · “Try to use common sense colors for the expected and actual rows. for example green or red. MU should be green, MD should be red.” · “there should also be the expectation for the day, which is either a Green day or a Red day. you can place this before 1st HOD and dont need other badges that are there before 1st hod like 3/3 even, so you can use this space instead” · “the rows should be aligned. the firt badge can simply be E Tue and A Tue or something like that so the entire row is symmetrical”',
         status='implemented', version='15.80', why='four instructions in a row on the same row, mid-build, then the fifth — “the rows should be aligned. the first badge can simply be E Tue and A Tue … so the entire row is symmetrical” — after the first v15.80 installer; mockups of the two rows from his own courier rows in the chat; shipped in v15.80'),
    dict(id='R-22', kind='DATA', by='review', asOf='2026-09-08',
         text='Export the outcome queue (feat) from the IndexedDB archive the way the snapshots already are, and stop mirroring the snapshots into localStorage — so the day file carries the WHOLE session’s outcomes, not the last ~2 hours.',
         changes='featMergeRecs — one rule, LS ∪ archive by key|t (LS wins per record); repoUpsertFeat keeps the in-memory FEAT_ARCHIVE current (it was boot-time only); featStats reads LS ∪ archive for every day (it read the LS queue alone for a day LS still held — the live face had the same hole); repoFeatDay reads the IDB feat store by date; repoExportDay writes feat = LS ∪ archive with featSource {ls, archive, merged: n/from/to per symbol} and rebuilds the matrix from it; featHealth() reports archive / merged / mergedBars / the spans; day-digest.py carries featSource and names a pre-v15.82 file. NOT done: the snapshot mirror stays in localStorage — its readers (the forward labeller, the node-born seed, the day line) are synchronous; the archive already holds every bar and the shedder may keep trimming the mirror',
         evidence='F-20, mechanism found live 2026-09-08: the localStorage budget (3600 KB) shed the queue’s first bar from 08:36 to 09:00 by 10:44 CT (shed 1, recorder 3542 KB); the exported snapshots come from IDB, which is why they looked intact; F-10c named the archive on 08-31 · his words: “lets go with your recommendation”',
         status='implemented', version='15.82', why='his pick from the outstanding list, 2026-09-08 ~11:45 CT; shipped in v15.82 — featMergeRecs (LS ∪ archive, LS wins per record), the in-memory archive kept current by repoUpsertFeat, featStats through the merge, repoFeatDay + the export’s merged feat with featSource, the matrix rebuilt; the snapshot mirror in localStorage left as it is (its readers are synchronous; the archive already has every bar)'),
    dict(id='R-23', kind='FEATURE', by='review', asOf='2026-09-08',
         text='The IRT export’s SPXW and SPY rows take their ES prices straight from Skylit’s own ES1 derived rows (its live ratio, every minute) instead of the panel’s SPX→ES basis from the IF spot — “match Skylit always”.',
         changes='the feed observer keeps the ES1 and NQ1 gex/levels payloads (LASTFUTDER; never history over live); ensureFeeds self-fetches the one the app is not asking for, once a minute, 0DTE window, only while the export is on; skylitFutPx(futSym, book, strike) answers with the derived row’s k or strike × the same ratio; irtBuildCsv writes it for SPXW KING, G2–G5, SPY KING and the IF 0DTE rows (one ruler), and for QQQ KING + its G rows on NQ1; the basis is the fallback, tilde-tagged; IRT_LAST.skyWhy / nqSkyWhy and the gear’s IRT line say which ruler the file is on',
         evidence='2026-09-08 10:25 CT, same second from his tab: the same five SPXW strikes in the same order, 0.5–1.5 pt apart (7703.25 vs 7704.64 at 10:0x; 7705.00 vs 7704.45 at 10:25) because the IF spot refreshes on a 3-minute clock and Skylit’s ratio each minute; SKYLIT-FEEDS § THE ES1 BOOK · his words: “yes, i want them to match skylits own ES1 prices”',
         status='implemented', version='15.81', why='asked once after the projection reading, answered yes; shipped in v15.81 — LASTFUTDER, skylitFutPx, selfFetchFut, every ES and NQ row on the derived price with the basis as the tilde-tagged fallback'),
]

if __name__ == '__main__':
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'nightly'))
    import recommend as R
    doc = R.load()
    R.merge(doc, ROWS, None, {}, doc.get('asOf') or '2026-09-04', [])
    R.atomic_write(R.REC, json.dumps(doc, ensure_ascii=False, indent=1))
    print('wrote learning/recommendations.json ·', len(ROWS), 'review rows ·', doc['counts'])
