// (v1.0) SCRIPT B — the InsiderFinance chain reader.
//
// Their page embeds the FULL option chain in __NEXT_DATA__ (13,210 contracts for SPY), each carrying
// {strike, expireYear, expireMonth, expireDay, cp, gamma, delta, openInterest, impliedVol, bid, ask}.
// So we recompute from source rather than scraping their rendered header — their CSS, layout and wording
// can all change and this keeps working.
//
// VERIFIED LIVE 2026-08-20 against their own published figures, spot 761.14 — every value exact:
//   All expirations : Call 17.8B · Put -32.2B · Net -14.4B · ratio 0.55 · CR 800 · PS 760 · Mag 760
//   through Friday  : ratio 0.40 · CR 775 · PS 760 · Mag 760
//   0DTE            : ratio 0.02 · PS 760 · Mag 760 · CR suppressed (0.05B call -> they print N/A)
// The formula is GEX = gamma * OI * 100 * spot^2 * 0.01, puts negative.
const fs=require('fs'); const src=fs.readFileSync('./current/gex-if-levels.user.js','utf8');
let pass=0, fail=0; const ok=(c,m,g)=>{ if(c){pass++;console.log('PASS '+m);} else {fail++;console.log('FAIL '+m+(g!==undefined?' -> '+JSON.stringify(g):''));} };
function ex(n){const re=new RegExp('function\\s+'+n+'\\s*\\(','g');const m=re.exec(src);let i=src.indexOf('{',m.index),d=0,e=-1;for(let k=i;k<src.length;k++){if(src[k]==='{')d++;else if(src[k]==='}'){d--;if(d===0){e=k;break;}}}return src.slice(m.index,e+1);}
eval(ex('ymdNum')); eval(ex('windows')); eval(ex('extractChain'));
global.SIDE_MIN=0.05;
// (v11.53) dexSkewFor, expectedMove and gammaFlip are CALLED BY computeAll and exist in the
// companion — ex() extracts one body, so every callee must be evalled too or the whole block
// throws ReferenceError and the file scores zero. hdrText/hdrNum are the v1.9 header scraper.
eval(['ymdOf','ymdNum','windows','levelsFor','dexSkewFor','expectedMove','gammaFlip',
      'hdrText','hdrNum','extractChain','computeAll'].map(ex).join('\n'));

