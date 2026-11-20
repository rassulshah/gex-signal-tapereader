// tp_reader_2400.js - menthorq-reader.user.js (Reader 3.10.x, staged 2026-11-20 09:46 CT) todayPlanHtf() and its helpers, copied verbatim
// by gen_2400.py: the reference the C++ TodayPlan.h is checked against (t_hl2400.cpp via golden_2400.txt)
  const esc = s => String(s == null ? '' : s).replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
  const MQAB = [[/^0DTE Call Resistance/, 'CR0'], [/^Call Resistance/, 'CR'], [/^0DTE Put Support/, 'PS0'], [/^Put Support/, 'PS'], [/^0DTE HVL/, 'HVL0'], [/^HVL/, 'HVL'],
    [/^0DTE Gamma Wall/, 'GW0'], [/^Gamma Wall/, 'GW'], [/^1D Min/, '1DL'], [/^1D Max/, '1DH'], [/^GEX ?(\d+)/, 'G$1']];
  const mqAb = z => [...new Set((z || []).map(t => { for (const [re, c] of MQAB) if (re.test(String(t))) return String(t).replace(/ [\d,.]+$/, '').replace(re, c); return ''; }).filter(Boolean))].join('+');
  const hmMin = hm => { const q = String(hm || '').match(/(\d{1,2}):(\d{2})/); return q ? +q[1] * 60 + +q[2] : NaN; };
  const t12 = t => { if (!/^\d\d:\d\d$/.test(t || '')) return t || ''; let [hh, mm] = t.split(':').map(Number); const ap = hh >= 12 ? 'PM' : 'AM'; hh = hh % 12 || 12; return `${hh}:${String(mm).padStart(2, '0')} ${ap}`; };
  function KN(k) { return String(k == null ? '' : k).replace(/\bLH\b/g, 'LonHI').replace(/\bLL\b/g, 'LonLO'); }   // (Rassul: "LonHI and LonLO so i dont confuse it with a lower low")
  function todayPlanHtf() {
    const B = (HB_ && HB_.markets) || {}, ms = ['ES', 'NQ', 'CL', 'NG', 'GC', 'HG', 'EU'].filter(m => S.markets[m]), none = [], NEAR_EM = 0.25, TAG_EM = 0.06;
    const nowMin = hmMin(S.now_ct), win = nowMin >= 1020 || nowMin <= 660;
    const CHS = { UPTHRUST: 'background:#7f1d1d;color:#fecaca;border-color:#b91c1c', SPRING: 'background:#14532d;color:#bbf7d0;border-color:#15803d' };
    let rows = '';
    for (const m of ms) {
      const x = S.markets[m] || {}, H = (B[m] || {}).htf, px = x.price_live ?? x.price, em = x.em;
      if (!H || H.stale || px == null || !em) { none.push(m); continue; }
      const lad = (x.ladder || []).filter(l => l.price != null);
      const line = (nm, buy) => {
        const k = nm.toLowerCase(), v = H[k];
        if (v == null || !H[k + 'ok']) return '';
        const d = Math.abs(px - v) / em, beyond = buy ? (H.dir < 0 && H.ext != null && H.ext < v) : (H.dir > 0 && H.ext != null && H.ext > v);
        const chip = beyond ? (buy ? 'SPRING' : 'UPTHRUST') : d <= NEAR_EM ? 'NEAR' : '';
        if (!chip) return '';
        const fm = (H.fmt || {})[k] || String(v);
        const tags = [...new Set(lad.filter(l => Math.abs(l.price - v) <= TAG_EM * em).map(l => l.mq_only ? mqAb(l.mq_zone) : KN(String(l.code || '').replace(/ · /g, '/')).replace(/\bAH\b/g, 'AsiaHI').replace(/\bAL\b/g, 'AsiaLO') + ((l.mq_zone || []).length ? '+' + mqAb(l.mq_zone) : '')).filter(Boolean))].join(' · ');
        const far = lad.filter(l => buy ? l.price >= v + NEAR_EM * em : l.price <= v - NEAR_EM * em).sort((a, b) => buy ? a.price - b.price : b.price - a.price)[0];
        const tip = QA(`Where can I ${buy ? 'buy' : 'sell'} ${m} today?`,
          `${nm} ${fm} = the 3-hour ${buy ? 'support' : 'resistance'} (${nm[1] === '1' ? 'the last 3-hour pivot ' + (buy ? 'low' : 'high') : 'the ' + (buy ? 'lower' : 'higher') + ' of the last two 3-hour pivot ' + (buy ? 'lows' : 'highs')})${tags ? ', with ' + tags + ' on it' : ''}. Price is ${d.toFixed(2)} EM away. ${chip === 'NEAR' ? 'NEAR = within 0.25 expected move.' : chip + ' = the 3-hour swing has gone ' + (buy ? 'below' : 'above') + ' it and no 3-hour bar has closed through it.'}`,
          `${chip === 'NEAR' ? 'Get ready' : 'Now watch'} for your CoB ${buy ? 'up from below' : 'down from above'} ${fm}: a 3-minute wave that closes back ${buy ? 'above' : 'below'} it, count 10+. The Cob window is 5:00 PM - 11:00 AM CT. T1 = the next level at least 0.25 EM away${far ? ' (' + far.price_fmt + ')' : ''}; after the CoB prints use its 1.33 extension.`,
          'ES R1 7,649.75 AsiaHI · LonHI UPTHRUST -> the 7:33 AM high 7,669.25 went above R1, the CoB down closed back below it.');
        return `<div style="display:flex;align-items:center;gap:6px;white-space:nowrap" title="${esc(tip)}"><b class="tpl" style="color:${buy ? '#4ade80' : '#f87171'}">${esc(nm)} ${esc(fm)}</b> <span class="tpm">${esc(tags)}</span> <span class="chip ${chip === 'NEAR' ? 'TESTING' : ''}" style="${CHS[chip] || ''}">${chip}</span><span class="ptg" style="margin-left:auto">${far ? `T1 <b>${esc(far.price_fmt)}</b>` : ''}</span></div>`;
      };
      const same = (a, b) => a != null && b != null && Math.abs(a - b) <= Math.abs(a) * 1e-9;
      const b = line('S1', true) + (same(H.s1, H.s2) ? '' : line('S2', true)), s_ = line('R1', false) + (same(H.r1, H.r2) ? '' : line('R2', false));
      if (!b && !s_) { none.push(m); continue; }
      const dash = '<span class="ev">-</span>';
      rows += `<tr class="rw"><td class="lmk tpc"><b class="tpk">${m === 'EU' ? '6E' : m}</b></td><td class="lkl tpc">${b || dash}</td><td class="lkl tpc">${s_ || dash}</td></tr>`;
    }
    const tip = QA('Which levels should I watch today?', 'For each market the 3-hour support (S1, S2) to BUY and resistance (R1, R2) to SELL, as your CoB Signal indicator draws them - only when the level is NEAR (within 0.25 expected move) or in a SPRING / UPTHRUST position (the 3-hour swing is beyond a level that no 3-hour bar has closed through). The blue tags are the named key levels and MenthorQ levels sitting on it.', 'The trade is your CoB off that level: a 3-minute wave from the swing extreme that closes back through the level, count 10+, at least 1.9x the last 20 waves, between 5:00 PM and 11:00 AM CT.', 'ES R1 7,649.75 AsiaHI · LonHI UPTHRUST · T1 7,601.50.');
    return `<div class="sec tpsec"><div class="h" title="${esc(tip)}">Today plan · updates live · ${esc(t12(S.now_ct || ''))} CT · Cob window 5:00 PM – 11:00 AM: ${win ? 'OPEN' : 'CLOSED'} <span class="ev">ⓘ</span></div>` +
      `<table class="t tpt" style="width:1000px;table-layout:fixed"><colgroup><col style="width:48px"><col style="width:476px"><col style="width:476px"></colgroup><tr><th>Mkt</th><th style="color:#4ade80;background:rgba(34,197,94,.12)">Buy at (3-hour support)</th><th style="color:#f87171;background:rgba(239,68,68,.12)">Sell at (3-hour resistance)</th></tr>` +
      `${rows || '<tr><td colspan="3" class="ev">nothing setting up right now - no 3-hour level NEAR or in a SPRING / UPTHRUST position</td></tr>'}</table>${none.length && rows ? `<div class="ev" style="margin-top:4px">Nothing near or setting up: ${none.map(m => m === 'EU' ? '6E' : m).join(' · ')}</div>` : ''}</div>`;
  }
