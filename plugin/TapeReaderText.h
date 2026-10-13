// TapeReaderText.h -- lsTapeReader 1.0.1 (2026-10-13; one change from 1.0.0: the refill letter R -> I): EVERY word the Tape Reader box writes, in ONE table, so the phrasing can be
// swapped without touching the logic (coordinator 11:58: "keep the text templates in ONE table-driven place").
// Phrasing follows "Tape Reader Language Guide (MBO, 3-min, reversals at key levels)", research_notes/TapeReader language/
// LANGUAGE-GUIDE.md, compiled 2026-10-13 (the version used for 1.0.0):
//   * order book = intent, tape = fact: resting size is never called support / resistance ("bids stacking ... untested");
//   * aggression is paired with its result ("hitting bid 4.1x - bid held");
//   * pulling is read relative to where price is going ("as price came in" vs "away from price");
//   * absorption / exhaustion need confirmation -> the bias side gets a "Needs ..." phrase; mixed reads say "Mixed - wait";
//   * a sweep names its outcome (no follow-through = trapped, accepted = initiative); intent is never asserted;
//   * icebergs as numbers; no hype words; reads in the first / last 5 min of RTH or on a roll are flagged "Low trust".
// Letters: S = SWEEP (one aggressor order through >= N price levels), L = LARGE single order (aggressive in BidAskStats, pulled in
// PullStats, added in StackStats), I = REFILL / ICEBERG (StackStats; R until 1.0.0, now R = stop run). Abbreviations: lvls, LQ, +GEX / -GEX, dlrs, Inval, Tgt, LOD/HOD,
// proj, A / AE? / Ex?. Ratios as 4.4x. Placeholders are {name}; an unknown placeholder is left empty.
#ifndef LS_TAPEREADER_TEXT_H
#define LS_TAPEREADER_TEXT_H
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace trt {

struct Tpl { const char* key; const char* text; };

// ---- THE TABLE ------------------------------------------------------------------------------------------------------------
static const Tpl TABLE[] = {
    // box frame
    { "header",          "Tape Reader v{ver}  \xB7  {mkt}  \xB7  {time}" },
    { "bias.bull",       "BULLISH {n} of 4" },
    { "bias.bear",       "BEARISH {n} of 4" },
    { "bias.mixed",      "MIXED" },
    { "label.buyers",    "BUYERS" },
    { "label.sellers",   "SELLERS" },
    { "label.levels",    "LEVELS" },
    { "quiet",           "quiet" },
    // line 1, centre (three spaces between the parts, as the approved mockup)
    { "turn",            "Turn {dir} at {lvl} {p}" },
    { "turn.learning",   "Turn odds learning" },
    { "swing",           "Swing {x}+ ticks {p}" },
    { "next",            "Next: {px} {lvl}" },
    { "centre.sep",      "   " },
    // BidAskStats (tape = fact): aggression always paired with its result
    { "bas.sweep.held",  "Swept {lv} lvls{lot}{into} - no follow-through, {book} held" },
    { "bas.sweep.gave",  "Swept {lv} lvls{lot}{into} - accepted, {book} giving way" },
    { "bas.hit.held",    "{verb} {x}{lot}{into} - {book} held" },
    { "bas.hit.gave",    "{verb} {x}{lot}{into} - {book} giving way" },
    { "verb.hitbid",     "Hitting bid" },
    { "verb.liftoffer",  "Lifting offer" },
    { "lot",             " ({n}-lot)" },
    { "into",            " into {lvl}" },
    { "at",              " at {lvl}" },
    // PullStats (book = intent): relative to where price is going, never "spoof"
    { "pull.into",       "Pulled {book} {x}{lot} as price came in{at}" },
    { "pull.away",       "Pulled {book} {x}{lot} away from price{at}" },
    { "pull.bids.into",  " - support leaving" },
    { "pull.offers.into"," - path opening up" },
    { "pull.away.tail",  " - backing off" },
    // StackStats: stacking is untested intent, refills / icebergs are numbers
    { "stack.add",       "{Book} stacking {x}{lot}{at} - untested" },
    { "stack.refill",    "{Book} reloading (I {rx}){at}{ice}" },
    { "stack.both",      "{Book} stacking {x}{lot} + reloading (I {rx}){at}{ice}" },
    { "ice",             ", iceberg" },
    { "ice.n",           ", iceberg {shown} shown / {traded} traded" },
    // Delta Profile (A / AE? / Ex?): absorption and exhaustion as facts that need confirmation
    { "delta.a",         "Absorbing at {lvl}{px} (A {x}x{ae})" },
    { "delta.ae.tag",    ", {ae}" },
    { "delta.ae.att",    "Faded into {lvl}{px} ({ae}) - no new size" },
    { "delta.ex",        "Last push to {px} on thin {flow} ({mark}) - exhausted" },
    { "flow.selling",    "selling" },
    { "flow.buying",     "buying" },
    // confirmation (the bias side), mixed
    { "need.bull",       "Needs a higher low / lift the offer (S)" },
    { "need.bear",       "Needs a lower high / hit the bid (S)" },
    // LEVELS line
    { "liq",             "{side} LQ {bx}x{on}" },
    { "liq.on",          " on {lvl}" },
    { "liq.bid",         "Bid" },
    { "liq.offer",       "Offer" },
    { "gex.pos.below",   "+GEX {lvl} {px} (dlrs buy dips)" },
    { "gex.pos.above",   "+GEX {lvl} {px} (dlrs sell rips)" },
    { "gex.neg",         "-GEX {lvl} {px} (dlrs chase)" },
    { "hl.in",           "{side} IN {p}%" },
    { "against",         "Against: {list}" },
    { "inval.below",     "Inval < {px}" },
    { "inval.above",     "Inval > {px}" },
    { "tgt",             "Tgt {px} {lvl}{odds}" },
    { "tgt.odds",        " ({p}%{q})" },
    { "tgt.proj",        "proj {side} {px}" },
    { "mixed.wait",      "Mixed - wait" },
    { "watch",           "Watch {up} / {dn}" },
    { "lowtrust",        "Low trust ({why})" },
    { "lowtrust.open",   "RTH open" },
    { "lowtrust.close",  "RTH close" },
    { "lowtrust.roll",   "roll" },
    // against-list items
    { "ag.flow.sells",   "flow sells" },
    { "ag.flow.buys",    "flow buys" },
    { "ag.lvl.broken",   "{lvl} broken" },
    { "ag.lvl.held",     "{lvl} held" },
    { "ag.trend.up",     "trend up" },
    { "ag.trend.dn",     "trend down" },
    { "ag.trend.sup",    "strong uptrend" },
    { "ag.trend.sdn",    "strong downtrend" },
    // the box when it cannot read
    { "note.narrow",     "Tape Reader: pane too narrow" },
    { "note.short",      "Tape Reader: pane too short" },
    { "note.nomarket",   "Tape Reader: unknown market" },
};

inline const char* text(const char* key)
{
    for (size_t i = 0; i < sizeof(TABLE) / sizeof(TABLE[0]); ++i) if (std::strcmp(TABLE[i].key, key) == 0) return TABLE[i].text;
    return "";
}
typedef std::vector<std::pair<std::string, std::string> > Args;
// the template with {name} replaced from args ({Name} = the value with its first letter upper case); unknown names -> ""
inline std::string fill(const char* key, const Args& a = Args())
{
    const std::string t = text(key);
    std::string o;
    for (size_t i = 0; i < t.size(); ++i) {
        if (t[i] != '{') { o += t[i]; continue; }
        const size_t e = t.find('}', i);
        if (e == std::string::npos) { o += t.substr(i); break; }
        std::string name = t.substr(i + 1, e - i - 1);
        bool cap = !name.empty() && name[0] >= 'A' && name[0] <= 'Z';
        if (cap) name[0] = (char)(name[0] - 'A' + 'a');
        for (size_t k = 0; k < a.size(); ++k) if (a[k].first == name) {
            std::string v = a[k].second;
            if (cap && !v.empty() && v[0] >= 'a' && v[0] <= 'z') v[0] = (char)(v[0] - 'a' + 'A');
            o += v; break;
        }
        i = e;
    }
    return o;
}
inline std::string capFirst(std::string s) { if (!s.empty() && s[0] >= 'a' && s[0] <= 'z') s[0] = (char)(s[0] - 'a' + 'A'); return s; }

} // namespace trt
#endif
