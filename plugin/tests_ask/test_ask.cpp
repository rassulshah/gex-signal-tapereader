// test_ask.cpp -- unit tests for AskBoard.h 0.1.0 (HB_FAKE board, Linux). Build: see run_tests.sh
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>
#include "AskBoard.h"

static int fails = 0, checks = 0;
#define CHECK(c) do { ++checks; if (!(c)) { ++fails; std::printf("FAIL line %d: %s\n", __LINE__, #c); } } while (0)

static const std::string SYM = "ESZ26", CH = "chart-1"; static const long SPB = 60;

static rx::Row row(double px, std::map<std::string, std::string> kv) { rx::Row r; r.px = px; r.kv = kv; return r; }

static void testIndex() {
    rx::clear("LIQ", SYM, SPB, CH);
    const long long T0 = 1000000;   // bar time (s)
    std::vector<rx::Row> rows = { row(7712.0, {{"D.lots","310"},{"D.orders","4"},{"R.side","bull"}}), row(7711.75, {{"D.lots","45"}}),
                                  row(7700.0, {{"M.lots_x","2.4"}}), row(7690.0, {{"R.only","x"}}) };
    const int kept = rx::post("LIQ", SYM, SPB, CH, 5000, T0, 7711.5, rows);
    CHECK(kept == 3);                                                    // the READ-only row is not indexed
    rx::Index I = rx::read("LIQ", SYM, SPB, CH, 6000, T0);
    CHECK(I.ok && I.bar == T0 && I.rows.size() == 3);
    const rx::Row* r = rx::at(I, 7712.0, 0.0); double x = 0;
    CHECK(r && rx::fieldD(r, "D.lots", x) && x == 310);
    CHECK(r && r->kv.count("R.side") == 0);                             // opinions never stored in the index
    CHECK(!rx::fieldD(r, "lots", x));                                    // a key without its class is not a fact
    CHECK(rx::at(I, 7705.0, 0.5) == nullptr);                            // nothing within tolerance
    double tot = 0; int n = 0; CHECK(rx::sum(I, 7711.875, 0.13, "D.lots", tot, n) && n == 2 && tot == 355);
    // no look-ahead: the next bar's index is invisible to a reader deciding T0, and the T0 index stays readable
    rx::post("LIQ", SYM, SPB, CH, 7000, T0 + SPB, 7713.0, { row(7713.0, {{"D.lots","90"}}) });
    CHECK(rx::read("LIQ", SYM, SPB, CH, 7500, T0).bar == T0);
    CHECK(rx::read("LIQ", SYM, SPB, CH, 7500, T0 + SPB).bar == T0 + SPB);
    CHECK(!rx::read("LIQ", SYM, SPB, CH, 7500, T0 - SPB).ok);
    // freshness: an index older than 60 s is ignored (indicator removed / stopped)
    CHECK(!rx::read("LIQ", SYM, SPB, CH, 5000 + mst::FRESH_MS + 3000, T0 + SPB).ok);
    // clock running backwards -> ignored
    CHECK(!rx::read("LIQ", SYM, SPB, CH, 4000, T0).ok);
    // another chart / symbol is isolated
    CHECK(!rx::read("LIQ", SYM, SPB, "chart-2", 7500, T0 + SPB).ok);
    CHECK(!rx::read("LIQ", "NQZ26", SPB, CH, 7500, T0 + SPB).ok);
    // size cap: the rows nearest the last price are kept
    std::vector<rx::Row> big; for (int i = 0; i < 3000; ++i) big.push_back(row(7000.0 + i * 0.25, {{"D.lots", std::to_string(i)},{"D.orders","3"},{"D.placed_ms","123456789"}}));
    const int k2 = rx::post("LIQ", SYM, SPB, CH, 8000, T0 + 2 * SPB, 7375.0, big);
    rx::Index B = rx::read("LIQ", SYM, SPB, CH, 8000, T0 + 2 * SPB);
    CHECK(k2 > 100 && k2 < 3000 && B.ok && (int)B.rows.size() == k2 && B.dropped == 3000 - k2);
    CHECK(rx::at(B, 7375.0, 0.0) != nullptr && rx::at(B, 7000.0, 0.0) == nullptr);
    std::string raw; bb::bbGet(rx::keyOf("LIQ", SYM, SPB, CH, 0), raw); CHECK(raw.size() <= rx::MAX_INDEX + 128);
    // separators in values are neutralised (no injection of extra rows / fields)
    rx::post("DP", SYM, SPB, CH, 9000, T0, 7711.0, { row(7711.0, {{"D.note","a/b,c=d|e"}}) });
    rx::Index D = rx::read("DP", SYM, SPB, CH, 9000, T0); CHECK(D.ok && D.rows.size() == 1 && D.rows[0].kv.size() == 1);
    // "?" is unknown, never a number
    rx::post("DP", SYM, SPB, CH, 9000, T0 + SPB, 7711.0, { row(7711.0, {{"D.delta","?"}}) });
    rx::Index U = rx::read("DP", SYM, SPB, CH, 9000, T0 + SPB); CHECK(U.ok && !rx::fieldD(rx::at(U, 7711.0, 0), "D.delta", x));
    // garbage is rejected
    bb::bbSet(rx::keyOf("AGG", SYM, SPB, CH, 0), "junk"); CHECK(!rx::read("AGG", SYM, SPB, CH, 9000, T0).ok);
}

