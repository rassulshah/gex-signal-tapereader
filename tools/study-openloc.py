#!/usr/bin/env python3
"""
PRIOR-DAY VALUE AREA — does WHERE WE OPEN relative to it predict anything?

Operator, 2026-08-30: "if we open above prior day poc or vah or in between vah and Val, how does
that help us predict? What about combinations."

⚠ THIS IS A DIFFERENT QUESTION FROM THE ONE ALREADY ANSWERED. tools/study-profile.py asked whether
price TAGS these levels and found nothing: a prior POC was tagged 46.6% of sessions against 46.3%
for a SHAM level at the same distance. That killed them as support/resistance MARKERS. It says
nothing about whether the OPEN'S LOCATION inside or outside value carries information — which is
the market-profile claim, and is tested here for the first time.

⚠⚠ AND THIS ONE IS KNOWN AT THE OPEN. If it works it is worth more than the shipped GD/RD rule,
which cannot speak until ~09:03.
"""
import pickle, statistics, math, collections
S=pickle.load(open('/tmp/prof.pkl','rb')); ds=sorted(S)

R=[]
for i in range(1,len(ds)):
    d=ds[i]; p=S[ds[i-1]]['prof']; T=S[d]
    o,c,h,l = T['o'],T['c'],T['h'],T['l']
    va = p['vah']-p['val']
    if va<=0: continue
    if   o>p['vah']: loc='ABOVE VAH'
    elif o<p['val']: loc='BELOW VAL'
    elif o>p['poc']: loc='VALUE UPPER'
    else:            loc='VALUE LOWER'
    R.append(dict(d=d,loc=loc,green=1 if c>o else 0,o=o,c=c,h=h,l=l,
                  poc=p['poc'],vah=p['vah'],val=p['val'],va=va,
                  # did price come BACK into value at any point?
                  into_va = (l<=p['vah'] and h>=p['val']),
                  # did the day's extreme form AT a profile level (within 0.15 x VA width)?
                  hi_at = min(abs(h-p['poc']),abs(h-p['vah']),abs(h-p['val']))<=0.15*va,
                  lo_at = min(abs(l-p['poc']),abs(l-p['vah']),abs(l-p['val']))<=0.15*va))
n=len(R); base=sum(r['green'] for r in R)/n
print('n=%d sessions   base green %.1f%%\n'%(n,100*base))

print('%-13s %5s %8s %9s   %s'%('open is','n','green','vs base','reading'))
print('-'*64)
for loc in ['ABOVE VAH','VALUE UPPER','VALUE LOWER','BELOW VAL']:
    g=[r for r in R if r['loc']==loc]
    if len(g)<15: continue
    gr=sum(r['green'] for r in g)/len(g)
    se=math.sqrt(gr*(1-gr)/len(g))
    z=(gr-base)/se
    print('%-13s %5d %7.0f%% %+7.0f pp   %s'%(loc,len(g),100*gr,100*(gr-base),
          'nothing (|z|=%.1f)'%abs(z) if abs(z)<2 else '⇐ REAL, z=%+.1f'%z))

print('\nSIMPLER SPLIT — open above or below the prior POC (the one line most people use):')
for nm,f in [('above prior POC',lambda r:r['o']>r['poc']),('below prior POC',lambda r:r['o']<=r['poc'])]:
    g=[r for r in R if f(r)]
    gr=sum(r['green'] for r in g)/len(g); se=math.sqrt(gr*(1-gr)/len(g))
    print('  %-16s n=%3d  green %.0f%%  (%+.0f pp vs base, z=%+.1f)'%(nm,len(g),100*gr,100*(gr-base),(gr-base)/se))

print('\nOPEN OUTSIDE VALUE — does price come BACK into the value area?')
out=[r for r in R if r['loc'] in ('ABOVE VAH','BELOW VAL')]
ins=[r for r in R if r['loc'] not in ('ABOVE VAH','BELOW VAL')]
print('  opened outside value: n=%d, returned into value %.0f%% of the time'%(len(out),100*sum(r['into_va'] for r in out)/len(out)))
print('  (the classic market-profile claim is that this is HIGH — judge it against how often value is simply nearby)')

print('\nDO THE EXTREMES FORM AT PROFILE LEVELS?  (within 15% of the value-area width)')
print('  the HIGH sat at a profile level on %.0f%% of days'%(100*sum(r['hi_at'] for r in R)/n))
print('  the LOW  sat at a profile level on %.0f%% of days'%(100*sum(r['lo_at'] for r in R)/n))
print('  ⚠ needs a SHAM comparison before either number means anything — see below.')

# ---------------------------------------------------------------------------------------------
print('\n' + '='*70)
print('THE STEELMAN — profile theory claims RANGE and ROTATION, not direction')
print('='*70)
rngs=sorted(r['h']-r['l'] for r in R); med=rngs[len(rngs)//2]
print('\nDoes opening OUTSIDE value make a BIG-RANGE (trend) day?   median range %.1f pts'%med)
for nm,f in [('outside value',lambda r:r['loc'] in ('ABOVE VAH','BELOW VAL')),
             ('inside value', lambda r:r['loc'] not in ('ABOVE VAH','BELOW VAL'))]:
    g=[r for r in R if f(r)]
    big=sum(1 for r in g if (r['h']-r['l'])>med)/len(g)
    se=math.sqrt(big*(1-big)/len(g))
    print('  %-15s n=%3d   big-range day %.0f%%   (50%% by construction, z=%+.1f)'%(nm,len(g),100*big,(big-.5)/se))

# ⚠ SHAM CONTROLS. A level near price gets tagged because it is NEAR, not because it is a level.
import random
rnd=random.Random(5)
print('\nSHAM CONTROL — the same tests against FAKE levels at the same distances')
real_hi=sum(r['hi_at'] for r in R)/n; real_lo=sum(r['lo_at'] for r in R)/n
sh_hi=sh_lo=0
for r in R:
    off=[r['poc']-r['o'], r['vah']-r['o'], r['val']-r['o']]
    sham=[r['o']+rnd.choice([-1,1])*abs(x) for x in off]        # same distance, wrong side
    if min(abs(r['h']-x) for x in sham)<=0.15*r['va']: sh_hi+=1
    if min(abs(r['l']-x) for x in sham)<=0.15*r['va']: sh_lo+=1
print('  HIGH at a level:  real %.0f%%   sham %.0f%%   -> %s'%(100*real_hi,100*sh_hi/n,
      'REAL' if real_hi-sh_hi/n>.05 else 'DISTANCE EXPLAINS IT'))
print('  LOW  at a level:  real %.0f%%   sham %.0f%%   -> %s'%(100*real_lo,100*sh_lo/n,
      'REAL' if real_lo-sh_lo/n>.05 else 'DISTANCE EXPLAINS IT'))
sh_in=0
for r in R:
    if r['loc'] not in ('ABOVE VAH','BELOW VAL'): continue
    w=r['va']; mid=r['o']+(1 if r['loc']=='BELOW VAL' else -1)*(abs(r['o']-r['poc']))
    if r['l']<=mid+w/2 and r['h']>=mid-w/2: sh_in+=1
outn=sum(1 for r in R if r['loc'] in ('ABOVE VAH','BELOW VAL'))
print('  return INTO value: real 61%%   sham band at equal distance %.0f%%   -> %s'
      %(100*sh_in/outn,'REAL' if .61-sh_in/outn>.05 else 'DISTANCE EXPLAINS IT'))
