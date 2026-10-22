// test_arrival_stamp.cpp (2026-10-22) -- ArrivalStamp.h 1.0.0 + MboRecord.h 1.2.0 (MBO recorder fix P1: arrival-time stamps).
// Build (Linux): g++ -std=c++17 -I.. test_arrival_stamp.cpp -o t && ./t     MinGW check: add -Dsmall=char
#include "../ArrivalStamp.h"
#include "../MboRecord.h"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdlib>
static int P = 0, F = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++F; std::printf("FAIL: %s\n", m); } } while (0)
static mrec::Ev ev(int h, int mi, int s, int act, long long oid) {
    mrec::Ev e; e.y = 2026; e.mo = 10; e.d = 19; e.h = h; e.mi = mi; e.s = s; e.action = act; e.price = 7767; e.prev = 7767; e.size = 2;
    e.oid = oid; e.side = 1; e.bid = 7766.75; e.ask = 7767; return e;
}
static std::vector<std::string> lines(const std::string& p) { std::vector<std::string> v; std::ifstream f(p.c_str()); std::string l; while (std::getline(f, l)) v.push_back(l); return v; }
int main() {
    // 1. the clock alone
    { arr::Clock c; int y = 2026, mo = 10, d = 19, h = 22, mi = 47, s = 26;
      CHECK(!c.apply(y, mo, d, h, mi, s), "first stamp kept");
      int y2 = 2026, mo2 = 10, d2 = 19, h2 = 22, mi2 = 31, s2 = 8;          // the 22:31:08 FILL between 22:47:26 rows (ES 10-19)
      CHECK(c.apply(y2, mo2, d2, h2, mi2, s2) && h2 == 22 && mi2 == 47 && s2 == 26, "stale FILL re-stamped to 22:47:26");
      CHECK(c.restamped == 1 && c.maxBackS == 978, "counted: 1 row, 978 s back");
      int y3 = 2026, mo3 = 10, d3 = 19, h3 = 22, mi3 = 47, s3 = 27;
      CHECK(!c.apply(y3, mo3, d3, h3, mi3, s3) && s3 == 27, "the next on-time row is untouched");
      int y4 = 2026, mo4 = 10, d4 = 19, h4 = 22, mi4 = 47, s4 = 27;
      CHECK(!c.apply(y4, mo4, d4, h4, mi4, s4), "same second is not stale"); }
    { arr::Clock c; int y = 2026, mo = 11, d = 1, h = 1, mi = 59, s = 59; c.apply(y, mo, d, h, mi, s);
      int y2 = 2026, mo2 = 11, d2 = 1, h2 = 1, mi2 = 0, s2 = 0;                 // DST fall-back: 01:59:59 CDT -> 01:00:00 CST
      CHECK(!c.apply(y2, mo2, d2, h2, mi2, s2) && h2 == 1 && mi2 == 0 && c.kept == 1, "DST fall-back hour kept (59:59 back)");
      int y3 = 2026, mo3 = 11, d3 = 1, h3 = 1, mi3 = 0, s3 = 5;
      CHECK(!c.apply(y3, mo3, d3, h3, mi3, s3), "after the fall-back the new clock is the reference"); }
    { arr::Clock c; int y = 2026, mo = 10, d = 18, h = 23, mi = 59, s = 59; c.apply(y, mo, d, h, mi, s);
      int y2 = 2026, mo2 = 10, d2 = 19, h2 = 0, mi2 = 0, s2 = 1;
      CHECK(!c.apply(y2, mo2, d2, h2, mi2, s2), "midnight / day change is forward");
      int y3 = 2026, mo3 = 10, d3 = 18, h3 = 23, mi3 = 59, s3 = 50;
      CHECK(c.apply(y3, mo3, d3, h3, mi3, s3) && d3 == 19 && h3 == 0 && s3 == 1, "stale row across midnight -> the new day's time"); }
    { arr::Clock c; int y = 2026, mo = 0, d = 0, h = 0, mi = 0, s = 0; CHECK(!c.apply(y, mo, d, h, mi, s) && c.last == -1, "not a date: untouched, no reference"); }
    // 2. the recorder writes the stale row into the ARRIVAL hour file with the arrival time
    char tmpl[] = "/tmp/arrXXXXXX"; const char* dir = mkdtemp(tmpl); CHECK(dir != nullptr, "temp dir");
    mrec::rootOverride() = dir; mrec::testFreeBytes() = 100LL * 1024 * 1024 * 1024;
    {   mrec::Recorder r; r.setMarket("ES"); r.setOn(true);
        r.add(ev(10, 59, 58, 0, 1));
        r.add(ev(11, 0, 2, 1, 2));
        r.add(ev(10, 59, 30, 3, 777));          // stale FILL (resting order's time) arriving at 11:00:02
        r.add(ev(11, 0, 3, 2, 3));
        r.tick(1000, false);
        const std::string base = std::string(dir) + "/2026-10-19/ES/";
        const std::vector<std::string> a = lines(base + "mbo_10.csv"), b = lines(base + "mbo_11.csv");
        CHECK(a.size() == 2 && a[0] == mrec::HEADER, "10:00 file: header + the one on-time 10:59:58 row");
        CHECK(b.size() == 4, "11:00 file: header + 3 rows (the stale FILL is in its arrival hour)");
        CHECK(b.size() == 4 && b[2].find("2026-10-19 11:00:02|FILL|") == 0 && b[2].find("|777|") != std::string::npos, "stale FILL written as 11:00:02, feed order kept");
        std::ostringstream st; r.status(st);
        CHECK(st.str().find(",restamped 1,max_back_s 32,") != std::string::npos, "status line: restamped 1, max_back_s 32");
        CHECK(std::string(mrec::VERSION) == "1.2.0" && std::string(arr::VERSION) == "1.0.0", "versions");
        r.setMarket("NQ"); CHECK(r.arrival.last == -1, "a market change resets the clock");
    }
    std::string rm = std::string("rm -rf ") + dir; (void)std::system(rm.c_str());
    std::printf("ArrivalStamp / MboRecord 1.2.0: %d checks, %d failed\n", P + F, F);
    return F ? 1 : 0;
}
