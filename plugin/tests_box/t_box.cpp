#include "../TurnBox.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
// ES 2026-11-27: the 9:09 turn. Inputs: the pane's candles (IRT store + RevRead record), lsSR runs / bursts, the bid-side lots below HVL per
// candle (the IRT order recording, = IRT volume at price), the record's biggest aggressor order, the two big bids followed order by order.
int main() {
  const std::string D = "/home/claude/srfull/data2/ES-2026-11-27";
  std::vector<prd::Cdl> C; std::vector<long long> K; std::vector<std::string> L; std::string line; std::ifstream f(D + "/cdl.txt");
  while (std::getline(f, line)) { std::istringstream s(line); prd::Cdl c; long long key; std::string lab; int bk; s >> key >> lab >> c.o >> c.h >> c.l >> c.c >> c.buy >> c.sell >> bk >> c.nb >> c.na >> c.iceB >> c.iceA; c.ok = true; c.book = bk; C.push_back(c); K.push_back(key); L.push_back(lab); }
  std::vector<prd::KeyRun> R; { std::ifstream g(D + "/runs.txt"); while (std::getline(g, line)) { std::istringstream s(line); prd::KeyRun r; s >> r.end >> r.lvl >> r.px >> r.side >> r.x >> r.vol >> r.ovol >> r.reach; R.push_back(r); } }
  std::map<long long, std::pair<double,double>> BU; { std::ifstream g(D + "/burst.txt"); while (std::getline(g, line)) { std::istringstream s(line); long long e; double u, d; s >> e >> u >> d; BU[e] = {u, d}; } }
  auto burst = [&](long long e, int side) -> double { auto it = BU.find(e); if (it == BU.end()) return -1; return side < 0 ? it->second.second : it->second.first; };
  auto clock = [&](int k) { int h = std::stoi(L[(size_t)k].substr(0, 2)), m = std::stoi(L[(size_t)k].substr(3)); char b[16]; std::snprintf(b, sizeof b, "%d:%02d %s", h % 12 == 0 ? 12 : h % 12, m, h < 12 ? "AM" : "PM"); return std::string(b); };
  std::vector<long long> RT(K.size()); for (size_t i = 0; i < K.size(); ++i) RT[i] = K[i] + 2082844800LL;
  const prd::LocalTable T = prd::localTable(C, K, burst);
  int i909 = -1; for (size_t i = 0; i < L.size(); ++i) if (L[i] == "09:09" && K[i] > 1795766000) i909 = (int)i; assert(i909 > 0);
  // bid-side (seller) lots below 7230 per candle end (CT minute bins from the recording)
  std::map<std::string, std::pair<long long,double>> SB = { {"09:00", {1209, 694041.25 + 8045530.25}}, {"09:03", {2249, 11470642.25 + 3809123.25 + 975985.75}}, {"09:06", {1784, 289186.5 + 12602922.75}}, {"09:09", {769, 4582235.25 + 975936.75}} };
  auto beyond = [&](long long rtk, long long lvT, int sd, long long& lots, double& pxl) -> bool { (void)lvT; (void)sd; for (size_t i = 0; i < RT.size(); ++i) if (RT[i] == rtk) { auto it = SB.find(L[i]); if (it == SB.end() || K[i] < 1795766000) return false; lots = it->second.first; pxl = it->second.second; return true; } return false; };
  auto aggrBig = [&](long long rtk, int side) -> long long { return rtk == RT[(size_t)i909] && side < 0 ? 163 : 0; };
  // the two big bids at 7227 (ticks 28908), as the recording has them
  tbx::BigTrack B; const int spb = 180; const long long t900 = RT[(size_t)i909] - 540;   // 9:00:00 in RTDATE
  B.onEvent(t900 + 0, spb, 0, 0, 28908, 100, 15251902, 51);
  for (int q = 0; q < 100; q += 10) B.onEvent(t900 + 18, spb, 3, 1, 28908, 10, 15251902, 51);
  B.onEvent(t900 + 34, spb, 0, 0, 28908, 134, 15276194, 51);
  for (int q = 0; q < 134; q += 2) B.onEvent(t900 + 34, spb, 3, 1, 28908, 2, 15276194, 51);
  const prd::Ctx cx = prd::context(C, K, i909, R, T, clock, 1, 0.25); int pv = -1; prd::srBasic(C, K, i909, R, T, clock, 0.25, 1, &pv);
  const tbx::Box bx = tbx::box(C, K, RT, i909, cx, pv, 0.25, burst, beyond, aggrBig, &B, 51, clock);
  std::cout << bx.head << "\n"; for (const tbx::Line& l : bx.lines) std::cout << "  " << l.lab << ": " << l.text << "  [" << l.col << "]\n";
  assert(bx.ok && bx.lines.size() == 4);
  assert(bx.lines[0].text.find("used up") == 0 && bx.lines[0].text.find("9:03 AM and 9:06 AM") != std::string::npos);
  assert(bx.lines[1].text.find("fuel - about 6,000 lots sold under HVL") != std::string::npos && bx.lines[1].text.find("little covered yet") != std::string::npos);
  assert(bx.lines[2].text.find("absorbed the flush") == 0);
  assert(bx.lines[3].text.find("163-lot sell") != std::string::npos);
  // tracker: a pulled big order counts as pulled; a small order is never followed; a reload grows orig, not first
  tbx::BigTrack Q; Q.onEvent(100, 180, 0, 0, 10, 80, 1, 50); Q.onEvent(101, 180, 3, 1, 10, 30, 1, 50); Q.onEvent(101, 180, 1, 0, 10, 50, 1, 50); Q.onEvent(102, 180, 2, 0, 10, 50, 1, 50);
  Q.onEvent(100, 180, 0, 0, 10, 20, 2, 50); assert(Q.open.empty() && Q.done.size() == 1 && Q.done[0].traded == 30 && Q.done[0].pulled == 50);
  const tbx::Defense d = tbx::defense(Q, 0, 0, 100, 0, 1000); assert(d.n == 1 && d.maxLots == 80);
  // a DEL right after a FILL is the trade, not a pull; a MOD down not after a fill is a partial pull; a MOD up is a reload
  tbx::BigTrack Z; Z.onEvent(100, 180, 0, 0, 10, 100, 7, 50); Z.onEvent(101, 180, 3, 1, 10, 60, 7, 50); Z.onEvent(101, 180, 2, 0, 10, 40, 7, 50);
  assert(Z.done.size() == 1 && Z.done[0].traded == 60 && Z.done[0].pulled == 0);
  tbx::BigTrack Y; Y.onEvent(100, 180, 0, 0, 10, 100, 8, 50); Y.onEvent(101, 180, 1, 0, 10, 70, 8, 50); Y.onEvent(102, 180, 1, 0, 10, 120, 8, 50);
  assert(Y.open.at(8).pulled == 30 && Y.open.at(8).orig == 150 && Y.open.at(8).first == 100);
  std::cout << "ok\n";
}
