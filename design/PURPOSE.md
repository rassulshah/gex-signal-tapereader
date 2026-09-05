# PURPOSE — what this application is FOR

**Operator, 2026-09-02, in his own words.** Quoted, not paraphrased, because every design argument in
this project should be settled against it:

> "The purpose of the application is to be able to identify two key turning points which are the
> high of the day and low of day, in order to profit from the move from high to low or low to high.
> As a side objective, it is also to identify pullback turning points (aka reversals or
> deflections). In order to do this I am relying on gamma levels. What we see repeatedly is a node
> with gamma that causes the deflection creating a reversal — either the trend reversal from the
> high of day or low of day, or a pullback reversal that stops the pullback leading the market into
> trend continuation.
>
> The approach that I am taking is twofold. First, create statistics that measure the day including
> the turning points and more — this is what is identified in the HOD/LOD section. Second, the node
> ladder, which tracks the king movements, which has the ability to attract price as well as deflect
> it. On the other side of the ladder are the roll arrows and the delta along with ROC and
> qualifiers. The purpose of this is to track gamma movement so that when gamma is building on a
> pullback, it may be a pullback node that causes the reversal and trend continuation. Furthermore
> the gamma node may come at a high or low causing a HOD or LOD deflection."

> ⚠ **READ THIS BEFORE PROPOSING, PRIORITISING OR CUTTING ANYTHING.** It is registered in
> `skills/gex/SKILL.md` and pinned by `test_purpose.js`, so a context that skips it fails the suite.

---

## 1 · THE ONE SENTENCE

**Find the day's turning points — the HOD and the LOD — early enough to trade the move between them.
Secondarily, find the pullback turning points that resume a trend rather than end it.**

Everything else in this panel is instrumentation for that. A feature that does not help locate a
turning point, or help judge whether one is forming, is decoration.

## 2 · THE MECHANISM HE IS TRADING

**A gamma node deflects price, and the deflection is the turning point.** That is the causal claim
the whole panel rests on. Two kinds:

    TREND REVERSAL     a node at the extreme turns the day     -> the HOD or the LOD
    PULLBACK REVERSAL  a node stops a counter-move             -> trend CONTINUATION, not an end

⚠ **THE SECOND ONE IS EASY TO MISREAD AS THE FIRST, AND CONFUSING THEM IS THE EXPENSIVE ERROR** —
the two call for opposite trades: a pullback deflection means *stay in*, a HOD/LOD deflection means
*turn around*. Any read the
panel emits about a deflection should be explicit about which it thinks it is seeing.

## 3 · THE TWO HALVES, AND WHAT EACH IS FOR

### A · The ⓪a HOD/LOD section — **measure the day**
Statistics about the turning points and the shape of the session: which extreme printed first, how
long it took, how long it has stood, the wick family, the gap between extremes, the MUD leg, the
range and what it was worth. **A over E** — every live figure against its own base rate, so "is this
turn unusual" is answerable rather than felt.

### B · The node ladder — **watch the gamma that causes the turn**
The king and its migrations: a node **attracts** price as well as **deflects** it. The lane shows
where the crown has sat and when it moved.

### C · The right-hand columns — **track gamma MOVEMENT**, in his stated order
> *"the arrow column shows the movement of gamma rolling from one strike to another, the delta
> profile shows how much gamma is moving, the state says it in words by classifying it, the roc
> gives you a percentage."*

    ⇄ arrows   WHERE gamma is rolling, strike to strike
    Δ15m       HOW MUCH is moving
    STATE      WHAT IT MEANS, classified in words
    ROC 15m    AS A RATE

⚠ **This is one narrative and the order is his.** Do not reorder these without asking him.

**Why it exists:** *gamma BUILDING on a pullback may be the node that causes the reversal and the
trend continuation; gamma building at a high or a low may be the node that causes the HOD or LOD
deflection.* So the question the columns answer is **"is a deflection being built right now, and
where?"** — which is the leading indicator for the turning points §1 is about.

## 3b · THE STANDING REQUIREMENT ON HOW IT IS BUILT (operator, 2026-09-04, his words)

