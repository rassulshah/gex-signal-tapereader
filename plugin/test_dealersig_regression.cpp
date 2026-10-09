#include "irtsdk.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "DealerSig.cpp"

static int passes = 0, fails = 0;
#define CHECK(c, m) do { if (c) { ++passes; std::printf("  ok    %s\n", m); } else { ++fails; std::printf("  FAIL  %s (line %d)\n", m, __LINE__); } } while (0)

static std::string ioRoot = "/tmp/dealersig-regression";
static std::string dealerPath()
{
    return ioRoot + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Dealer-GC.csv";
}
static void writeExternalCsv(const std::string& text)
{
    std::ofstream f(dealerPath().c_str(), std::ios::trunc);
    f << text;
    f.close();
}
static RTDATE utc(int hour, int minute)
{
    std::tm t = {};
    t.tm_year = 2026 - 1900; t.tm_mon = 9; t.tm_mday = 1; t.tm_hour = hour; t.tm_min = minute;
    return (RTDATE)timegm(&t);
}
static void setChart()
{
    mockrt::dates[0] = utc(10, 0); mockrt::dates[1] = utc(10, 1); mockrt::dates[2] = utc(10, 2);
}

int main()
{
    std::printf("test_dealersig_regression -- actual DealerSig.cpp with mock drawing SDK and external CSV files\n");
    mkdir(ioRoot.c_str(), 0700);
    setenv("USERPROFILE", ioRoot.c_str(), 1);
    unlink(dealerPath().c_str());
    mockrt::reset(); setChart();

    DealerSig d;
    CHECK(d.setup() == RTX_OK, "setup accepts the preserved parameter schema");

    CHECK(d.load() && d.sourceState == "source unavailable", "a missing external CSV is reported as unavailable");
    CHECK(!d.load(), "an unchanged unavailable external CSV is cached instead of reopened");

    writeExternalCsv("VERSION|1.9\nMARKET|GC\nSIG|2026-10-01 10:00:30|oB|B|1|initial buy\n");
    d.fillSignals(0);
    CHECK(mockrt::out1[0] == kSignalTrue && mockrt::out2[0] == 0, "a valid external GC SIG sets the matching buy output");

    writeExternalCsv("VERSION|1.9\nMARKET|GC\n# removal uses a different file identity\n");
    d.fillSignals(2);
    CHECK(mockrt::out1[0] == 0 && mockrt::out2[0] == 0, "a changed source fully clears removed historical signals despite incremental calc");

    mockrt::lists[(size_t)SX.ledger] = 1;
    writeExternalCsv("VERSION|1.9\nMARKET|GC\nLSIG|2026-10-01 10:00:30|SC|B|1|ledger buy\n");
    d.fillSignals(2);
    CHECK(mockrt::out1[0] == kSignalTrue, "enabling ledger includes its external LSIG output");
    mockrt::lists[(size_t)SX.ledger] = 0;
    d.fillSignals(2);
    CHECK(mockrt::out1[0] == 0, "disabling ledger fully clears its prior historical output");

    mockrt::out1[0] = kSignalTrue;
    writeExternalCsv("VERSION|1.9\nSIG|2026-10-01 10:00:30|bad|B|1|missing identity\n");
    d.fillSignals(2);
    CHECK(d.sourceState == "market missing" && mockrt::out1[0] == 0 && d.D.sigs.empty(), "a missing MARKET identity is refused and clears stale output");

    mockrt::out1[0] = kSignalTrue;
    writeExternalCsv("VERSION|1.9\nMARKET|ES\nSIG|2026-10-01 10:00:30|bad|B|1|wrong market\n");
    d.fillSignals(2);
    CHECK(d.sourceState == "market mismatch" && mockrt::out1[0] == 0 && d.D.sigs.empty(), "a filename/content market mismatch is refused and clears stale output");

    writeExternalCsv("VERSION|1.9\nMARKET|GC\nSIG|2026-10-01 10:00:30|oB|B|1|draw pending source\n");
    d.fillSignals(0);
    CHECK(mockrt::out1[0] == kSignalTrue, "restoring a matching source restores the buy output");
    writeExternalCsv("VERSION|1.9\nMARKET|GC\n# draw detects a changed empty source first\n");
    d.draw();
    d.fillSignals(2);
    CHECK(mockrt::out1[0] == 0, "a draw-side reload retains pending full signal invalidation for the next calc");

    writeExternalCsv("VERSION|1.9\nMARKET|GC\nSIG|2026-10-01 10:00:30|oB|B|1|draw point\n");
    mockrt::pntResult = RTX_FAIL; mockrt::drawTextCalls = 0;
    d.draw();
    CHECK(mockrt::pntCalls > 0 && mockrt::drawTextCalls == 0, "a failed SDK PNT::set is not dereferenced or drawn");

    std::printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