static void testChannel() {
    const long long T = 2000040; const long long now = 100000;
    bb::bbSet(ask::qKey("LIQ", SYM, SPB, CH), ""); bb::bbSet(ask::aKey("DP", SYM, SPB, CH), "");
    CHECK(ask::post("DP", "LOTS_AT", "7712", T, 1, SPB, SYM, CH, now).empty());      // a lookup topic is not a request
    CHECK(ask::post("DP", "NOPE", "7712", T, 1, SPB, SYM, CH, now).empty());
    const std::string id = ask::post("DP", "ORDER_FATE", "7712.00", T, 2, SPB, SYM, CH, now);
    CHECK(id.size() == 8);
    CHECK(ask::post("DP", "ORDER_FATE", "7712.00", T, 2, SPB, SYM, CH, now) == id);   // same question = same id, not queued twice
    CHECK(ask::readQueue("LIQ", SYM, SPB, CH).size() == 1);
    // asker side, before any answer: PENDING; deadline T + 2 bars
    ask::Answer A = ask::collect("DP", id, T + 2 * SPB, T, SYM, CH.empty() ? 0 : SPB, CH);
    CHECK(A.st == ask::AState::PENDING);
    // the owner cannot answer with a bar decided before the question
    std::vector<ask::Q> P = ask::pending("LIQ", SYM, SPB, CH, now + 10, T - SPB); CHECK(P.empty());
    P = ask::pending("LIQ", SYM, SPB, CH, now + 500, T + SPB); CHECK(P.size() == 1 && P[0].topic == "ORDER_FATE" && P[0].args == "7712.00");
    CHECK(!ask::answer(P[0], T - SPB, {{"D.state","pulled"}}, SYM, SPB, CH, now + 500));
    CHECK(ask::answer(P[0], T + SPB, {{"D.state","pulled"},{"D.lots","310"},{"R.view","bear"}}, SYM, SPB, CH, now + 500));
    CHECK(ask::readQueue("LIQ", SYM, SPB, CH).empty());                                // answered -> removed
    // the asker still deciding bar T cannot see an answer decided for T + 1 bar (no look-ahead)
    A = ask::collect("DP", id, T + 2 * SPB, T, SYM, SPB, CH); CHECK(A.st == ask::AState::PENDING);
    A = ask::collect("DP", id, T + 2 * SPB, T + SPB, SYM, SPB, CH);
    CHECK(A.st == ask::AState::ANSWERED && A.ansBar == T + SPB && A.latMs == 500 && A.kv["D.state"] == "pulled" && A.kv.count("R.view") == 0);
    // expiry: an unanswered question past its deadline
    const std::string id2 = ask::post("DP", "DEF_RELOAD", "7700", T, 1, SPB, SYM, CH, now);
    CHECK(ask::readQueue("BOOK", SYM, SPB, CH).size() == 1);
    CHECK(ask::collect("DP", id2, T + SPB, T + SPB, SYM, SPB, CH).st == ask::AState::PENDING);
    CHECK(ask::collect("DP", id2, T + SPB, T + 2 * SPB, SYM, SPB, CH).st == ask::AState::EXPIRED);
    CHECK(ask::pending("BOOK", SYM, SPB, CH, now + 10, T + 2 * SPB).empty());          // owner past the deadline: not answered late
    // wait is capped at 5 bars
    ask::Q q; const std::string id3 = ask::post("STAGE", "TRAPPED_AT", "7690", T, 99, SPB, SYM, CH, now);
    std::vector<ask::Q> dq = ask::readQueue("DP", SYM, SPB, CH); CHECK(dq.size() == 1 && dq[0].deadlineBar == T + 5 * SPB && dq[0].id == id3);
    // queue cap 16, oldest dropped; stale questions (> 2 min) dropped
    bb::bbSet(ask::qKey("LIQ", SYM, SPB, CH), "");
    for (int i = 0; i < 20; ++i) ask::post("DP", "ORDER_FATE", std::to_string(7700 + i), T, 1, SPB, SYM, CH, now + i);
    std::vector<ask::Q> lq = ask::readQueue("LIQ", SYM, SPB, CH); CHECK(lq.size() == ask::MAX_Q && lq.front().args == "7704");
    ask::post("DP", "ORDER_FATE", "7800", T, 1, SPB, SYM, CH, now + ask::Q_TTL_MS + 100);
    CHECK(ask::readQueue("LIQ", SYM, SPB, CH).size() <= 2);
    // args are cleaned (cannot break the queue)
    bb::bbSet(ask::qKey("LIQ", SYM, SPB, CH), "");
    ask::post("DP", "ORDER_FATE", "77|12^x", T, 1, SPB, SYM, CH, now); CHECK(ask::readQueue("LIQ", SYM, SPB, CH).size() == 1);
    (void)q;
}

