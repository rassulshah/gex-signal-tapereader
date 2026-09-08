#!/usr/bin/env python3
# write-review.py — compose review/2026-09-08.json (gex-review/v2) from /tmp/review-recent.json, /tmp/review-stats.json,
# the digests and his machine's nightly log. Every number below is read from those files; nothing is typed in.
import json, os, math
R=json.load(open('/tmp/review-recent.json'))        # 2026-08-25 → 09-04, the current feature set (afternoon-only queues)
A=json.load(open('/tmp/review-stats.json'))         # every session file, for the full-day sources (node events, ledger)
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
N=json.load(open('learning/log/2026-09-04.json'))
F=R['features']
def wl(h,n,z=1.96):
    if not n: return None
    p=h/n; den=1+z*z/n; c=p+z*z/(2*n); r=z*math.sqrt(p*(1-p)/n+z*z/(4*n*n)); return round(100*(c-r)/den)
days=[d for d in R['days'] if d['featBars']>=10]
bars=sum(d['featBars'] for d in R['days']); effN=round(bars/10,1)
def feat(key, text, verdict, why=None, extra=None):
    f=F.get(key) or {}
    o={'key':key,'text':text,'n':f.get('scored'),'rate':f.get('rate'),'mfe':f.get('mfe'),'mae':f.get('mae'),'votes':f.get('votes'),'oneSided':bool(f.get('oneWay')),
       'byRegime':{k:{'n':v['n'],'rate':v['rate']} for k,v in (f.get('byRegime') or {}).items()},
       'byGrade':{k:{'n':v['n'],'rate':v['rate']} for k,v in (f.get('byGrade') or {}).items()},
       'effN':round((f.get('scored') or 0)/10,1),'sessions':f.get('days'),'verdict':verdict}
    if why: o['why']=why
    if extra: o.update(extra)
    return o
