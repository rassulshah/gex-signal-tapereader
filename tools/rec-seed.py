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
         text='The candidate score (v15.79) built from tested rules — each factor’s weight is its out-of-sample rate — instead of hand-set weights.',
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
]

if __name__ == '__main__':
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'nightly'))
    import recommend as R
    doc = R.load()
    R.merge(doc, ROWS, None, {}, doc.get('asOf') or '2026-09-04', [])
    R.atomic_write(R.REC, json.dumps(doc, ensure_ascii=False, indent=1))
    print('wrote learning/recommendations.json ·', len(ROWS), 'review rows ·', doc['counts'])
