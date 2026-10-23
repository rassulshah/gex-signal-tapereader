#include "ReversalStageStream.h"
#include <cstdio>
#include <unistd.h>
static int good = 0, bad = 0;
#define CHECK(x, name) do { if (x) ++good; else { ++bad; std::printf("FAIL %d %s\n", __LINE__, name); } } while (0)
static void write(const std::string& p, const std::string& s) { std::ofstream f(p.c_str(), std::ios::binary | std::ios::trunc); f << s; }
static std::string all(const std::string& p) { std::ifstream f(p.c_str()); std::stringstream s; s << f.rdbuf(); return s.str(); }
static rse::Event ev(long long key, int pane = lst::ATK, const char* code = "K") {
    rse::Event e; e.key = key; e.pane = pane; e.l = lst::make(pane, code, lst::LOW, -1, "native lots");
    e.m.typed = true; e.m.contract = "GCZ6"; e.m.id = rse::eventId(pane, key, e.l, key); e.m.cause = key;
    e.m.published = key + 1; e.m.quality = rse::COMPLETE; e.m.basis = rse::MBO; e.m.lots = 40; e.m.priceTicks = 45000; return e;
}
static rse::Frame frame(long long key, int pane, int quality = rse::COMPLETE) {
    rse::Frame f; f.key = key; f.pane = pane; f.quality = quality; f.contract = "GCZ6"; f.published = key + 1; return f;
}
int main() {
    char t[] = "/tmp/rseXXXXXX"; const char* root = mkdtemp(t); CHECK(root != nullptr, "temporary directory"); if (!root) return 1;
    const std::string dir(root), journal = dir + "/journal"; rse::Event e = ev(100), parsed;
    CHECK(rse::parse(rse::line(e), parsed) && parsed.m.typed && parsed.m.priceTicks == 45000 && parsed.m.contract == "GCZ6", "typed roundtrip");
    for (int q = rse::NA; q <= rse::DRAINED; ++q) { e.m.outcome = q; CHECK(rse::parse(rse::line(e), parsed) && parsed.m.outcome == q, "queue outcome preserved"); }
    CHECK(rse::supportive(rse::HELD) && rse::supportive(rse::REFILLED) && !rse::supportive(rse::BROKE) && !rse::supportive(rse::NA), "support differs from unknown/adverse");
    CHECK(rse::adverse(rse::BROKE) && rse::adverse(rse::DRAINED) && !rse::adverse(rse::HELD), "adverse differs from hold");
    CHECK(!rse::parse(rse::line(e) + "|injected", parsed), "extra fields rejected");
    e.m.ratio = std::numeric_limits<double>::infinity(); CHECK(!rse::parse(rse::line(e), parsed), "infinite ratio rejected"); e.m.ratio = -1;
    e.m.lots = -2; CHECK(!rse::parse(rse::line(e), parsed), "negative quantity rejected"); e.m.lots = 40;
    e.m.published = 0; CHECK(!rse::parse(rse::line(e), parsed), "missing availability rejected"); e.m.published = 101;
    e.m.priceTicks = LLONG_MIN; CHECK(rse::parse(rse::line(e), parsed) && parsed.m.priceTicks == LLONG_MIN, "unknown price sentinel retained");
    for (int q = rse::UNVERIFIED; q <= rse::PENDING; ++q) { const auto f = frame(100, lst::ATK, q); rse::Frame x; CHECK(rse::parse(rse::line(f), x) && x.quality == q, "source quality roundtrip"); }
    CHECK(rse::fileName(lst::ATK, "GC", "GCZ6", 60) != rse::fileName(lst::ATK, "GC", "GCG7", 60), "contract-specific sources");
    const std::string p = dir + "/publisher"; rse::Publisher publisher; e = ev(100);
    CHECK(publisher.append(p, {e}, {frame(100, lst::ATK)}), "publisher succeeds"); const auto first = all(p);
    CHECK(publisher.append(p, {e}, {frame(100, lst::ATK)}) && all(p) == first, "duplicate event/frame suppressed");
    rse::Publisher restart; CHECK(restart.append(p, {e}, {frame(100, lst::ATK)}) && all(p) == first, "publisher restart dedup");
    write(p, "torn-invalid-row"); rse::Publisher torn; CHECK(torn.append(p, {e}, {}) && all(p).find("\nLE|2|") != std::string::npos, "torn row cannot concatenate new evidence");
    long long off = 0; int lines = 0; write(p, rse::line(e)); rsh::tail(p, off, [&](const std::string&) { ++lines; }); CHECK(lines == 0 && off == 0, "unterminated row remains unread");
    rsh::append(p, "complete\n"); rsh::tail(p, off, [&](const std::string&) { ++lines; }); CHECK(lines == 2, "append separates unterminated prior row");
    write(p, std::string(5000, 'z') + "\n" + rse::line(e) + "\n"); off = 0; lines = 0; rsh::tail(p, off, [&](const std::string&) { ++lines; }); CHECK(lines == 1, "oversized row skipped");
    rsh::Inbox in; in.bind(dir, "GC", "GCZ6", 60, journal);
    write(in.paths[lst::ATK], rse::line(e) + "\n" + rse::line(frame(100, lst::ATK)) + "\n");
    write(in.paths[lst::DEF], rse::line(frame(100, lst::DEF)) + "\n");
    in.read(130, 100, 1, false); CHECK(in.pendingCount() == 1 && in.quality(100) == rse::COMPLETE, "typed observation plus both acknowledgements");
    std::vector<rsh::Assignment> added; CHECK(in.admit(100, 130, added) && added.size() == 1 && added[0].received == 130 && added[0].key == 100, "actual receipt and effective decision recorded");
    CHECK(all(journal).find("RJ|1|100|130|") == 0, "persistent receipt journal");
    rsh::Inbox r; r.bind(dir, "GC", "GCZ6", 60, journal); r.read(150, 140, 1, true);
    const auto restored = r.takeRestored(); CHECK(restored.size() == 1 && restored[0].received == 130 && r.pendingCount() == 0, "restart restores assignment instead of re-admitting");
    rse::Event late = ev(105, lst::DEF, "H"); late.m.cause = 100; late.m.published = 145;
    rsh::append(r.paths[lst::DEF], rse::line(late) + "\n"); r.read(150, 140, 1, true); added.clear();
    CHECK(r.admit(140, 150, added) && added.size() == 1 && added[0].event.key == 105 && added[0].key == 140, "late observation keeps native key but applies now, never at 105");
    rsh::Inbox cold; cold.bind(dir, "GC", "GCZ6", 60, dir + "/cold"); cold.read(150, 140, 1, false);
    CHECK(cold.pendingCount() == 0 && cold.ignoredHistory == 2, "cold start does not reconstruct old live confirmations");
    rsh::Inbox missing; missing.bind(dir, "GC", "GCG7", 60, dir + "/other"); missing.read(150, 140, 1, false);
    CHECK(missing.quality(140) == rse::PENDING && !missing.terminal(140), "missing producer is pending, not neutral");
    rsh::append(in.paths[lst::DEF], rse::line(frame(100, lst::DEF, rse::PARTIAL)) + "\n"); in.read(160, 100, 1, true);
    CHECK(in.quality(100) == rse::PARTIAL, "partial producer blocks complete quality");
    rse::Event wrong = ev(100); wrong.m.id += "wrong"; wrong.m.contract = "GCG7"; rsh::append(in.paths[lst::ATK], rse::line(wrong) + "\n"); in.read(160, 100, 1, true);
    CHECK(in.wrongContract > 0, "wrong contract rejected");
    rsh::Assignment future; future.key = 100; future.received = 300; future.event = ev(100); future.event.m.id += "future";
    write(dir + "/future", rsh::line(future) + "\n"); rsh::Inbox f; f.bind(dir, "GC", "GCZ6", 60, dir + "/future"); f.read(160, 140, 1, true);
    CHECK(f.takeRestored().empty() && f.bad > 0, "future receipt journal cannot leak backward");
    // Loss and truncation of an acknowledged producer cannot retain live COMPLETE.
    const std::string sourceCopy=all(in.paths[lst::DEF]); std::remove(in.paths[lst::DEF].c_str());
    in.read(170,100,1,true); CHECK(in.quality(100)==rse::UNAVAILABLE,"lost producer blocks readiness");
    write(in.paths[lst::DEF],sourceCopy); in.read(170,100,1,true); CHECK(in.quality(100)==rse::PARTIAL,"restored producer restores actual quality");
    write(in.paths[lst::DEF],""); in.read(170,100,1,true); CHECK(in.quality(100)==rse::PENDING,"truncated producer loses stale acknowledgement");
    e=ev(110); e.m.parentCause=100; CHECK(rse::parse(rse::line(e),parsed)&&parsed.m.parentCause==100,"native pivot parent roundtrip");
    e.m.parentCause=111; CHECK(!rse::parse(rse::line(e),parsed),"future pivot parent rejected");
    const std::string pendingPath=dir+"/pending-publisher"; rse::Publisher pp; e=ev(100);e.m.quality=rse::PENDING;
    CHECK(pp.append(pendingPath,{e},{frame(100,lst::ATK,rse::PENDING)})&&all(pendingPath).find("LE|")==std::string::npos,"unfinished event does not become first-wins proof");
    e.m.quality=rse::COMPLETE; CHECK(pp.append(pendingPath,{e},{frame(100,lst::ATK)})&&all(pendingPath).find("LE|2|")!=std::string::npos,"event publishes when source finalizes");
    const std::vector<lst::Lt> warnings={lst::make(lst::DEF,"A",lst::LOW,-1),lst::make(lst::DEF,"Q!",lst::LOW,-1,"broke")};
    CHECK(lst::pick(lst::DEF,warnings).shown[0]==1&&lst::isWarning(lst::DEF,"Q!"),"queue failure is visible ahead of positive holding");
    std::printf("Evidence transport: %d passed, %d failed\n", good, bad); return bad ? 1 : 0;
}
