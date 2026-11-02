// t_hl2170.cpp - lsHodLod 2.17.0 HodLodHtf.h: the HTF trend (efficiency) + RS (RSI 14 vs itself) on hand-made closes and on the real
// ES 3-min closes of the last 21 days to Nov 2 14:27 CT (htf_es_1102.csv: bar END secs, close), checked against an independent Python run.
//   (from plugin\tests_hodlod) g++ -std=c++17 -Wall -Wextra -Werror -I.. t_hl2170.cpp -o t2170 && ./t2170
#include <cstdio>
#include <fstream>
#include <sstream>
#include "HodLodHtf.h"
static int pass = 0, fail = 0;
static void CHECK(bool c, const std::string& m) { if (c) ++pass; else { ++fail; std::printf("FAIL %s\n", m.c_str()); } }
int main() {
    {   std::vector<double> up; for (int i = 0; i < 21; ++i) up.push_back(100 + i); double er = 0; CHECK(htf::efficiency(up, 20, er) && er == 1.0, "a straight rise = efficiency 1.0");
        std::vector<double> zz; for (int i = 0; i < 21; ++i) zz.push_back(i % 2 ? 101 : 100); CHECK(htf::efficiency(zz, 20, er) && std::fabs(er) < 1e-9, "up/down/up.. back to the start = 0");
        CHECK(!htf::efficiency(std::vector<double>(10, 1.0), 20, er), "too few closes = no value");
        double r = 0; CHECK(htf::rsi(up, 14, r) && r == 100.0, "only up-closes = RSI 100"); }
    {   int d = 0; CHECK(htf::trendText(-0.03, d) == "Side" && d == 0, "(1.1.0) 3 = Side"); CHECK(htf::trendText(0.45, d) == "StrUp" && d > 0, "StrUp");
        CHECK(htf::trendText(-0.31, d) == "Dn" && d < 0, "Dn"); CHECK(htf::trendText(0.25, d) == "Up", "Up"); CHECK(htf::trendText(-0.6, d) == "StrDn", "StrDn");
        CHECK(htf::rsText(37.0, d) == "RS 37 Wk" && d < 0, "RS 37 Wk"); CHECK(htf::rsText(64, d) == "RS 64 Str" && d > 0, "RS 64 Str");
        CHECK(htf::rsText(50, d) == "RS 50 Mod" && d == 0, "RS 50 Mod"); CHECK(htf::rsText(74, d) == "RS 74 VStr", "RS 74 VStr"); CHECK(htf::rsText(28, d) == "RS 28 VWk", "RS 28 VWk"); }
    {   // hours by bar END: 10:03 .. 11:00 = one hour; sessions from 17:00 CT
        std::vector<htf::In> b = { { 10 * 3600 + 180, 1 }, { 11 * 3600, 2 }, { 11 * 3600 + 180, 3 }, { 16 * 3600, 4 }, { 17 * 3600 + 180, 5 } };
        CHECK(htf::hourCloses(b) == std::vector<double>({ 2, 3, 4, 5 }), "hour closes by bar END");
        CHECK(htf::sessionCloses(b) == std::vector<double>({ 4, 5 }), "a new session at 17:00 CT"); }
    {   std::ifstream f("htf_es_1102.csv"); std::string ln; std::vector<htf::In> b;
        while (std::getline(f, ln)) { std::istringstream s(ln); std::string a, c; std::getline(s, a, ','); std::getline(s, c, ','); if (!a.empty()) b.push_back(htf::In{ std::stoll(a), std::stod(c) }); }
        const htf::Out o = htf::compute(b);
        char m[200]; std::snprintf(m, sizeof m, "real ES (%zu bars): %d hours %d sessions, 60m ER %.4f day ER %.4f RSI %.2f / %.2f", b.size(), o.nH, o.nD, o.hEr, o.dEr, o.hRsi, o.dRsi);
        CHECK(b.size() == 6770 && o.nH == 340 && o.nD == 16 && std::fabs(o.hEr + 0.0311) < 1e-3 && std::fabs(o.dEr + 0.3109) < 1e-3 && std::fabs(o.hRsi - 37.0384) < 0.01 && std::fabs(o.dRsi - 31.0339) < 0.01,
              std::string("= the Python reference (-0.0311 / -0.3109 / 37.04 / 31.03): ") + m); std::printf("%s\n", m); }
    std::printf("t_hl2170 (HodLodHtf %s): %d passed, %d failed\n", htf::VERSION, pass, fail); return fail ? 1 : 0;
}
