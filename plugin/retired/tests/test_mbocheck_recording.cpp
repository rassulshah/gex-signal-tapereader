// MboCheck raw-recorder host regression. Build from plugin/:
//   g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-pie -no-pie -Itests_st/mock -I. test_mbocheck_recording.cpp -o test_mbocheck_recording
#include "irtsdk.h"
namespace mock { Feed feed; Host* current = nullptr; }
// This established mock has no periodicity methods; 60 seconds is sufficient only for the diagnostic file identity in this host test.
#define getPeriodicity getSecondsPerBar
#define getIntradayInterval getSecondsPerBar
#include "MboCheck.cpp"
#undef getIntradayInterval
#undef getPeriodicity
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

static int failures = 0;
static void check(bool ok, const char* what)
{
    if (!ok) { ++failures; std::printf("FAIL %s\n", what); }
}
static std::vector<std::string> lines(const std::string& path)
{
    std::vector<std::string> out; std::ifstream f(path.c_str()); std::string line;
    while (std::getline(f, line)) out.push_back(line);
    return out;
}
static bool onlyOrder(const std::vector<std::string>& rows, int id)
{
    return rows.size() == 2 && rows[0].find("seq|ct_time|steady_ms|action|side|") == 0
        && rows[1].find("|" + std::to_string(id) + "|") != std::string::npos;
}

int main()
{
    const std::string home = std::string("/tmp/mbocheck_recording_") + std::to_string((long long)getpid());
    setenv("USERPROFILE", home.c_str(), 1);
    mock::Host host; mock::current = &host;
    host.symbol = "ESZ26"; host.root = "ES"; host.seconds = 60; host.now = 17UL * 3600UL;
    // One SDK getNext() drain crosses the 17:00 CT session boundary. The baseline used the final stamp for both rows.
    mock::feed.push(0, 1, 100.0f, 5, 16UL * 3600UL + 59UL * 60UL + 59UL, 101);
    mock::feed.push(2, 0, 99.75f, 7, 17UL * 3600UL, 202);

    MboCheck p;
    p.setup(); p.draw();
    MCState* state = p.testState();
    const std::string stem = state ? mbo::fileStem(state->id) : std::string();
    const std::string raw = home + "\\InvestorRT\\rtx\\lsFlexLevels\\MboCheck\\raw\\" + stem + "-";
    const std::vector<std::string> pre = lines(raw + "19700101.csv");
    const std::vector<std::string> post = lines(raw + "19700102.csv");
    check(state && state->p.events == 2 && state->p.recorderLines == 2 && state->p.recorderFailures == 0 && state->p.recorderLostLines == 0,
          "two drained DBO rows are durable with no recorder failure or hidden loss");
    check(onlyOrder(pre, 101) && onlyOrder(post, 202), "one boundary-spanning drain writes each raw order ID to its event-derived session file");

    const std::string status = home + "\\InvestorRT\\rtx\\lsFlexLevels\\MboCheck\\" + stem + ".txt";
    const std::vector<std::string> statusRows = lines(status);
    bool recorderTruth = false;
    for (size_t i = 0; i < statusRows.size(); ++i) if (statusRows[i].find("RECORDER,durable_lines 2,append_failures 0,lost_lines 0,unassigned_timestamp_lines 0") == 0) recorderTruth = true;
    check(recorderTruth, "status makes recorder durability and partial-data counters explicit");

    p.destroy();
    std::remove((raw + "19700101.csv").c_str()); std::remove((raw + "19700102.csv").c_str()); std::remove(status.c_str());
    rmdir((home + "\\InvestorRT\\rtx\\lsFlexLevels\\MboCheck\\raw").c_str());
    rmdir((home + "\\InvestorRT\\rtx\\lsFlexLevels\\MboCheck").c_str());
    std::printf("RESULT %d failed\n", failures);
    return failures ? 1 : 0;
}
