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
         text='The candidate score (v15.73) built from tested rules — each factor’s weight is its out-of-sample rate — instead of hand-set weights.',
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
]

if __name__ == '__main__':
    sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'nightly'))
    import recommend as R
    doc = R.load()
    R.merge(doc, ROWS, None, {}, doc.get('asOf') or '2026-09-04', [])
    R.atomic_write(R.REC, json.dumps(doc, ensure_ascii=False, indent=1))
    print('wrote learning/recommendations.json ·', len(ROWS), 'review rows ·', doc['counts'])
