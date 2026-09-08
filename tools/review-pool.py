#!/usr/bin/env python3
# review-pool.py — POOL WHAT THE DAY FILES CAN ANSWER for a review (stage ⑤). Reads data/*.json with json (never cat).
#   python3 tools/review-pool.py [FROM-DATE] [OUT.json]      e.g.  python3 tools/review-pool.py 2026-08-25 /tmp/review-recent.json
# Per session: the feature-queue coverage (F-20). Pooled: every feature's n / scored / rate / votes / regime / grade / MFE / MAE
# from the queue; the farside touch call scored EXACTLY on the ES rows after each record, the NO call separately; the lodhod
# close-scored cells; levelstate by nearest state; Next Stop by rule / grade; PB Entry by rule / grade / state / polarity; the
# decision cells; the node-event outcomes by type and the ledger's deflect-on-touch by state (full-day sources); the wick
# family per session from the ES rows; the READ-vs-spine contradictions on the newest day. Written by the 2026-09-08 review.
import json, glob, os, collections, datetime, math
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
import sys
DAYS=[f for f in sorted(glob.glob('data/2026-0[89]-*.json')) if not f.endswith(('08-29.json','08-30.json')) and (len(sys.argv)<2 or os.path.basename(f)[:10]>=sys.argv[1])]
def ct(ms): return datetime.datetime.utcfromtimestamp(ms/1000-5*3600)
def wilson_lo(h,n,z=1.96):
    if n==0: return None
    p=h/n; den=1+z*z/n; c=p+z*z/(2*n); r=z*math.sqrt(p*(1-p)/n+z*z/(4*n*n)); return round(100*(c-r)/den)
