#ifndef LS_CHAINSPEC_H
#define LS_CHAINSPEC_H
// ChainSpec.h 1.1.0 (2026-10-28 overnight, Rassul 21:52 / 21:57 CT): the NODE LADDER (kind Z, lsBookFlow -> the READ tiers A / A? / A+),
// the node defender read (kind Y, the READ -> lsBookFlow's At node row + mark = a DISPLAY stage of that plugin at layer 25, so the graph
// keeps one direction: lsBookFlow (10) -> READ (20) -> lsBookFlow.AtNode (25), nothing fed back); B = depth levels + big-order lines (LP 0.62.1)
// ChainSpec.h 1.0.0 (2026-10-28, Rassul 20:11-20:18 CT "all the indicators communicating ... be careful not to create any infinite or
// redundant loops ... the right architecture so each indicator has what it needs as input and outputs what others can use") -- the STATIC
// dependency graph of the price chart's indicators: who produces which fact (one owner each) and who reads it, by LAYER:
//   L0  raw data (chart bars, volume at price, MBO / depth)                                                       layer 0  (IRT)
//   L1a facts from raw data: resting orders + LMT F prints, delta nodes + candle trap lots, defending, trapped    layer 10
//   L1b facts built on L1a:  the level list with odds + the liquidity top-10 rank (lsDealerProfile)              layer 15
//   L2  signals: READ codes / A, Ex, BO                                                                          layer 20
//   L3  the combiner: one read with odds                                                                         layer 30
// (layer 1 is split in two so lsDealerProfile, which ranks lsLiquidityProfile's resting orders, sits ABOVE it and never beside it)
// RULES (checked by chain::check, run by tests_chain/test_chain.cpp and by the nightly QUALITY CHECK through chain_graph.json):
//   1. a reader only reads facts of a LOWER layer, or of its OWN layer as DECIDED facts (state 1) when it decides with them, or LIVE facts
//      for display placement only (Use DISPLAY: never recorded, never decides anything) - NEVER a higher layer;
//   2. every fact kind has exactly ONE producer (the ATR, the level list and the liquidity rank each have one owner);
//   3. the read graph has no cycle;
//   4. every input an indicator needs has a producer.
// SDK-free.
#include <string>
#include <vector>
#include <map>
#include <set>
namespace chain {
static const char* const VERSION = "1.4.0";   // (1.4.0, 2026-10-28) + kind S (the touch tags: lsLiquidityProfile 0.66.0 decides STAYS+ / STAYS / GONE / GONE+ and HELD / BROKE from the MBO feed, layer 10), read by lsDealerProfile 2.32.0 for DISPLAY (the tag in the Liquidity column, HELD on the chart); the votes ride on the S facts (val) - no combiner / Tape Reader reads them yet (that edge is added when one does)   // (1.3.0, 2026-10-28) + kind H (the hourly flow at each tiered A + the current flow) from lsDeltaProfile.READ, read by the Banner (display); the nightly joins the T-line flow to the scored combiner reads (not a live combiner input yet)   // (1.2.0) + lsLiquidityProfile.Banner (layer 25, display: R / Y / T)
enum Use { DECIDE = 0, DISPLAY = 1 };
struct Node { std::string name; int layer; };
struct Edge { std::string from, to; char kind; Use use; };   // 'from' produces kind, 'to' reads it
struct Producer { char kind; std::string owner; int layer; const char* what; };
inline std::vector<Node> nodes() {
    return { {"IRT", 0}, {"lsLiquidityProfile", 10}, {"lsDeltaProfile", 10}, {"lsBookFlow", 10}, {"lsTrappedTraders", 10}, {"lsDealerProfile", 15},
             {"lsDeltaProfile.READ", 20}, {"lsLiquidityProfile.Ex", 20}, {"lsBigBidAsk", 20}, {"lsBookFlow.AtNode", 25}, {"lsLiquidityProfile.Banner", 25}, {"lsDeltaProfile.Combiner", 30} };
}
inline std::vector<Producer> producers() {
    return { {'B', "lsLiquidityProfile", 10, "resting liquidity within reach: the depth book's lots per price + the big-order lines (0.62.1)"},
             {'Z', "lsBookFlow", 10, "the node ladder: added / pulled / filled lots per price per closed candle (SignalBoard 1.5.0 LRA_SLD_)"},
             {'Y', "lsDeltaProfile.READ", 20, "the node defender read of each tiered A (holding / pulling, net x)"},
             {'P', "lsLiquidityProfile", 10, "LMT F prints (biggest single resting order filled, S20 / S10 / S5)"},
             {'S', "lsLiquidityProfile", 10, "touch tags of big resting levels: 15-s STAYS+ / STAYS / weak / GONE / GONE+ with the signed vote, then HELD / BROKE / EXPIRED (LiqTouch.h 1.0.0)"},
             {'N', "lsDeltaProfile", 10, "delta nodes (top 3 rows of the 90-min profile, ratio)"},
             {'C', "lsDeltaProfile", 10, "candle trap lots (sellers below the close / buyers above it)"},
             {'D', "lsBookFlow", 10, "defending: refill effort / pulling per side"},
             {'T', "lsTrappedTraders", 10, "trapped lots vs normal"},
             {'V', "lsDealerProfile", 15, "key / volume / gamma levels with odds (owns the level list and the ATR used to tag)"},
             {'Q', "lsDealerProfile", 15, "liquidity top 10 with rank (owns the rank)"},
             {'R', "lsDeltaProfile.READ", 20, "READ codes (A)"},
             {'H', "lsDeltaProfile.READ", 20, "the hourly flow: 4 closed hours vs the last hour at each tiered A (F?) + the current flow (DLT 2.30.0)"},
             {'X', "lsLiquidityProfile.Ex", 20, "Ex / Ex? at key levels"},
             {'L', "lsBigBidAsk", 20, "BO / BD letters (SignalBoard 1.3.0 kind L)"},
             {'K', "lsDeltaProfile.Combiner", 30, "the combined read: turn / swing odds"} };
}
inline std::vector<Edge> edges() {
    return { {"lsLiquidityProfile", "lsDealerProfile", 'B', DECIDE},          // the liquidity rank and the level tags come from LP's resting orders
             {"lsLiquidityProfile", "lsDealerProfile", 'S', DISPLAY},         // (1.4.0) the touch tag in the Liquidity column + HELD on the chart (drawn, latched for shown levels)
             {"lsDeltaProfile", "lsTrappedTraders", 'C', DECIDE},             // Who is trapped? falls back to the Delta Profile's candle lots
             {"lsDealerProfile", "lsDeltaProfile.READ", 'V', DECIDE},         // READ: level context of a node (logged)
             {"lsLiquidityProfile", "lsDeltaProfile.READ", 'P', DECIDE},      // READ: the LMT F print at the node row (logged, not drawn)
             {"lsBookFlow", "lsDeltaProfile.READ", 'D', DECIDE},              // READ: defending at the node (logged - no tested value yet)
             {"lsTrappedTraders", "lsDeltaProfile.READ", 'T', DECIDE},        // READ: trapped lots at the node (logged - no tested value yet)
             {"lsDeltaProfile", "lsDeltaProfile.READ", 'N', DECIDE},          // READ reads the layer-1 nodes
             {"lsLiquidityProfile.Ex", "lsDeltaProfile.READ", 'X', DECIDE},   // Ex shown in the READ column (same layer, decided only)
             {"lsDealerProfile", "lsDeltaProfile.Combiner", 'V', DECIDE},     // key / gamma level at the candle
             {"lsLiquidityProfile", "lsDeltaProfile.Combiner", 'P', DECIDE},  // big fill at the candle
             {"lsDeltaProfile.READ", "lsDeltaProfile.Combiner", 'R', DECIDE}, // A at the candle
             {"lsDeltaProfile", "lsDeltaProfile.Combiner", 'N', DECIDE},      // node / ratio
             {"lsBigBidAsk", "lsLiquidityProfile.Ex", 'L', DISPLAY},          // Ex stacks against the BO letter (placement only)
             {"lsBookFlow", "lsDeltaProfile.READ", 'Z', DECIDE},              // (1.1.0) the READ tier: the defenders at the A node (A+ / A)
             {"lsDeltaProfile.READ", "lsBookFlow.AtNode", 'Y', DISPLAY},
             {"lsDeltaProfile.READ", "lsLiquidityProfile.Banner", 'R', DISPLAY},   // (1.2.0) the top read = the Delta Profile read (DpBanner.h)
             {"lsDeltaProfile.READ", "lsLiquidityProfile.Banner", 'Y', DISPLAY},
             {"lsDeltaProfile.READ", "lsLiquidityProfile.Banner", 'H', DISPLAY},   // (1.3.0) the banner's "with 4h flow, pullback ?"
             {"lsTrappedTraders", "lsLiquidityProfile.Banner", 'T', DISPLAY} };    // (1.1.0) the Defense pane's At node row + mark (display only)        // Ex stacks against the BO letter (placement only)
}
struct Report { bool ok = true; std::vector<std::string> problems; };
namespace detail {
inline bool dfs(const std::string& u, std::map<std::string, std::vector<std::string> >& adj, std::map<std::string, int>& col) {
    col[u] = 1;
    for (const std::string& v : adj[u]) { if (col[v] == 1) return true; if (col[v] == 0 && dfs(v, adj, col)) return true; }
    col[u] = 2; return false;
}
}
inline Report check(const std::vector<Node>& N, const std::vector<Producer>& P, const std::vector<Edge>& E) {
    Report r; std::map<std::string, int> layer; for (const Node& n : N) layer[n.name] = n.layer;
    std::map<char, std::string> owner;
    for (const Producer& p : P) {
        if (owner.count(p.kind) && owner[p.kind] != p.owner) { r.ok = false; r.problems.push_back(std::string("duplicate producer for kind ") + p.kind); }
        owner[p.kind] = p.owner;
        if (!layer.count(p.owner)) { r.ok = false; r.problems.push_back("producer not in the graph: " + p.owner); }
        else if (layer[p.owner] != p.layer) { r.ok = false; r.problems.push_back("producer layer mismatch: " + p.owner); }
    }
    std::map<std::string, std::vector<std::string> > adj;
    for (const Edge& e : E) {
        if (!owner.count(e.kind)) { r.ok = false; r.problems.push_back(std::string("no producer for kind ") + e.kind + " read by " + e.to); continue; }
        if (owner[e.kind] != e.from) { r.ok = false; r.problems.push_back(std::string("kind ") + e.kind + " read from " + e.from + " but owned by " + owner[e.kind]); }
        if (!layer.count(e.from) || !layer.count(e.to)) { r.ok = false; r.problems.push_back("edge outside the graph: " + e.from + " -> " + e.to); continue; }
        if (layer[e.from] > layer[e.to]) { r.ok = false; r.problems.push_back("reads a HIGHER layer: " + e.to + " <- " + e.from); }
        adj[e.from].push_back(e.to);
    }
    std::map<std::string, int> col;
    for (const Node& n : N) if (col[n.name] == 0 && detail::dfs(n.name, adj, col)) { r.ok = false; r.problems.push_back("cycle through " + n.name); break; }
    return r;
}
inline Report checkDefault() { return check(nodes(), producers(), edges()); }
}  // namespace chain
#endif
