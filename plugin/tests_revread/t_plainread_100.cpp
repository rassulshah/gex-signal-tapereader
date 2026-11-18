// PlainRead 1.0.0 / CombRead 1.2.0 / VRRead 1.2.0 / WaveRead 1.2.0 - the plain three-line reads (Rassul 2026-11-17 21:15-21:17 CT
// "more human readable and simplified ... i need something practical", "i like it. lets do this"; mockup pane-reads-simple-mockup.html)
#include "../CombRead.h"
#include "../VRRead.h"
#include "../WaveRead.h"
#include <cstdio>
static int ok = 0, bad = 0;
#define CHECK(c, m) do { if (c) ++ok; else { ++bad; std::printf("FAIL line %d: %s\n", __LINE__, m); } } while (0)
static bool tidy(const std::vector<std::string>& L) { for (const std::string& s : L) { if (s.empty() || (s.find('(') != std::string::npos && s.find("x)") == std::string::npos)) return false; } return true; }

int main() {
    // plr helpers
    CHECK(plr::noParens("A small green candle (0.7x its normal range), closed near its high (93%) with almost no upper wick.") == "A small green candle, closed near its high with almost no upper wick.", "noParens");
    { const std::vector<std::string> R = plr::reasons("big buyers lifting, but hidden sellers"); CHECK(R.size() == 2 && R[1] == "hidden sellers", "reasons drop 'but'"); }
    CHECK(plr::butJoin("A big buyer swept up through the offers.", "Hidden sellers soaked up the buying.") == "A big buyer swept up through the offers, but hidden sellers soaked up the buying.", "butJoin");

    // the ONE read - his 9:12 PM ES candle: bids +453, offers -12, delta +44 (no push) -> "Buyers leaning in ?"
    {   cbr::In x; x.mkt = "ES"; x.bookOk = true; x.add[0] = 1500; x.pull[0] = 1047; x.add[1] = 1000; x.pull[1] = 1012; x.deltaOk = true; x.dl = 44; x.dlPrev = 30; x.dlX = 0.6; x.chgX = 0.3;
        const cbr::Out o = cbr::toPlain(x, cbr::read(x));
        CHECK(o.head == "Buyers leaning in ?", o.head.c_str()); CHECK(o.verdict == 1, "verdict kept");
        CHECK(o.rows.empty() && o.why.empty() && o.ex.size() == 2, "no table, two lines");
        CHECK(o.ex[0] == "Bids stacking under price, offers thinning." && o.ex[1].rfind("Watch: bids hold", 0) == 0, o.ex[0].c_str()); }
    // book + delta agree, sellers
    {   cbr::In x; x.mkt = "ES"; x.bookOk = true; x.add[0] = 500; x.pull[0] = 700; x.add[1] = 900; x.pull[1] = 300; x.deltaOk = true; x.dl = -400; x.dlPrev = -50; x.dlX = 3.0; x.chgX = 2.5;
        const cbr::Out o = cbr::toPlain(x, cbr::read(x)); CHECK(o.head == "Sellers in control ?" && o.verdict == -1 && o.ex.size() == 2, o.head.c_str()); }
    // mixed: bids building, sellers hitting harder
    {   cbr::In x; x.mkt = "ES"; x.bookOk = true; x.add[0] = 900; x.pull[0] = 300; x.add[1] = 400; x.pull[1] = 400; x.deltaOk = true; x.dl = -400; x.dlPrev = -50; x.dlX = 3.0; x.chgX = 2.5;
        const cbr::Out o = cbr::toPlain(x, cbr::read(x)); CHECK(o.head == "Mixed ?" && o.ex[0] == "Bids building, but sellers hitting harder.", o.ex.empty() ? "?" : o.ex[0].c_str()); }
    // quiet, no data
    {   cbr::In x; x.mkt = "ES"; const cbr::Out o = cbr::toPlain(x, cbr::read(x)); CHECK(o.head == "Quiet" && o.ex[0] == "No order data for this candle." && o.ex[1] == "Nothing to act on.", o.ex[0].c_str()); }

    // VOLUME: a 100-lot buy sweep of 6 levels + iceberg offers on a heavy candle -> the sentences in words, no numbers
    {   vrd::Ctx x; for (int i = 0; i < 25; ++i) { vrd::Cd k; k.o = 100 + i * 0.1; k.c = k.o + (i % 2 ? 0.2 : -0.1); k.h = std::max(k.o, k.c) + 0.1; k.l = std::min(k.o, k.c) - 0.1; k.vol = 500; x.c.push_back(k); }
        vrd::Cd big; big.o = 102.6; big.h = 103; big.l = 102.4; big.c = 102.9; big.vol = 3000; x.c.push_back(big);
        x.in.ok = true; x.in.bigBuy = 100; x.in.bigSell = 20; x.in.swBuy = 6; x.in.swSell = 1; x.in.hidSell = 300;
        const vrd::Out raw = vrd::volumeRead(x), o = vrd::toPlain(raw, false);
        CHECK(o.rows.empty() && o.b1.empty() && o.ex.size() == 2, "volume: two lines, no table");
        CHECK(o.verdict == raw.verdict, "volume: the verdict does not change");
        CHECK(o.ex[0].find("big buyer") != std::string::npos || o.ex[0].find("Hidden sellers") != std::string::npos || o.ex[0].find("absorbed") != std::string::npos, o.ex[0].c_str());
        CHECK(o.head.find("Bullish") == std::string::npos && o.head.find("Bearish") == std::string::npos, o.head.c_str());
        CHECK(tidy(o.ex), "volume: plain words"); std::printf("   volume: %s | %s | %s\n", o.head.c_str(), o.ex[0].c_str(), o.ex[1].c_str()); }
    // RANGE: a small green candle closing at its high -> Quiet + the shape without numbers
    {   vrd::Ctx x; for (int i = 0; i < 25; ++i) { vrd::Cd k; k.o = 100 + i * 0.1; k.c = k.o + (i % 2 ? 0.4 : -0.3); k.h = std::max(k.o, k.c) + 0.2; k.l = std::min(k.o, k.c) - 0.2; k.vol = 500; x.c.push_back(k); }
        vrd::Cd s; s.o = 102.5; s.l = 102.45; s.c = 102.8; s.h = 102.82; s.vol = 400; x.c.push_back(s);
        const vrd::Out o = vrd::toPlain(vrd::rangeRead(x), true);
        CHECK(o.ex.size() == 2 && o.ex[0].find('(') == std::string::npos && o.rows.empty(), o.ex.empty() ? "?" : o.ex[0].c_str());
        std::printf("   range: %s | %s | %s\n", o.head.c_str(), o.ex[0].c_str(), o.ex[1].c_str()); }

    // WAVE: down volume building over the last 3 waves -> "Sellers stronger ?" + "Down waves are getting more volume ... (1.5x)."
    {   std::vector<wvr::W> ws; for (int i = 0; i < 8; ++i) { wvr::W w; w.dir = i % 2 ? -1 : 1; w.vol = w.dir < 0 ? 1500 : 1000; w.volX = 1.0; w.size = 10; w.sizeX = 1.0; ws.push_back(w); }
        const wvr::Out raw = wvr::read(ws, -1, "Dn 9:12 PM  Q 4/6"), o = wvr::toPlain(raw);
        CHECK(o.verdict == raw.verdict && o.rows.empty(), "wave: verdict kept, no table");
        bool cob = false; for (const auto& s : o.ex) if (s.rfind("Cob: ", 0) == 0) cob = true; CHECK(cob, "wave: the Cob line stays");
        CHECK(o.ex.back().rfind("Watch", 0) == 0 || o.ex.back() == "Nothing to act on.", o.ex.back().c_str());
        std::printf("   wave: %s | %s\n", o.head.c_str(), o.ex[0].c_str()); }
    std::printf("t_plainread_100: %d passed, %d failed\n", ok, bad); return bad ? 1 : 0;
}