out={'days':[], 'features':{}, 'featuresAfternoon':{}, 'farside':{}, 'lodhod':{}, 'levelstate':{}, 'nodeEvents':{}, 'ledger':{}, 'contradictions':[], 'calibration':{}, 'nextStop':{}, 'pbEntry':{}, 'wick':{}}
feat_pool=collections.defaultdict(lambda: {'n':0,'scored':0,'hits':0,'votes':collections.Counter(),'byRegime':collections.defaultdict(lambda:[0,0]),'byGrade':collections.defaultdict(lambda:[0,0]),'mfe':[], 'mae':[], 'days':set()})
fars=collections.defaultdict(lambda:[0,0])      # p-decile -> [n, traded]
fars_no=[0,0]; fars_no_days=collections.Counter()
lod_cells=collections.defaultdict(lambda:[0,0])  # table p bucket -> [n called, hit]
lod_first=[]  # first crossing of 70% per day: (day, cell p, outcome)
lvl=collections.defaultdict(lambda:[0,0])  # nearest state -> [n, hit]
nev=collections.defaultdict(lambda: collections.Counter())
ledger_state=collections.defaultdict(lambda: collections.Counter())
ledger_tot=collections.Counter()
ns_rule=collections.defaultdict(lambda:[0,0]); ns_grade=collections.defaultdict(lambda:[0,0])
pb_rule=collections.defaultdict(lambda:[0,0,0]); pb_grade=collections.defaultdict(lambda:[0,0,0]); pb_state=collections.defaultdict(lambda:[0,0,0]); pb_pol=collections.defaultdict(lambda:[0,0,0])
dec_cells=collections.defaultdict(lambda:[0,0])
wick_days=[]
for f in DAYS:
    d=json.load(open(f)); day=d['date']
    S=(d.get('snaps') or {}).get('SPY') or []; F=(d.get('feat') or {}).get('SPY') or []
    bars=sorted(set(r['bar'] for r in F if isinstance(r.get('bar'),(int,float))))
    cov=round(100*len(bars)/max(1,len(S))) if S else None
    info={'day':day,'version':d.get('version'),'snaps':len(S),'featBars':len(bars),'coverPct':cov,
          'featFrom':ct(bars[0]).strftime('%H:%M') if bars else None,'featTo':ct(bars[-1]).strftime('%H:%M') if bars else None,
          'snapFrom':ct(S[0]['t']).strftime('%H:%M') if S else None,'snapTo':ct(S[-1]['t']).strftime('%H:%M') if S else None,
          'nodeEvents':len(d.get('nodeEvents') or []), 'keys':len(set(r['key'] for r in F))}
    out['days'].append(info)
    trimmed = cov is not None and cov<60
    # ---- features pooled (every day with records; the afternoon-only flag rides along) ----
    for r in F:
        if not r.get('resolved'): continue
        k=r['key']; P=feat_pool[k]; P['n']+=1; P['days'].add(day)
        h=r.get('hit')
        rec=r.get('rec') or {}
        v=None
        for vk in ['verdict','side','dir','rollDir','call','vote']:
            if isinstance(rec,dict) and rec.get(vk) not in (None,'',0): v=rec.get(vk); break
        if v is not None: P['votes'][str(v)]+=1
        rg=(rec.get('regime') or {}).get('tag') if isinstance(rec,dict) else None
        if h is not None and h is not False and h is not True or isinstance(h,bool):
            pass
        if h is not None:
            P['scored']+=1; hit=1 if (h is True or h==1) else 0; P['hits']+=hit
            if rg: P['byRegime'][rg][0]+=1; P['byRegime'][rg][1]+=hit
            g=rec.get('grade') if isinstance(rec,dict) else None
            if g: P['byGrade'][g][0]+=1; P['byGrade'][g][1]+=hit
        if isinstance(r.get('mfe'),(int,float)): P['mfe'].append(r['mfe'])
        if isinstance(r.get('mae'),(int,float)): P['mae'].append(r['mae'])
        # farside: score the touch call per level against the day's bars after the record
        if k=='farside' and isinstance(rec,dict) and rec.get('ok') and rec.get('levels'):
            # the far side = the OTHER extremity; a level traded if the ES close-path reached it. Use futBars ES rows after the bar.
            pass
        if k=='lodhod' and isinstance(rec,dict) and rec.get('ok') and r.get('atClose'):
            p=rec.get('p'); called=rec.get('called')
            if isinstance(p,(int,float)) and called:
                b=f"{int(p//20)*20}-{int(p//20)*20+19}"; lod_cells[b][0]+=1; lod_cells[b][1]+=1 if h else 0
        if k=='levelstate' and isinstance(rec,dict) and rec.get('nearest'):
            st=rec['nearest'].get('st');
            if st and h is not None: lvl[st][0]+=1; lvl[st][1]+=1 if h else 0
        if k=='nextStop' and isinstance(rec,dict) and h is not None:
            ns_rule[rec.get('rule')][0]+=1; ns_rule[rec.get('rule')][1]+=1 if h else 0
            ns_grade[rec.get('grade')][0]+=1; ns_grade[rec.get('grade')][1]+=1 if h else 0
        if k=='pbEntry' and isinstance(rec,dict):
            o=r.get('out') or {}
            touched=o.get('touched'); hh=o.get('hit')
            for key,dd in [(rec.get('rule'),pb_rule),(rec.get('grade'),pb_grade),(rec.get('state'),pb_state),(rec.get('pol'),pb_pol)]:
                dd[key][0]+=1
                if touched: dd[key][1]+=1
                if hh: dd[key][2]+=1
        if k=='decision' and isinstance(rec,dict) and h is not None:
            cell=rec.get('cell') or (str(rec.get('dirGrade',''))+'x'+str(rec.get('nodeGrade','')))
            dec_cells[cell][0]+=1; dec_cells[cell][1]+=1 if h else 0
    # ---- farside touch call, scored exactly on the ES rows ----
    FB=((d.get('futBars') or {}).get('ES') or {}).get('rows') or []
    es=[(r[0],r) for r in FB if r and isinstance(r[0],(int,float))]
    dayD=datetime.date(*map(int,day.split('-')))
    esday=[(t,r) for t,r in es if datetime.datetime.utcfromtimestamp(t-5*3600).date()==dayD and 8*60+30<=datetime.datetime.utcfromtimestamp(t-5*3600).hour*60+datetime.datetime.utcfromtimestamp(t-5*3600).minute<15*60]
    for r in F:
        if r['key']!='farside' or not r.get('resolved'): continue
        rec=r.get('rec') or {}
        if not (isinstance(rec,dict) and rec.get('ok') and rec.get('levels') and esday): continue
        t0=r['t']/1000; after=[x for t,x in esday if t>=t0]
        if not after: continue
        lo=min(x[3] for x in after); hi=max(x[2] for x in after)
        side=rec.get('side')
        for L in rec['levels']:
            p=L.get('p'); px=L.get('px')
            if not isinstance(p,(int,float)) or not isinstance(px,(int,float)): continue
            traded = (lo<=px) if side=='LOD' else (hi>=px)
            dec=f"{int(min(p,99)//10)*10}s"; fars[dec][0]+=1; fars[dec][1]+=1 if traded else 0
            if p<=20:
                fars_no[0]+=1; fars_no[1]+=1 if traded else 0
                if traded: fars_no_days[day]+=1
    # ---- node events (full day) ----
    for e in d.get('nodeEvents') or []:
        ty=e.get('ty'); o=(e.get('o10') or {}).get('k') if isinstance(e.get('o10'),dict) else None
        if ty and o: nev[ty][o]+=1
    # ---- ledger (full day) ----
    LG=(d.get('ledger') or {}).get('SPY')
    if isinstance(LG,dict) and isinstance(LG.get('nodes'),dict):
        for kk,nd in LG['nodes'].items():
            for tch in nd.get('touches') or []:
                st=tch.get('state'); rc=tch.get('react')
                if st and rc: ledger_state[st][rc]+=1
            for c in ['deflect','through','stall']: ledger_tot[c]+=nd.get(c) or 0
    # ---- wick family from ES rows (the E row's A values), for the record ----
    if esday:
        op=esday[0][1][1]; hi=max(esday,key=lambda x:x[1][2]); lo=min(esday,key=lambda x:x[1][3])
        first='HOD' if hi[0]<lo[0] else 'LOD'; firstT=(hi if first=='HOD' else lo)[0]; secT=(lo if first=='HOD' else hi)[0]
        wend=None
        for t,x in esday:
            if t<firstT: continue
            if (x[4]<=op if first=='HOD' else x[4]>=op): wend=t; break
        wick_days.append({'day':day,'dow':dayD.strftime('%a'),'open':op,'hod':hi[1][2],'hodT':datetime.datetime.utcfromtimestamp(hi[0]-5*3600).strftime('%H:%M'),'lod':lo[1][3],'lodT':datetime.datetime.utcfromtimestamp(lo[0]-5*3600).strftime('%H:%M'),
            'first':first,'took':round((firstT-esday[0][0])/60),'wend':datetime.datetime.utcfromtimestamp(wend-5*3600).strftime('%H:%M') if wend else None,
            'bop':round((wend-firstT)/60) if wend else None,'mud':round((secT-wend)/60) if wend else None,'rng':round(hi[1][2]-lo[1][3],2),'close':esday[-1][1][4],'green':esday[-1][1][4]>op,'bars':len(esday)})