> "the idea is to have a trading decision support system that is data driven. I want to have top quality insight
> and decision support. In order to do that we are building out data capture, analysis testing back to dashboard
> for everything that is displayed on the dashboard including hod lod time, nodes, setups, directional prediction,
> reads and more."

And, the same evening: *"i envision clicking on the save, the data getting saved and the analysis occurring and
the analysis tab being updated."* The consequence, stated once so no later context re-derives it: **every element
the dashboard shows must have a chain — captured in the record → counted by the nightly (Analysis) → tested out of
sample by the register (Testing) → approved as a rule → rendered with its n → the read recorded and scored the next
day — and one definition travels that whole chain.** An element without a chain is DESCRIPTIVE and must say so;
it may not imply a claim. His end of day was one click (the 💾) until v15.71 (*"the next step is to automatically have
the application trigger the save button instead of me clicking it … if the save button has not been pressed and the
time is [after market hours], trigger it. if the save button for the previous day has not been triggered and the time
is during non market hours, trigger it."* — the same day); now the panel writes the day itself after the close and any
missed day outside market hours, the 💾 is the override, and everything after the file is the machine's, except the
review, which is a session's (and says so). The build order follows from this: the outcomes the counts are scored
against (turn vs stay in), the read recorded and scored, the count → test draft, recommendations with his tick,
the live pre-tap read joined to the tested number. **And the Learn tab is part of the same chain** (his addition,
the same evening: "in this process, the learn tab should also be updated"): the rules L-n carry the nightly's
numbers for the class each rule names; the gauge's predict part is the scored reads; the day's strongest taps are
offered as candidate examples for him to teach; the taught examples are re-checked against the record under the
current definitions — the blind calls and the teaching stay his and mine. And the sharpest form of it, his words the same evening: *"when you think about it, the entire data, analysis
and testing process results in learning and it is from the learning that can know something and make a decision
based on what you see on the dashboard."* So the chain ends at KNOWLEDGE, not at the dashboard: data → analysis
(counts) → testing (trials) → learning (what survived the trial, with its evidence) → the decision at the
dashboard, which draws from the knowledge and nothing else. Knowledge has degrees, and the face shows which one a
number carries: confirmed (tested out of sample) · provisional (counted, untested) · doctrine (Skylit's word,
unmeasured here) · descriptive (a fact about now, no claim). A data-driven decision-support system is exactly
this chain, closed. **Its name, his choice: the Data Analysis process — `design/DATA-ANALYSIS-PROCESS.md`** ("lets
call the process Data Analysis process to keep it simple"); the Rec tab is where what to implement is discussed. `design/PROCESS.md` is the loop; `design/ARCHITECTURE.md` the machinery;
`design/DASHBOARD-INVENTORY.md` the element-by-element ledger of what has a chain and what is still descriptive.

## 4 · WHAT THIS MEANS FOR EVERY FUTURE DECISION

1. **A feature earns its place by helping call a turning point.** If it cannot be traced to §1 or
   §2, it is decoration — and this panel has already cut a gamma profile, a TAPS column, four badges
   and a ROLL words column for exactly that reason.
2. **Building gamma is the leading signal; the deflection is the event; the HOD/LOD statistic is the
   scorecard.** Those are three different time horizons and a surface should be clear which it is on.
3. **Distinguish the two reversal kinds, always.** See §2.
4. ⚠ **The mechanism is a HYPOTHESIS HE IS TRADING, not a proven law.** "What we see repeatedly" is
   his observation. The panel should keep making it *checkable* — base rates, hit rates, the A-over-E
   table — rather than asserting it. A surface that assumes the claim and cannot be scored against it
   is the thing to be suspicious of.
5. **Recording is not a side feature.** Every base rate in §3A and every replay of a forming
   turn depends on the recorder having captured the session. On 2026-09-02 a replay left parked on
   the previous day cost four hours of capture — the fix (v15.45) is a purpose-level fix, not a UI one.

## 5 · WHAT THIS FILE IS NOT

It is not a roadmap (`roadmap/PRODUCT-ROADMAP.md`), not a decision log (`session-state/DECISIONS.md`)
and not a dependency list (`design/DEPENDENCIES.md`). It is the standard those three are judged
against. If one of them conflicts with this file, **this file wins and the other one is wrong.**
