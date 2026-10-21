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
    CHECK(mockrt::bools[(size_t)SX.ledger] == 0 && SX.ledger == SX.gap + 1, "(1.5.5) Dealer ledger marks is a check box, off by default, in the same row");

    CHECK(d.load() && d.sourceState == "source unavailable", "a missing external CSV is reported as unavailable");
    CHECK(!d.load(), "an unchanged unavailable external CSV is cached instead of reopened");

    writeExternalCsv("VERSION|1.9\nMARKET|GC\nSIG|2026-10-01 10:00:30|Dm|B|1|initial buy\n");
    d.fillSignals(0);
    CHECK(mockrt::out1[0] == kSignalTrue && mockrt::out2[0] == 0, "a valid external GC SIG sets the matching buy output");

    writeExternalCsv("VERSION|1.9\nMARKET|GC\n# removal uses a different file identity\n");
    d.fillSignals(2);
    CHECK(mockrt::out1[0] == 0 && mockrt::out2[0] == 0, "a changed source fully clears removed historical signals despite incremental calc");

    mockrt::bools[(size_t)SX.ledger] = 1;
    writeExternalCsv("VERSION|1.9\nMARKET|GC\nLSIG|2026-10-01 10:00:30|SC|B|1|ledger buy\n");
    d.fillSignals(2);
#if LRA_SHOW_OPTIONS_CAUSES
    CHECK(mockrt::out1[0] == kSignalTrue, "enabling ledger includes its external LSIG output (options gate restored)");
#else
    CHECK(mockrt::out1[0] == 0, "(1.7.0) the dealer ledger (options reasons) stays off while the options_causes gate is closed, even when ticked");
#endif
    mockrt::bools[(size_t)SX.ledger] = 0;
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

    writeExternalCsv("VERSION|1.9\nMARKET|GC\nSIG|2026-10-01 10:00:30|Dm|B|1|draw pending source\n");
    d.fillSignals(0);
    CHECK(mockrt::out1[0] == kSignalTrue, "restoring a matching source restores the buy output");
    writeExternalCsv("VERSION|1.9\nMARKET|GC\n# draw detects a changed empty source first\n");
    d.draw();
    d.fillSignals(2);
    CHECK(mockrt::out1[0] == 0, "a draw-side reload retains pending full signal invalidation for the next calc");

    writeExternalCsv("VERSION|1.9\nMARKET|GC\nSIG|2026-10-01 10:00:30|Dm|B|1|draw point\n");
    mockrt::pntResult = RTX_FAIL; mockrt::drawTextCalls = 0;
    d.draw();
    CHECK(mockrt::pntCalls > 0 && mockrt::drawTextCalls == 0, "a failed SDK PNT::set is not dereferenced or drawn");

    // ---- (1.7.0, 2026-10-14 18:25 CT audit display list, section 10) before -> after on the bar
    mockrt::pntResult = RTX_OK;
    writeExternalCsv("VERSION|1.9\nMARKET|GC\nSIG|2026-10-01 10:00:30|Dem|B|1|demand\nSIG|2026-10-01 10:00:30|Tr?|B|1|trapped\n"
                     "SIG|2026-10-01 10:01:30|Sup|S|1|supply\nSIG|2026-10-01 10:01:30|oSh|B|1|short cover\nSIG|2026-10-01 10:02:30|Van|B|1|vanna\n"
                     "SIG|2026-10-01 10:02:30|Abs|B|1|absorbed\nSIG|2026-10-01 10:02:30|Chm|S|1|charm\n");
    mockrt::drawnTexts.clear();
    d.draw();
    auto drew = [&](const char* t) { for (size_t i = 0; i < mockrt::drawnTexts.size(); i++) if (mockrt::drawnTexts[i] == t) return true; return false; };
#if !LRA_SHOW_OPTIONS_CAUSES && !LRA_SHOW_ABSORPTION_REASON
    CHECK(drew("Dem?") && !drew("Dem"), "Dem -> Dem? (footprint fact, MARK ?)");
    CHECK(drew("Tr?") && !drew("Tr"), "Tr? stays Tr? (one ?, the 1.5.0 strip then the 1.7.0 mark)");
    CHECK(drew("Sup?") && !drew("Sup"), "Sup -> Sup?");
    CHECK(!drew("oSh") && !drew("Sh") && !drew("Van") && !drew("Chm"), "options codes HIDDEN (options_causes gate)");
    CHECK(!drew("Abs") && !drew("Abs?"), "Abs HIDDEN (absorption_reason gate)");
    CHECK(mockrt::drawnTexts.size() == 3, "only the three footprint marks are drawn");
    d.fillSignals(0);
    CHECK(mockrt::out1[0] == kSignalTrue && mockrt::out2[1] == kSignalTrue && mockrt::out1[2] == 0 && mockrt::out2[2] == 0, "signal outputs follow the same gates (bar 2: only options / Abs -> no output)");
    CHECK(dsText(dl::Data::Sig(), false).empty(), "an empty code draws nothing");
#endif

    {   // (1.8.0, Rassul 16:05 CT 2026-10-21 "fix this, it should be solid") the lsHodLod table over the first mark: it is not painted,
        // the others are; the signal outputs are unchanged; a table on another chart changes nothing
        mockrt::drawnTexts.clear(); mockrt::textBoxes.clear(); d.draw();
        const size_t n0 = mockrt::drawnTexts.size();
        CHECK(n0 >= 1, "[keep-out] marks drawn with no table");
        if (n0 >= 1) {
            const std::vector<int> b = mockrt::textBoxes[0]; const bdk::Box T = bdk::mk(b[0] - 3, b[1] - 3, b[2] + 3, b[3] + 3);
            bdk::publishHodLod(mockrt::root, 60, mockrt::chart, 1, T);
            mockrt::drawnTexts.clear(); mockrt::textBoxes.clear(); d.draw();
            bool inside = false; for (auto& x : mockrt::textBoxes) if (bdk::hitsTable(T, x[0], x[1], x[2], x[3])) inside = true;
            CHECK(!inside && mockrt::drawnTexts.size() < n0, "[keep-out] the mark under the table is not painted");
            d.fillSignals(0); const unsigned long o1 = mockrt::out1[0];
            bdk::clearHodLod(mockrt::root, 60, mockrt::chart); d.fillSignals(0);
            CHECK(mockrt::out1[0] == o1, "[keep-out] the signal output is the same with or without the table");
            bdk::publishHodLod(mockrt::root, 60, "another chart", 1, T);
            mockrt::drawnTexts.clear(); d.draw();
            CHECK(mockrt::drawnTexts.size() == n0, "[keep-out] a table on another chart: every mark drawn");
            bdk::clearHodLod(mockrt::root, 60, "another chart");
        }
    }
    std::printf("\n%d passed, %d failed\n", passes, fails);
    return fails;
}