# ---- contradictions on 09-04: READ vs the direction spine, per bar ----
d=json.load(open('data/2026-09-04.json'))
for r in (d.get('feat') or {}).get('SPY') or []:
    if r['key']!='dir': continue
    rec=r.get('rec') or {}; rd=rec.get('read') or {}
    if not rd: continue
    v=rec.get('verdict'); rv=rd.get('verdict'); leg=rd.get('legDir')
    agree = (v=='DN' and rv=='BEARISH') or (v=='UP' and rv=='BULLISH') or (v=='SIDE' and rv in ('NEUTRAL','SIDE',None))
    if not agree:
        out['contradictions'].append({'bar':ct(r['bar']).strftime('%H:%M'),'spine':v,'capped':rec.get('capped'),'read':rv,'legDir':leg,'dirSrc':rd.get('dirSrc'),'grade':rec.get('grade'),'hit':r.get('hit'),'sentence':(rd.get('sentence') or '')[:110]})
def pack(P):
    o={'n':P['n'],'scored':P['scored'],'hits':P['hits'],'rate':round(100*P['hits']/P['scored']) if P['scored'] else None,'days':len(P['days']),
       'votes':dict(P['votes'].most_common(6)),'byRegime':{k:{'n':v[0],'rate':round(100*v[1]/v[0]) if v[0] else None} for k,v in P['byRegime'].items()},
       'byGrade':{k:{'n':v[0],'rate':round(100*v[1]/v[0]) if v[0] else None} for k,v in sorted(P['byGrade'].items())},
       'mfe':round(sum(P['mfe'])/len(P['mfe']),2) if P['mfe'] else None,'mae':round(sum(P['mae'])/len(P['mae']),2) if P['mae'] else None}
    tot=sum(P['votes'].values());
    if tot and P['votes']: top=P['votes'].most_common(1)[0][1]; o['oneWay']= top/tot>=0.9
    return o