static void testPlan() {
    askp::Plan D = askp::parse("", "ES"); CHECK(!askp::on(D, "DP") && askp::questions(D, "DP", "rth_with").empty());
    const std::string f = "VERSION,1\r\nMARKET,ES\nASK,DP,rth_with,ORDER_FATE,2,P\nASK,DP,*,LOTS_AT,0,C\nASK,DP,rth_with,LOTS_AT,1,P\nASK,DP,rth_with,BOGUS,1,P\nASK,STAGE,*,ATTEMPT_N,0\n";
    askp::Plan P = askp::parse(f, "ES");
    CHECK(P.note == "4 question lines");
    CHECK(askp::questions(P, "DP", "rth_with").empty());                              // listed but no SWITCH line -> OFF -> asks nothing
    CHECK(askp::questions(askp::parse(f + "SWITCH,DP,0\n", "ES"), "DP", "rth_with").empty());
    CHECK(askp::questions(askp::parse(f + "SWITCH,DP,yes\n", "ES"), "DP", "rth_with").empty());   // anything but 1 / 2 = OFF
    askp::Plan P2 = askp::parse(f + "SWITCH,DP,1\n", "ES");                             // ON: proven questions only
    std::vector<std::pair<std::string, int>> qs = askp::questions(P2, "DP", "rth_with");
    CHECK(qs.size() == 2 && qs[0].first == "ORDER_FATE" && qs[0].second == 2 && qs[1].first == "LOTS_AT" && qs[1].second == 1);
    CHECK(askp::questions(P2, "DP", "eth_counter").empty());                          // the '*' LOTS_AT is a candidate: not asked when ON
    CHECK(askp::questions(P2, "STAGE", "rth_with").empty());
    askp::Plan S = askp::parse(f + "SWITCH,DP,2\nSWITCH,STAGE,2\n", "ES");               // SHADOW: proven + candidates
    CHECK(askp::shadow(S, "DP") && askp::questions(S, "DP", "eth_counter").size() == 1 && askp::questions(S, "STAGE", "x").size() == 1);
    cr::Read wo, wi; wo.side = 1; wi.side = -1;
    CHECK(askp::drawn(S, "DP", wo, wi).side == 1 && askp::drawn(P2, "DP", wo, wi).side == -1);   // shadow never draws the answered read
    askp::Plan W = askp::parse(f + "SWITCH,DP,1\n", "NQ");                              // a file for another market is ignored
    CHECK(!askp::on(W, "DP") && W.note.find("not NQ") != std::string::npos);
    askp::Plan C = askp::parse("ASK,DP,rth_with,ORDER_FATE,40,P\nSWITCH,DP,1\n", "ES");  // wait capped at 5
    CHECK(askp::questions(C, "DP", "rth_with")[0].second == 5);
}