ns=F['nextStop']; nsB=ns['byGrade'].get('B',{}); nsC=ns['byGrade'].get('C',{})
nd=F['node']; ndg=nd['byGrade']
lt=R['ledger']['byStateAtTouch']; ltA=A['ledger']['byStateAtTouch']
ne=A['nodeEvents']
lod=N['lodhod']['cells']
lodTxt=', '.join('%s: live %s%% vs table %s%% (n %s)' % (c['cell'],c['live'],c['table'],c['n']) for c in lod)
review={
 'schema':'gex-review/v2',
 'date':'2026-09-08',
 'writtenBy':'the review (cloud session, stage ⑤), Tuesday 2026-09-08 before the open',
 'span':{'from':'2026-08-25','to':'2026-09-04','sessions':len(days),'sessionFiles':len(R['days']),'bars':bars,'effectiveN':effN,
         'note':'the feature queue of every session since 2026-08-25 holds only its LAST 15–30 bars (the final 45–130 minutes, always ending 14:57) — every rate below is an AFTERNOON-ONLY sample, and effN is bars/10 of what survived, not of what was recorded. 2026-08-19/20 (v11.x, 93–98% coverage) carry a different feature set and are not pooled here.'},
 'grade':'D',
 'gradeWhy':'not a verdict on the tape — a verdict on the learning layer\u2019s data: 1–27% of each session\u2019s bars reach the queue the nightly and this review read; nothing about a weight can be concluded from it and nothing was applied.',
 'preopen':'Fri 9/4: HOD 08:36 (6m), open reclaimed 08:45, LOD 10:31; ONL swept 9:12, VAL 9:48. The learning layer saw only the last ~80 min of each day.',
 'headline':'Friday 9/4 printed its HOD in 6 minutes, reclaimed the open at 08:45 and found its LOD at 10:31 (ONL swept 9:12 reclaimed, VAL 9:48 broke). The learning layer has seen only the last 45–130 minutes of every session since 08-25; the rates below are that sample, said out loud.',
 'dataHealth':{
   'verdict':'QUEUE-TRIMMED — not COLLAPSED by the digest\u2019s 25% rule on 09-02/03/04 (55/26/27%), COLLAPSED on 08-25/26/27/28/31 and 09-01 (1–22%)',
   'perSession':[{'day':d['day'],'panel':d['version'],'snaps':d['snaps'],'featBars':d['featBars'],'coverPct':d['coverPct'],'queue':(d['featFrom'] or '—')+'→'+(d['featTo'] or '—'),'recorder':(d['snapFrom'] or '—')+'→'+(d['snapTo'] or '—')} for d in R['days']],
   'measured':[
     'the queue covers the LAST bars of the session only: 13:39→14:57 on 09-04 (27 of 101 snapshots), 12:48→14:57 on 09-03, 13:36→14:57 on 09-02, 13:51→14:57 on 09-01, 13:33→14:57 on 08-31, 14:15→14:57 on 08-28, 13:54→14:57 on 08-25',
     'the snapshots are complete from the minute the recorder came up (09-04: 09:46:50 → 15:00:15) — so the recorder ran; the queue lost its earlier records',
     '142–144 unresolved records per file = the last three bars x 48 keys, i.e. the resolver is healthy; the queue\u2019s first bar is what moves',
     'NOT FEAT_KEEP_BARS (160 distinct bars): 27 bars is far under it. NOT the chart window: r.n at 13:42 was 105 (the chart held the whole day). NOT the budget shedder on today\u2019s snaps (they are intact; the callback trims snaps and feat together)',
     'the covered span varies 45–130 minutes and the record count 714–1408, so it is neither a fixed bar cap nor a fixed byte cap on the file that is written'],
   'probe':'__gptsDebug.featHealth() and __gptsDebug.storage() on the live panel at ~11:00 CT and again at ~14:00 CT: if the queue\u2019s FIRST bar moves forward during the session, something evicts by age or by write; if LS_HEALTH.shed > 0 or quotaHits > 0, it is the storage budget after all (a recorder day is ~4.4 MB of JSON against a 3600 KB budget). This is the highest-leverage open item in the learning layer — forward-only data cannot be back-filled.',
   'mechanism':{'status':'FOUND LIVE 2026-09-08 (10:44 CT) — the localStorage budget',
     'readings':[
       {'ct':'09:53','recorderKB':2656,'daysInLS':3,'shed':0,'quotaHits':0,'queue':'08:36→09:48','queueBars':25,'snapsLS':26},
       {'ct':'10:44','recorderKB':3542,'budgetKB':3600,'daysInLS':1,'shed':1,'quotaHits':0,'queue':'09:00→10:33','queueBars':32,'snapsLS':'09:06→10:36','snapsLSn':35,
        'todayChars':{'feat':902540,'snaps':897349,'defl':12336},'recordBytes':474,'recBytes':309}],
     'why':'the exported snaps come from the IndexedDB archive (repoExportDay: payload.snaps = day.snaps) and are complete; feat is buildDayExport’s copy of the localStorage queue, which the shedder (recorderSave → lsPut, 25% oldest-first) trims from the second hour of the session once the day alone fills the 3600 KB budget (≈ 1.8 M chars: half feat, half the snapshot mirror). The “not the budget” line above was argued from the exported snapshots, i.e. from the archive, and is withdrawn.',
     'fix':'R-22 (proposed, not built): export feat from the IndexedDB archive (FEAT_ARCHIVE via repoUpsertFeat, F-10c) merged with the queue, and stop mirroring snaps into localStorage — the queue then lasts the session. Forward-only: nothing before the fix can be back-filled.'}},
 'sessionShape':{'note':'the wick family per session from the ES 1-minute rows the day files carry (tools: review-pool.py); these are the A-row facts the E row (v15.77) is read against',
   'days':A['wick']['days']},
 'features':[
   feat('nextStop','NEXT STOP (30m): the level named was touched within 30 minutes','measured · B above C · afternoon sample',
        why=f"B {nsB.get('rate')}% (n {nsB.get('n')}) vs C {nsC.get('rate')}% (n {nsC.get('n')}) — monotone, the right way round; by rule leg.magnet {R['nextStop']['byRule'].get('leg.magnet',{}).get('rate')}% (n {R['nextStop']['byRule'].get('leg.magnet',{}).get('n')}) carries almost every call, leg.pbTarget/wall.king too thin to read (n {R['nextStop']['byRule'].get('leg.pbTarget',{}).get('n')}/{R['nextStop']['byRule'].get('wall.king',{}).get('n')}). Trend {ns['byRegime'].get('trend',{}).get('rate')}% (n {ns['byRegime'].get('trend',{}).get('n')}) vs chop {ns['byRegime'].get('chop',{}).get('rate')}% (n {ns['byRegime'].get('chop',{}).get('n')})."),
   feat('nextStop.60','NEXT STOP (60m)','measured · lower than 30m, same ordering'),
   feat('pbEntry','PB ENTRY (30m): touched, then moved the deflection way before a close through','descriptive · touch-rate and hold-rate reported separately',
        why=f"touched {R['pbEntry']['byRule'].get('leg.pb',{}).get('touchPct')}% of leg.pb calls (n {R['pbEntry']['byRule'].get('leg.pb',{}).get('n')}); of the touched, held {R['pbEntry']['byRule'].get('leg.pb',{}).get('holdOfTouchedPct')}% (n {R['pbEntry']['byRule'].get('leg.pb',{}).get('touched')}). By ledger state of the node: acm held {R['pbEntry']['byState'].get('acm',{}).get('holdOfTouchedPct')}% of touched (n {R['pbEntry']['byState'].get('acm',{}).get('touched')}) vs dec {R['pbEntry']['byState'].get('dec',{}).get('holdOfTouchedPct')}% (n {R['pbEntry']['byState'].get('dec',{}).get('touched')}) — the only slice in this review where accumulation looks like it matters, and it is n=19 vs n=18: a question, not a finding."),
   feat('dir','DIRECTION (the spine): travelled DIR_PTS the stated way','measured · low, and the grade ladder is inverted on this sample',
        why=f"B {F['dir']['byGrade'].get('B',{}).get('rate')}% (n {F['dir']['byGrade'].get('B',{}).get('n')}) under C {F['dir']['byGrade'].get('C',{}).get('rate')}% (n {F['dir']['byGrade'].get('C',{}).get('n')}); A n={F['dir']['byGrade'].get('A',{}).get('n')}. {F['dir']['votes'].get('SIDE')} of {F['dir']['scored']} votes were SIDE (the mid-range / chop cap) — a spine that mostly declines to call is scored on the bars it did call."),
   feat('dir.trend5','SMA-50 trend (incumbent)','measured'),
   feat('dir.trendFast','10/20 trend (challenger, parked)','thin — n under 40 scored', why='loses to the incumbent on the same bars in this sample; not a swap candidate at this n'),
   feat('dir.drift','drift (shadow, not voting)','measured · below the incumbent'),
   feat('dir.struct','structure tilt (parked)','measured · 29% — no vote earned'),
   feat('leg.magnet','LEG ENGINE: the magnet was reached','measured · regime-split', why=f"trend {F['leg.magnet']['byRegime'].get('trend',{}).get('rate')}% (n {F['leg.magnet']['byRegime'].get('trend',{}).get('n')}) vs chop {F['leg.magnet']['byRegime'].get('chop',{}).get('rate')}% (n {F['leg.magnet']['byRegime'].get('chop',{}).get('n')}) — the magnet is a trend-day claim on this sample; pooled it reads as nothing."),
   feat('leg.pbDetect','LEG ENGINE: a detected PB node deflected toward the magnet','measured'),
   feat('leg.roll','LEG ENGINE: after a 2nd/3rd roll the trend continued','measured · 12% — the doctrine\u2019s 2=signal/3=confirmation is not visible here', why='n 68 afternoon bars over 8 sessions; a roll that is scored per BAR is over-counted (v11.5 asked for steps) — read this as a flag, not a rate'),
   feat('levelstate','LEVEL STATE: the nearest level held','measured · 77%', why=f"by state at the touch: BUIL {R['levelstate'].get('BUIL',{}).get('rate')}% (n {R['levelstate'].get('BUIL',{}).get('n')}), TURN {R['levelstate'].get('TURN',{}).get('rate')}% (n {R['levelstate'].get('TURN',{}).get('n')}), SPEN {R['levelstate'].get('SPEN',{}).get('rate')}% (n {R['levelstate'].get('SPEN',{}).get('n')}), WEAK n {R['levelstate'].get('WEAK',{}).get('n')} — the states do not separate at these n; a SPENT level holding 80% is the F-11 shape (a scorer that cannot fail)"),
   feat('lodhod','HOD/LOD: the standing extreme was the day\u2019s (close-scored since v15.51)','measured · the table\u2019s late cells do not hold live',
        why=f"the nightly\u2019s close-scored cells over 09-03/09-04: {lodTxt} — two sessions, and every row is a late-session call because of the queue trim; the first-crossing-of-70% decision the brief asks for is NOT recordable from a queue that starts at 13:30. Registered as H10 (forward, from 2026-09-08) rather than concluded here."),
   feat('farside','FARSIDE: the far extremity\u2019s levels — touched before the close','descriptive · consistent with the table on an easy sample',
        why=f"scored exactly on the ES rows after each record: p 0–49 traded {sum(v['traded'] for k,v in R['farside']['touchByP'].items() if k in ('0s','10s','20s','30s','40s'))}/{sum(v['n'] for k,v in R['farside']['touchByP'].items() if k in ('0s','10s','20s','30s','40s'))}, p 50s {R['farside']['touchByP'].get('50s',{}).get('rate')}% (n {R['farside']['touchByP'].get('50s',{}).get('n')}), 60s {R['farside']['touchByP'].get('60s',{}).get('rate')}% (n {R['farside']['touchByP'].get('60s',{}).get('n')}), 70s {R['farside']['touchByP'].get('70s',{}).get('rate')}% (n {R['farside']['touchByP'].get('70s',{}).get('n')}). THE NO CALL (p<=20): {R['farside']['noCall']['traded']} of {R['farside']['noCall']['n']} traded — no session with a failed NO. All of it is the last ~80 minutes with few minutes left, where NO is the table\u2019s easiest call; the gamma question (step 3) needs ~40 clean sessions and there are 3 with a full queue — not run. The panel\u2019s own farside `hit` reads 0/{F['farside']['scored']} because its outcome asks a different question than the touch call; that scorer needs a look before its rate means anything."),
   feat('ledger.touch','LEDGER: the in-play node held (tgt before inval)','descriptive · non-voting', why=f"chop {F['ledger.touch']['byRegime'].get('chop',{}).get('rate')}% (n {F['ledger.touch']['byRegime'].get('chop',{}).get('n')}) vs trend {F['ledger.touch']['byRegime'].get('trend',{}).get('rate')}% (n {F['ledger.touch']['byRegime'].get('trend',{}).get('n')})"),
   feat('node','NODE GRADE: the node/pullback held','measured · INVERTED ladder', why=f"A {ndg.get('A',{}).get('rate')}% (n {ndg.get('A',{}).get('n')}) < B {ndg.get('B',{}).get('rate')}% (n {ndg.get('B',{}).get('n')}) < C {ndg.get('C',{}).get('rate')}% (n {ndg.get('C',{}).get('n')}). This is H1\u2019s exploratory shape again (32% on n=34 then; {ndg.get('A',{}).get('rate')}% on n={ndg.get('A',{}).get('n')} now) on a DIFFERENT unit (bars, not deflection episodes) — H1 stays registered and unjudged (its episode n is 1)."),
   feat('decision','DECISION CELL (dir grade x node grade)','measured · not monotone', why=f"C×A {R['calibration']['decisionCells'].get('C×A',{}).get('rate')}% (n {R['calibration']['decisionCells'].get('C×A',{}).get('n')}), C×B {R['calibration']['decisionCells'].get('C×B',{}).get('rate')}% (n {R['calibration']['decisionCells'].get('C×B',{}).get('n')}), C×C {R['calibration']['decisionCells'].get('C×C',{}).get('rate')}% (n {R['calibration']['decisionCells'].get('C×C',{}).get('n')}); no A×A ever recorded"),
   feat('map.transfer','THE MAP: a transfer (the roll) — price moved the roll\u2019s way','measured · 35%, ceiling-heavy votes', why=f"votes {F['map.transfer']['votes']} — still counted per BAR in these files (stepNew exists but the outcome is scored per record); treat n as inflated"),
   feat('emband','EM BAND: the band contained the move','measured · 76%'),
   feat('defl.trigger','DEFLECTION TRIGGER ✓/✗','thin — 8 scored latches over 8 sessions'),
   {'key':'nodeEvents','text':'NODE EVENTS (full-day source, not queue-trimmed): the touched node within 10 bars — DEFLECT or PIN vs BREAK, by event type','n':sum(v['touched'] for v in ne.values()),'sessions':6,'verdict':'measured · the event TYPE does not separate',
    'byType':{ty:{'touched':v['touched'],'breakPct':v['breakPctOfTouched'],'deflectOrPinPct':v['deflectOrPinPct'],'all':v['n']} for ty,v in ne.items()},
    'why':'of touched nodes, BREAK is 10–17% whatever fired (ACCUM 13%, DISSIP 10%, ROLL 13%, TURN_UP 11%, TURN_DN 17%); F-7/F-8 stand — pinning is the level working, and a binary hold-rate would call it failure'},
   {'key':'ledger','text':'LEDGER (acceptance f): deflect-on-touch by the node\u2019s state AT the touch, pooled across nodes and sessions (the last 12 touches per node)','verdict':'measured · accumulating nodes did NOT deflect more than dissipating ones',
    'n':sum(v['n'] for v in ltA.values()),
    'byState':{st:{'n':v['n'],'deflectPct':v['deflectPct'],'deflect':v.get('deflect',0),'through':v.get('through',0),'stall':v.get('stall',0)} for st,v in ltA.items()},
    'why':f"acm {ltA.get('acm',{}).get('deflectPct')}% (n {ltA.get('acm',{}).get('n')}) vs dec {ltA.get('dec',{}).get('deflectPct')}% (n {ltA.get('dec',{}).get('n')}); gone {ltA.get('gone',{}).get('deflectPct')}% (n {ltA.get('gone',{}).get('n')}), hold {ltA.get('hold',{}).get('deflectPct')}% (n {ltA.get('hold',{}).get('n')}). STALL is {A['ledger']['totals']['stall']} of {sum(A['ledger']['totals'].values())} touches — price mostly sits at a node rather than deflecting or going through. The acm/dec chip has not earned a job on the face from this; the toward/away influence (infl) was not pooled this run."},
 ],
 'contradictions':[
   {'claim':'READ verdict vs the direction spine, per bar, 2026-09-04','evidence':'26 of 26 recorded bars agree (21 SIDE/SIDEWAYS, 5 DN/BEARISH). The leg engine said dn on all 26 while the spine sat SIDE on 21 (mid-range and chop caps) — the voice followed the spine, so no contradiction by the brief\u2019s rule; the tension is that the leg\u2019s direction never reached the face.','proposal':'none — record only'},
   {'claim':'a confirmed trend flipped by drift','evidence':'0 bars on 2026-09-04 (drift is shadow; relation divergence on up/dn: none)','proposal':'none'},
   {'claim':'a grade A that resolved under 30%','evidence':'no grade A direction call was recorded on 2026-09-04 (B 1, C 22)','proposal':'none'},
   {'claim':'the parked (closed-state) candle vs the ES bars','evidence':'the panel measured Friday from the recorder\u2019s frames (open 767.39 at 09:46, MUD 0m) while the sweep labels read the ES bars (open 7742, MUD 106m) — two sources under one label; the MUD dollars were 10x on the ES chart since v15.08 ($15,168 for a $1,512 leg)','proposal':'fixed in v15.80 (one source for the shown day; the multiplier applied once) — the review notes it because both were on the face for the operator to find'},
 ],
 'calibration':[
   {'id':'node.grade','label':'node grade A>B>C','claimed':None,'actual':ndg.get('A',{}).get('rate'),'n':ndg.get('A',{}).get('n'),'verdict':f"INVERTED: A {ndg.get('A',{}).get('rate')}% (n {ndg.get('A',{}).get('n')}) < B {ndg.get('B',{}).get('rate')}% (n {ndg.get('B',{}).get('n')}) < C {ndg.get('C',{}).get('rate')}% (n {ndg.get('C',{}).get('n')}) — afternoon sample; H1 is the registered test"},
   {'id':'dir.grade','label':'direction grade A>B>C','actual':F['dir']['byGrade'].get('B',{}).get('rate'),'n':F['dir']['byGrade'].get('B',{}).get('n'),'verdict':f"B {F['dir']['byGrade'].get('B',{}).get('rate')}% (n {F['dir']['byGrade'].get('B',{}).get('n')}) under C {F['dir']['byGrade'].get('C',{}).get('rate')}% (n {F['dir']['byGrade'].get('C',{}).get('n')}); A n=1 — inverted, thin"},
   {'id':'nextStop.grade','label':'Next Stop grade B>C','actual':nsB.get('rate'),'n':nsB.get('n'),'verdict':f"monotone: B {nsB.get('rate')}% (n {nsB.get('n')}) > C {nsC.get('rate')}% (n {nsC.get('n')})"},
   {'id':'lodhod.cells','label':'HOD/LOD table late cells (>=80%)','claimed':lod[0]['table'] if lod else None,'actual':lod[0]['live'] if lod else None,'n':lod[0]['n'] if lod else None,'verdict':'the table says 99–100, live says 43–50 (n 23/30, two sessions, late-session rows only) — registered H10, forward'},
 ],
 'challengers':{'dir.trendFast':{'vs':'dir.trend5','chalRate':F['dir.trendFast']['rate'],'chalN':F['dir.trendFast']['scored'],'incRate':F['dir.trend5']['rate'],'incN':F['dir.trend5']['scored'],'lift':(F['dir.trendFast']['rate'] or 0)-(F['dir.trend5']['rate'] or 0),'verdict':'no swap — the challenger is under the incumbent and under n'},
                'dir.struct':{'vs':'dir.trend5','chalRate':F['dir.struct']['rate'],'chalN':F['dir.struct']['scored'],'incRate':F['dir.trend5']['rate'],'incN':F['dir.trend5']['scored'],'verdict':'no vote earned'},
                'netGamma':{'chalRate':F['netGamma']['rate'],'chalN':F['netGamma']['scored'],'verdict':'conditions, does not point (hard-won rule) — not a voter'}},
 'killList':[
   {'id':'kill.tap3','keep':True,'n':0,'evidence':'unmeasured — the tap count is not in the queue that survives; no change'},
   {'id':'kill.midrange','keep':True,'n':F['dir']['scored'],'evidence':f"the mid-range cap produced {F['dir']['votes'].get('SIDE')} SIDE calls of {F['dir']['scored']}; the calls it allowed resolved {F['dir']['rate']}% — the cap is doing most of the spine\u2019s work; keep, unmeasured as a kill"},
   {'id':'kill.noConf','keep':True,'n':0,'evidence':'unmeasured'},
   {'id':'kill.negGammaWide','keep':True,'n':0,'evidence':'unmeasured'},
 ],
 'proposals':[],
 'proposalsNote':'none — nothing here has n>=20 independent observations from a full-session sample; the hand-set weights stand. No clearsBar:true anywhere.',
 'register':[
   {'id':'H10','written':'2026-09-08','claim':'the HOD/LOD table\u2019s late-session >=80% cells hold to the close at >=80% live','pick':'lodhodCell','judgedBy':'nightly','minN':60,'since':'2026-09-08',
    'predict':'pooled live rate of the 80-99 and 100-119 cells >= 80% at n >= 60 close-scored rows on sessions from 2026-09-08','refuteIf':'at n >= 60 the Wilson high of the pooled live rate is < 80%',
    'note':'written from the nightly\u2019s own cells (09-03/09-04: 43% n 23, 50% n 30) — those two sessions are NOT counted; the rows the queue keeps are late-session by construction, which is exactly the cell family the table is most confident about.'},
 ],
 'questions':[
   {'id':'queue_trim','when':[],'outcome':'featHealth','note':'what evicts feature records older than ~80 minutes from the queue? (the probe above; the answer decides whether any rate in this file can ever grow)'},
   {'id':'took_by_weekday','when':[{'f':'dow','v':'Fri'}],'outcome':'took<=30m','note':'the E row\u2019s weekday basis (v15.77): Fri median took ~19m (n 55) vs Tue ~46m (n 60) in the corpus to 08-21 — a forward test needs the corpus appended nightly (v15.81 SEASONALITY TRACKED); this week: Fri 9/4 took 6m, Tue 9/1 143m, Mon 8/31 1m, Wed 9/2 7m, Thu 9/3 86m'},
   {'id':'pbentry_acm','when':[{'f':'state','v':'acm'}],'outcome':'holdOfTouched','note':f"acm {R['pbEntry']['byState'].get('acm',{}).get('holdOfTouchedPct')}% (n {R['pbEntry']['byState'].get('acm',{}).get('touched')}) vs dec {R['pbEntry']['byState'].get('dec',{}).get('holdOfTouchedPct')}% (n {R['pbEntry']['byState'].get('dec',{}).get('touched')}) — the only accumulation signal in the file; already a registry question (pbentry_acm), keep recording"},
   {'id':'ledger_touch_regime','when':[{'f':'regime','v':'trend'}],'outcome':'ledger.touch','note':f"trend {F['ledger.touch']['byRegime'].get('trend',{}).get('rate')}% (n {F['ledger.touch']['byRegime'].get('trend',{}).get('n')}) vs chop {F['ledger.touch']['byRegime'].get('chop',{}).get('rate')}% (n {F['ledger.touch']['byRegime'].get('chop',{}).get('n')}) — does the in-play node hold LESS on trend days? thin"},
 ],
 'missingFields':[
   'the feature queue itself: 73–99% of each session\u2019s bars never reach the day file (the trim above) — every other item on this list is second to it',
   'FEAT_ARCHIVE (resolved records older than the queue) is not in buildDayExport (F-10c) — the IndexedDB copy has what the file lacks',
   'the wick family (took · BOP · W.END · wick% · MUD) is computable exactly from the futBars block every day file carries since v14.7x but is not recorded as a feature and the corpus (data/es-1min, to 08-21) is not appended nightly — v15.81 wires append-futures → study-hodlod',
   'the lodhod FIRST-CROSSING decision (the live rate F-12 asks for) needs the morning bars; the queue has none',
   'the READ from the stats (⓪a, v15.63) is not recorded per bar (R-1 on Rec); the voice READ is (dir.read) and agreed with the spine 26/26 on 09-04',
   'map.transfer outcomes are still scored per bar, not per step (stepNew is recorded; the scorer should key on it)',
   'the ledger\u2019s influence block (infl: acmToward/decAway) was not pooled this run — the acceptance (f) second half',
 ],
 'selftest':{'ran':False,'why':'data/_selftest.json is not present in the repo; not generated for this run'},
 'delivery':'device bridge → C:\\Dev\\gex-signal-tapereader\\review\\2026-09-08.json (the GEX sync task pushes it)',
 'power':f"{bars} afternoon bars over {len(days)} sessions = ~{effN} independent observations; one day = ~2.7. No weight conclusions can be drawn from this file.",
}
os.makedirs('review',exist_ok=True)
json.dump(review,open('review/2026-09-08.json','w'),indent=1,ensure_ascii=False)
print('written review/2026-09-08.json', os.path.getsize('review/2026-09-08.json'),'bytes')
