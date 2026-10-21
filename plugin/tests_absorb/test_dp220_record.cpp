// lsDeltaProfile 2.20.0: the wick A record line (conf=wick) round trip + settings rows. Build from tests_absorb:
// g++ -std=c++17 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -I../tests_dp217 -I../tests_delta -I.. test_dp220_record.cpp
#include "../DeltaProfile.cpp"
#include <cstdio>
namespace mock { Host* current = NULL; }
static int P = 0, F = 0;
#define CHECK(c, m) do { if (c) ++P; else { ++F; std::printf("FAIL line %d: %s\n", __LINE__, m); } } while (0)
int main() {
    Latch L; L.low = 31369; L.high = 31400; L.code = "A"; L.support = false; L.candSupport = false; L.sell = false;
    L.peakTime = 1791493080; L.formedTime = 1791493080; L.formedT = 1791487080; L.peakT = 1791487080; L.price = 7846.125;
    L.id = 7; L.tFirst = 1791487100; L.tDecided = 1791487110; L.decidedBy = 1791487080; L.net = 3500; L.mult = 8.9; L.conf = 'W';
    const std::string d = recordLine(L, 'D', 0.25, "EP");
    CHECK(d.find("|conf=wick") != std::string::npos, "D line ends in conf=wick");
    RecordEvent r; CHECK(parseRecordLine(d, 0.25, r) && r.event == 'D' && r.latch.conf == 'W' && r.latch.code == "A" && !r.latch.support, "conf=wick parsed back (restart redraws the same A)");
    L.conf = 'N'; RecordEvent r2; CHECK(parseRecordLine(recordLine(L, 'D', 0.25, "EP"), 0.25, r2) && r2.latch.conf == 'N', "conf=next unchanged");
    L.conf = 'S'; RecordEvent r3; CHECK(parseRecordLine(recordLine(L, 'D', 0.25, "EP"), 0.25, r3) && r3.latch.conf == 'S', "conf=same unchanged");
    std::string bad = d; bad.replace(bad.find("conf=wick"), 9, "conf=wack"); RecordEvent r4; CHECK(!parseRecordLine(bad, 0.25, r4), "an unknown conf token is rejected");
    const std::string f = recordLine(L, 'F', 0.25, "EP"); RecordEvent r5; CHECK(parseRecordLine(f, 0.25, r5) && r5.event == 'F', "F line (24 fields) still parses");
    CHECK(confirmedAbsorption(r.latch), "a wick A is a confirmed A (drawn, posted as final)");
    const ss::Spec& S = dpset::spec();
    CHECK(S.n == dpset::K_N && dpset::K_N == 6 && S.paramVersion == 15, "settings: 6 rows (2.21.0 + trap_on), parameter version 15");
    CHECK(std::string(S.rows[dpset::K_WICK_ON].key) == "wick_on" && S.rows[dpset::K_WICK_ON].def == 1, "Wick absorption on by default");
    CHECK(std::string(S.rows[dpset::K_WICK_X].key) == "wick_x" && S.rows[dpset::K_WICK_X].def == 3.0 && S.rows[dpset::K_WICK_X].lo == 1.0 && S.rows[dpset::K_WICK_X].hi == 20.0, "Wick attack x normal 3.0 (1 - 20)");
    CHECK(std::string(DLT_VERSION) == "2.21.0" && std::string(dpset::PLUGIN_VERSION) == "2.21.0", "versions");
    std::printf("dp 2.20.0 record / settings: %d checks, %d failed\n", P + F, F);
    return F ? 1 : 0;
}