out['features']={k:pack(P) for k,P in feat_pool.items()}
out['farside']={'touchByP':{k:{'n':v[0],'traded':v[1],'rate':round(100*v[1]/v[0]) if v[0] else None} for k,v in sorted(fars.items())},'noCall':{'n':fars_no[0],'traded':fars_no[1],'rate':round(100*fars_no[1]/fars_no[0]) if fars_no[0] else None,'daysWithFailedNo':dict(fars_no_days)}}
out['lodhod']={'atCloseByTableP':{k:{'n':v[0],'hit':v[1],'rate':round(100*v[1]/v[0]) if v[0] else None} for k,v in sorted(lod_cells.items())}}
out['levelstate']={k:{'n':v[0],'rate':round(100*v[1]/v[0]) if v[0] else None} for k,v in lvl.items()}
out['nodeEvents']={ty:{'n':sum(c.values()),**{k:v for k,v in c.items()},'deflectOrPinPct':round(100*(c.get('DEFLECT',0)+c.get('PIN',0))/max(1,sum(c.values())-c.get('NOTOUCH',0))),'breakPctOfTouched':round(100*c.get('BREAK',0)/max(1,sum(c.values())-c.get('NOTOUCH',0))),'touched':sum(c.values())-c.get('NOTOUCH',0)} for ty,c in nev.items()}
out['ledger']={'byStateAtTouch':{st:{'n':sum(c.values()),**dict(c),'deflectPct':round(100*c.get('deflect',0)/max(1,sum(c.values())))} for st,c in ledger_state.items()},'totals':dict(ledger_tot)}
out['nextStop']={'byRule':{k:{'n':v[0],'rate':round(100*v[1]/v[0]) if v[0] else None} for k,v in ns_rule.items()},'byGrade':{k:{'n':v[0],'rate':round(100*v[1]/v[0]) if v[0] else None} for k,v in ns_grade.items()}}
def pb(dd): return {str(k):{'n':v[0],'touched':v[1],'held':v[2],'touchPct':round(100*v[1]/v[0]) if v[0] else None,'holdOfTouchedPct':round(100*v[2]/v[1]) if v[1] else None} for k,v in dd.items()}
out['pbEntry']={'byRule':pb(pb_rule),'byGrade':pb(pb_grade),'byState':pb(pb_state),'byPol':pb(pb_pol)}
out['calibration']={'decisionCells':{k:{'n':v[0],'rate':round(100*v[1]/v[0]) if v[0] else None} for k,v in sorted(dec_cells.items())}}
out['wick']={'days':wick_days}
json.dump(out,open(sys.argv[2] if len(sys.argv)>2 else '/tmp/review-stats.json','w'),indent=1,default=str)
print('days'); [print(' ',json.dumps(x)) for x in out['days']]
print('\nwick days'); [print(' ',json.dumps(x)) for x in wick_days]
print('\nnodeEvents'); [print(' ',ty,json.dumps(v)) for ty,v in out['nodeEvents'].items()]
print('\nledger', json.dumps(out['ledger']))
print('\nfarside', json.dumps(out['farside']))
print('\nlodhod', json.dumps(out['lodhod']))
print('\nlevelstate', json.dumps(out['levelstate']))
print('\nnextStop', json.dumps(out['nextStop']))
print('\npbEntry', json.dumps(out['pbEntry'])[:1500])
print('\ncalibration', json.dumps(out['calibration']))
print('\ncontradictions 09-04', len(out['contradictions'])); [print(' ',json.dumps(c)) for c in out['contradictions'][:12]]
