import sys, json, pandas as pd, numpy as np
sys.path.insert(0,'../../level-reversal-analytics/analytics/exh2')
from exhaustion_data import load_market, DEFAULT_MARKETS, DEFAULT_DATA
from exhaustion_signals import SignalConfig, detect
from exhaustion_context import add_trend
import export_native_calibration as X
m, contract, cutoff = sys.argv[1], sys.argv[2], sys.argv[3]
cfgj=json.load(open('cfg.json'))
content,audit=X.build(cfgj,m,contract,cutoff)
open(f'cal_{m}.csv','w').write(content)
mc={**DEFAULT_MARKETS[m],**cfgj['markets'][m]}
bars,_=load_market(cfgj['root'],m,mc,{**DEFAULT_DATA,**cfgj['data']})
bars=bars[bars.contract==contract].reset_index(drop=True)
base={}
for line in content.splitlines():
    p=line.split(',')
    if p[0]=='BASE': base[int(p[1])]=(float(p[2]),float(p[3]),float(p[4]),int(p[5]))
mn=int([l for l in content.splitlines() if l.startswith('MIN_BASELINE')][0].split(',')[1])
for c in ('buy_ref','sell_ref','total_ref'): bars[c]=np.nan
ok=bars.sess>cutoff
for i in bars.index[ok]:
    b=base.get(int(bars.at[i,'slot']))
    if b and b[3]>=mn: bars.loc[i,['buy_ref','sell_ref','total_ref']]=b[:3]
cfg=SignalConfig(**cfgj['signal'])
f,e=detect(add_trend(bars,cfg),m,mc['tick'],cfg)
# bars for the host: END time as CT wall-clock epoch seconds
end=bars.end.dt.tz_convert('America/Chicago').dt.tz_localize(None)
out=pd.DataFrame({'t':((end-pd.Timestamp('1970-01-01'))//pd.Timedelta(seconds=1)).astype('int64'),'o':bars.o,'h':bars.h,'l':bars.l,'c':bars.c,'v':bars.v,'b':bars.bought,'s':bars.sold})
out.to_csv(f'bars_{m}.csv',index=False,header=False)
tr=add_trend(bars,cfg); ff=pd.DataFrame({'t':out.t,'trend':tr.trend.values,'atr':tr.atr_prior.values,'br':bars.buy_ref.values,'run':tr.signal_run_id.values,'elig':bars.signal_eligible.values}); ff.to_csv(f'py_trace_{m}.csv',index=False)
ev=e.copy(); ev['t']=[int(((pd.Timestamp(x).tz_convert('America/Chicago').tz_localize(None))-pd.Timestamp('1970-01-01'))//pd.Timedelta(seconds=1)) for x in f.loc[ev.signal_i,'end']] if len(ev) else []
ev[['t','kind','side']].to_csv(f'py_events_{m}.csv',index=False,header=False)
print(m,len(bars),'bars', e.kind.value_counts().to_dict(), 'after cutoff sessions', bars[ok].sess.nunique())