static void testRecords() {
    ask::Pending P; P.asker = "DP"; P.t = 2000040; P.qids = {"0a0b0c0d", "11223344"}; P.topics = {"ORDER_FATE", "LOTS_AT"};
    P.without.side = 1; P.without.conf = 55;
    cr::Read with; with.side = 1; with.conf = 70; with.reasons = "offer pulled | a=b; x";
    const std::string L = ask::askrLine(P, 2000100, 1, with);
    CHECK(L == "ASKR|1|DP|2000040|2000100|1/2|ORDER_FATE+LOTS_AT|1|55|1|70|offer pulled   a b  x" || L.find("ASKR|1|DP|2000040|2000100|1/2|ORDER_FATE+LOTS_AT|1|55|1|70|") == 0);
    CHECK(std::count(L.begin(), L.end(), '|') == 11);
    ask::Answer A; A.st = ask::AState::ANSWERED; A.ansBar = 2000100; A.latMs = 420;
    CHECK(ask::askLine("DP", "0a0b0c0d", "ORDER_FATE", 2000040, A) == "ASK|1|DP|0a0b0c0d|ORDER_FATE|2000040|answered|2000100|420");
    ask::Answer E; E.st = ask::AState::EXPIRED;
    CHECK(ask::askLine("DP", "11223344", "LOTS_AT", 2000040, E) == "ASK|1|DP|11223344|LOTS_AT|2000040|expired|-|-");
    CHECK(ask::recName("ES", 60, 20261026) == "Context-ASK-ES-60s-20261026.rec");
    CHECK(ask::recName("E/S|", 60, 20261026) == "Context-ASK-ES-60s-20261026.rec");
    const char* td = std::getenv("TMPDIR"); const std::string dir = td ? td : "/tmp";
    const std::string nm = "Context-ASK-T" + std::to_string((long)std::time(nullptr) % 100000) + "-60s-20261026.rec";
    CHECK(ask::appendRecord(dir, nm, L) && ask::appendRecord(dir, nm, "ASK|1|x"));
    { FILE* f = std::fopen((dir + "/" + nm).c_str(), "rb"); char b[600] = {0}; size_t n = f ? std::fread(b, 1, sizeof(b) - 1, f) : 0; if (f) std::fclose(f);
      CHECK(n > 0 && std::string(b).find("\r\nASK|1|x\r\n") != std::string::npos); std::remove((dir + "/" + nm).c_str()); }
    CHECK(!ask::appendRecord("", nm, L));
    // determinism: same inputs -> same question id
    CHECK(ask::qid("DP", "ORDER_FATE", 5, "7712") == ask::qid("DP", "ORDER_FATE", 5, "7712"));
    CHECK(ask::qid("DP", "ORDER_FATE", 5, "7712") != ask::qid("DP", "ORDER_FATE", 6, "7712"));
}

// end-to-end: DP asks LIQ "was the offer at 7,712 pulled?"; DP draws nothing while pending; the final read is decided when the answer arrives
static void testFlow() {
    const long long T = 3000000; long long now = 200000;
    bb::bbSet(ask::qKey("LIQ", SYM, SPB, CH), ""); bb::bbSet(ask::aKey("DP", SYM, SPB, CH), "");
    askp::Plan plan = askp::parse("MARKET,ES\nSWITCH,DP,1\nASK,DP,*,ORDER_FATE,2,P\n", "ES");
    ask::Pending P; P.asker = "DP"; P.t = T; P.without.side = -1; P.without.conf = 50;
    for (const auto& q : askp::questions(plan, "DP", "rth")) { P.qids.push_back(ask::post("DP", q.first, "7712", T, q.second, SPB, SYM, CH, now)); P.topics.push_back(q.first); P.deadline = T + q.second * SPB; }
    CHECK(P.qids.size() == 1 && !P.qids[0].empty());
    std::vector<std::string> drawn;                                                     // what DP has drawn, by bar
    // bar T: DP decides PENDING -> draws nothing for this read
    // bar T+1: LIQ runs, answers
    for (const ask::Q& q : ask::pending("LIQ", SYM, SPB, CH, now + 300, T + SPB)) ask::answer(q, T + SPB, {{"D.state","pulled"}}, SYM, SPB, CH, now + 300);
    // bar T+1: DP collects, makes the read FINAL at T+1 (not back-dated to T)
    ask::Answer A = ask::collect("DP", P.qids[0], P.deadline, T + SPB, SYM, SPB, CH);
    CHECK(A.st == ask::AState::ANSWERED);
    cr::Read with = P.without; if (A.kv["D.state"] == "pulled") { with.side = -1; with.conf = 68; with.reasons = "offer 7712 pulled"; }
    drawn.push_back(std::to_string(T + SPB) + ":" + std::to_string(with.side));
    const std::string L = ask::askrLine(P, T + SPB, 1, with);
    CHECK(L.find("ASKR|1|DP|3000000|3000060|1/1|ORDER_FATE|-1|50|-1|68|") == 0);
    CHECK(drawn.size() == 1 && drawn[0] == "3000060:-1");
    (void)now;
}

int main() {
    testIndex(); testChannel(); testPlan(); testRecords(); testFlow();
    std::printf("test_ask: %d checks, %d failed (rx %s, ask %s, askp %s)\n", checks, fails, rx::VERSION, ask::VERSION, askp::VERSION);
    return fails ? 1 : 0;
}