// ---------- metadata: the grant split is the whole point ----------
{
  ok(/@grant\s+GM_xmlhttpRequest/.test(src),'this script takes the grant');
  ok(/@connect\s+insiderfinance\.io/.test(src),'and declares the host it reaches');
  ok(/@match\s+https:\/\/app\.skylit\.ai\/atlas/.test(src),'it runs on the SAME page as the Tapereader, so they share an origin for localStorage');
  // ⚠ STRIP COMMENTS BEFORE GREPPING. FIFTH occurrence of the documented sub-trap: v1.15's Yahoo
  // courier quotes item 18's own hedge ("verify unsafeWindow access still OK") in a comment
  // explaining why the tap lives HERE and not in the panel — and this assertion went red over a
  // word in prose. A grep over comments tests the documentation, not the code.
  const code = src.replace(/\/\*[\s\S]*?\*\//g,' ').replace(/^\s*\/\/.*$/gm,' ');
  ok(!/unsafeWindow/.test(code),'it never touches the page window — it has no tape to break');
}
// ---------- extractChain ----------
{
  const good='<html><script id="__NEXT_DATA__" type="application/json">'+
    JSON.stringify({props:{pageProps:{initialData:{ticker:'SPY',spot:761.14,timestamp:'2026-08-20T20:25:34Z',isStale:false,
      options:[{strike:760,expireYear:2026,expireMonth:8,expireDay:20,cp:'P',gamma:0.01,openInterest:100}]}}}})+'</script></html>';
  const c=extractChain(good);
  ok(!c.err && c.spot===761.14,'a well-formed page yields the chain',c.err);
  ok(c.options.length===1,'with its options');
  ok(extractChain('<html>nothing</html>').err.indexOf('no __NEXT_DATA__')===0,'a page without the block says so plainly');
  ok(/initialData missing/.test(extractChain('<script id="__NEXT_DATA__">{"props":{"pageProps":{}}}</script>').err),'block present but empty is a DIFFERENT error');
  ok(/no options/.test(extractChain('<script id="__NEXT_DATA__">{"props":{"pageProps":{"initialData":{"spot":1}}}}</script>').err),'and so is a missing chain');
  ok(/no spot/.test(extractChain('<script id="__NEXT_DATA__">{"props":{"pageProps":{"initialData":{"options":[1]}}}}</script>').err),'and a missing spot');
  ok(/parse failed/.test(extractChain('<script id="__NEXT_DATA__">{not json</script>').err),'malformed JSON is caught, not thrown');
}
// ---------- windows: the user's definition, NOT theirs ----------
{
  // Thursday 2026-08-20 -> Friday is the 21st, so the window is 2 days
  const W=windows('2026-08-20T20:25:34Z');
  ok(W.today===20260820,'today comes from THEIR payload timestamp, not the browser clock',W.today);
  ok(W.friday===20260821,'and Friday of the CURRENT week',W.friday);
  // Monday -> Friday is 4 days out
  ok(windows('2026-08-17T14:00:00Z').friday===20260821,'from a Monday, Friday is still the 21st',windows('2026-08-17T14:00:00Z').friday);
  // ON a Friday the window collapses to that day — the known degeneracy, recorded not hidden
  const F=windows('2026-08-21T14:00:00Z');
  // (v1.1) this used to assert friday===today, which WAS the collapse: the through-Friday window
  // became a copy of the 0DTE window and CR equalled CR0. It now rolls forward instead.
  ok(F.today===20260821 && F.friday===20260828 && F.rolled===true,'on a Friday the window rolls forward rather than collapsing onto 0DTE',F);
  // a late-UTC timestamp must not roll the trading date forward
  ok(windows('2026-08-20T23:30:00Z').today===20260820,'a 23:30 UTC stamp is still the 20th in US hours',windows('2026-08-20T23:30:00Z').today);
}
// ---------- levelsFor: the verified rules ----------
{
  const S=761.14;
  const o=(k,cp,g,oi,y,m,d)=>({strike:k,cp:cp,gamma:g,openInterest:oi,expireYear:y||2026,expireMonth:m||8,expireDay:d||20});
  // build a book where the answers are known by construction
  const chain=[
    o(755,'P',0.02,5000), o(760,'P',0.03,9000), o(765,'P',0.02,3000),
    o(770,'C',0.02,1000), o(775,'C',0.03,6000), o(780,'C',0.01,500)
  ];
  const L=levelsFor(chain,S,()=>true);
  ok(L.ps===760,'PUT WALL = most negative net BELOW spot',L.ps);
  ok(L.cr===775,'CALL WALL = most positive net ABOVE spot',L.cr);
  ok(L.mag===760,'MAGNET = largest |net| anywhere',L.mag);
  ok(L.callGEX>0 && L.putGEX<0,'calls positive, puts negative',[L.callGEX,L.putGEX]);
  ok(L.strikes===6 && L.contracts===6,'counts reported',[L.strikes,L.contracts]);
  ok(L.pcOI!=null,'put/call OI ratio is computed — impossible from the Skylit feed, free here',L.pcOI);
  ok(L.maxPain!=null,'and MAX PAIN, same reason',L.maxPain);
}
{
  // the N/A case: a side holding almost none of the book names no wall, exactly as their page does
  const S=761.14;
  const o=(k,cp,g,oi)=>({strike:k,cp:cp,gamma:g,openInterest:oi,expireYear:2026,expireMonth:8,expireDay:20});
  const L=levelsFor([o(755,'P',0.03,9000), o(760,'P',0.03,9000), o(770,'C',0.0001,5)],S,()=>true);
  ok(L.cr===null,'a negligible call side names NO call wall',L.cr);
  ok(L.crSuppressed && L.crSuppressed.k===770,'the strike it would have named is kept for the explanation',L.crSuppressed);
  ok(L.crSuppressed.share<5,'with the share that disqualified it',L.crSuppressed.share);
  ok(L.ps===755||L.ps===760,'the side that DOES hold gamma keeps its wall',L.ps);
}
{
  ok(levelsFor([],761,()=>true)===null,'no contracts, no levels');
  ok(levelsFor([{strike:760,cp:'P'}],761,()=>true)===null,'rows without gamma/OI are skipped, and an all-bad set yields null');
}
// ---------- computeAll: three windows off one payload ----------
{
  const o=(k,cp,g,oi,d)=>({strike:k,cp:cp,gamma:g,openInterest:oi,expireYear:2026,expireMonth:8,expireDay:d});
  const ch={ spot:761.14, ticker:'SPY', t:'2026-08-20T20:25:34Z', stale:false, options:[
    // today (0DTE): put-dominated, negligible calls -> CR must be suppressed like theirs
    o(760,'P',0.03,9000,20), o(755,'P',0.02,4000,20), o(770,'C',0.0001,3,20),
    // Friday: adds a real call side
    o(775,'C',0.03,6000,21), o(760,'P',0.02,3000,21),
    // next week: OUTSIDE the through-Friday window, must not leak in
    o(800,'C',0.05,90000,28)
  ]};
  const R=computeAll(ch);
  ok(R.dte0.exps[0]===20260820,'0DTE is today',R.dte0.exps);
  ok(R.toFri.exps.length===2,'through-Friday spans today AND Friday',R.toFri.exps);
  ok(R.toFri.exps.indexOf(20260828)<0,'and EXCLUDES next week — that is the user definition, not a rolling 7 days',R.toFri.exps);
  ok(R.dte0.lv.cr===null,'0DTE names no call wall on an all-put book',R.dte0.lv.cr);
  ok(R.toFri.lv.cr===775,'through-Friday finds one once Friday calls are included',R.toFri.lv.cr);
  ok(R.all.lv.cr===800,'and All reaches the far strike the near windows correctly ignored',R.all.lv.cr);
  ok(R.all.nExps===3,'All reports how many expirations it covered',R.all.nExps);
  ok(R.spot===761.14 && R.ticker==='SPY','spot and ticker ride along');
  ok(typeof R.asOf==='number','and a capture time, so staleness is checkable');
}

// ---- (v1.1) THE COMPANION CARRIED THE SAME FRIDAY COLLAPSE -------------------------------
// Seen live on 2026-08-21: the stored payload reported friday:20260821 and today:20260821, so its
// "through Friday" window was a second copy of the 0DTE window and both level sets printed the
// same numbers — exactly the bug the Tapereader had in exp_mode=week.
{
  const ts=(iso)=>Date.parse(iso+'T18:00:00Z');     // midday ET, so the -4h shift stays on the same date
  const W=windows(ts('2026-08-21'));                // a Friday
  ok(W.today===20260821,'the payload date is read as the trading day',W.today);
  ok(W.friday===20260828,'on a FRIDAY the through-Friday window rolls to NEXT Friday',W.friday);
  ok(W.rolled===true,'and the roll is flagged so the panel can disclose it');
  ok(W.friday!==W.today,'the second window is genuinely a second window');
}
{
  const ts=(iso)=>Date.parse(iso+'T18:00:00Z');
  const W=windows(ts('2026-08-19'));                // a Wednesday
  ok(W.friday===20260821,'midweek the window still runs to THIS Friday',W.friday);
  ok(!W.rolled,'and nothing is rolled');
}
{
  const ts=(iso)=>Date.parse(iso+'T18:00:00Z');
  const W=windows(ts('2026-08-24'));                // a Monday
  ok(W.friday===20260828,'Monday runs to that same week Friday',W.friday);
  ok(!W.rolled,'no roll on a Monday');
}

// ---- (v1.2) THEIR PUBLISHED NUMBERS ARE TAKEN, NOT RECOMPUTED ----------------------------
// Their page header prints Zero Gamma, Call Wall and Put Wall (SPY 766.48 / 800 / 760 on
// 2026-08-21). v11.29 computed its own gamma flip instead and was one step from labelling it IF.
{
  const mk=(d)=>'<script id="__NEXT_DATA__" type="application/json">'+JSON.stringify(
    {props:{pageProps:{initialData:Object.assign({spot:765.2,options:[{strike:765,cp:'C',gamma:0.01,openInterest:10,expireYear:2026,expireMonth:8,expireDay:21}]},d)}}})+'</script>';
  let ch=extractChain(mk({zeroGamma:766.48, callWall:800, putWall:760}));
  ok(!ch.err,'a payload with published levels parses',ch.err);
  ok(ch.pub.zeroGamma===766.48,'their zero gamma is taken verbatim',ch.pub);
  ok(ch.pub.callWall===800 && ch.pub.putWall===760,'so are their walls');

  // (v1.10) This fixture used to pair spot 765.2 with zeroGammaLevel 7679.88 — an SPX value beside a
  // SPY spot, which is the very scale-mixing this project keeps getting bitten by. The new sanity gate
  // rejects it, correctly. Renamed keys are now tested with a value that is on the SAME SCALE as spot.
  ch=extractChain(mk({zeroGammaLevel:767.99, callWallPrice:770}));
  ok(ch.pub.zeroGamma===767.99,'a renamed zero-gamma key is found by pattern rather than lost',ch.pub);
  ok(ch.pub.callWall===770,'and so is a renamed call wall');

  // ---- (v1.10) THE SANITY GATE ----
  // A truncation bug in v1.9 put Zero Gamma on the face as 764 beside a spot of 7674, under THEIR name.
  // The regex was the proximate cause; the defect was that nothing checked the value against spot.
  ch=extractChain(mk({zeroGamma:764}));                       // spot is 765.2 -> 764 is plausible here
  ok(ch.pub.zeroGamma===764,'a level near spot is accepted',ch.pub.zeroGamma);
  ch=extractChain(mk({zeroGamma:76.4}));                      // an order of magnitude low
  ok(ch.pub.zeroGamma===null,'a level 10x BELOW spot is rejected, not shown under their name',ch.pub.zeroGamma);
  ok(/^REJECTED:/.test(String(ch.pubSrc.zeroGamma)),'and the rejection is RECORDED so it can be diagnosed',ch.pubSrc.zeroGamma);
  ch=extractChain(mk({callWall:7900}));                       // an order of magnitude high
  ok(ch.pub.callWall===null,'a level 10x ABOVE spot is rejected too',ch.pub.callWall);
  ch=extractChain(mk({zeroGamma:766.48, atmIV:6.2, pcRatio:1.36}));
  ok(ch.pub.atmIV===6.2 && ch.pub.pcRatio===1.36,'ratios and IV are NOT gated — they are not prices',[ch.pub.atmIV,ch.pub.pcRatio]);

  ch=extractChain(mk({}));
  ok(ch.pub.zeroGamma===null && ch.pub.callWall===null,'when they publish nothing the fields are null — never invented',ch.pub);

  ch=extractChain(mk({zeroGamma:'n/a'}));
  ok(ch.pub.zeroGamma===null,'a non-numeric zero gamma is treated as absent');
}

console.log('\n'+pass+' passed, '+fail+' failed');
process.exit(fail?1:0);
