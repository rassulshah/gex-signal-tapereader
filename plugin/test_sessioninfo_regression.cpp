#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include "SessionInfo.cpp"

static int checks = 0;

static void check(bool condition, const char* label)
{
    ++checks;
    if (!condition) {
        std::cerr << "FAIL: " << label << "\n";
        std::exit(1);
    }
}

static std::string sessionPath(const std::string& profile, const std::string& market)
{
    return profile + "\\InvestorRT\\rtx\\lsFlexLevels\\LRA-Session-" + market + ".csv";
}

static bool sawText(const std::string& wanted)
{
    const std::vector<std::string>& text = sessioninfo_mock::drawnText();
    for (size_t i = 0; i < text.size(); ++i) if (text[i] == wanted) return true;
    return false;
}

int main()
{
    char tempTemplate[] = "/tmp/sessioninfo-regression-XXXXXX";
    char* tempDir = mkdtemp(tempTemplate);
    check(tempDir != 0, "mkdtemp");
    std::string profile(tempDir);
    check(setenv("USERPROFILE", profile.c_str(), 1) == 0, "set USERPROFILE");

    check(hm("08:15") == 495, "strict parser accepts valid HH:MM");
    check(hm("8:15") == -1 && hm("24:00") == -1 && hm("12:60") == -1 && hm("1x:00") == -1, "strict parser rejects malformed times");
    check(inWindow(15, 1410, 30) && inWindow(0, 1410, 30) && !inWindow(30, 1410, 30), "overnight window membership");
    int parsedInt = 0;
    check(parseInt("2147483647", parsedInt) && parsedInt == INT_MAX && parseInt("-2147483648", parsedInt) && parsedInt == INT_MIN,
          "external integer parser accepts both int endpoints");
    check(!parseInt("2147483648", parsedInt) && !parseInt("-2147483649", parsedInt) && !parseInt("12min", parsedInt),
          "external integer parser rejects overflow and trailing text");

    const std::string esFile = sessionPath(profile, "ES");
    {
        std::ofstream out(esFile.c_str());
        out << "EXP|24:00\n";
        out << "WIN|ACTIVE|09:60|10:00\n";
        out << "WIN|OTHER|08:00|09:00\n";
        for (int i = 0; i < 65; ++i) out << "WIN|ACTIVE|23:30|00:30|x|" << i << "\n";
        for (int i = 0; i < 65; ++i) out << "CAL|08:00|High|event " << i << "\n";
    }
    SessionInfo parsed;
    parsed.testSetRoot("ESZ26");
    parsed.cfg.market = 0;
    parsed.load();
    check(parsed.sourceAvailable, "complete existing source is available");
    check(parsed.exp.empty(), "invalid expiry is ignored rather than formatted as a false time");
    check(parsed.W.size() == MAX_SESSION_WINDOWS, "windows are bounded");
    check(parsed.W[0].fromMin == 1410 && parsed.W[0].toMin == 30, "validated window stores parsed minutes");
    check(parsed.news.size() == MAX_SESSION_NEWS, "news records are bounded");

    const std::string nqFile = sessionPath(profile, "NQ");
    std::remove(nqFile.c_str());
    SessionInfo missing;
    missing.testSetRoot("NQZ26");
    missing.cfg.market = 0;
    missing.load();
    const std::string missingPath = nqFile;
    check(!missing.sourceAvailable && missing.stamp == -1 && missing.path == missingPath, "missing file state is cached");
    missing.W.push_back(Win());
    missing.load();
    check(missing.W.size() == 1, "unchanged missing file bypasses a repeated open/parse attempt");

    SessionInfo configured;
    configured.setup();
    configured.testSetRoot("ESZ26");
    configured.testSetList(SP.market, 5); // HG override selected in a settings dialog on the ES chart.
    configured.testSetList(SP.show, 0);
    configured.testSetList(SP.moveX, 0);
    configured.testSetList(SP.bg, 0);
    configured.parmsApply();
    const std::string hgFile = sessionPath(profile, "HG");
    { std::ofstream out(hgFile.c_str()); out << "EXP|12:00\n"; }

    SessionInfo reopened;
    reopened.setup();
    reopened.testSetRoot("ESZ26");
    reopened.draw();
    check(reopened.cfg.market == 5 && reopened.mkt == "HG", "saved Market selection is restored by native chart market when lists are unavailable");

    sessioninfo_mock::clearDrawnText();
    SessionInfo unavailable;
    unavailable.setup();
    unavailable.testSetRoot("NQZ26");
    unavailable.draw();
    check(!unavailable.sourceAvailable && sawText("NQ session data unavailable"), "missing external source is explicit in the panel");

    SessionInfo fitter;
    std::string longText(1024, 'a');
    fitter.testResetWidthCalls();
    std::string fitted = fitter.fit(longText, 100, 8, false);
    check(fitted.size() == 100 && fitted.substr(97) == "...", "fit retains the longest width-safe prefix");
    check(fitter.testWidthCalls() <= 13, "fit uses logarithmic width measurements for long external text");

    std::remove(esFile.c_str());
    std::remove(nqFile.c_str());
    std::remove(hgFile.c_str());
    std::remove(dl::placePath("SessionInfoMarket", "ES").c_str());
    std::remove(dl::placePath("SessionInfo", "HG").c_str());
    std::remove(dl::placePath("SessionInfoBg", "HG").c_str());
    std::remove(dl::placePath("SessionInfoShow", "HG").c_str());
    rmdir(profile.c_str());

    std::cout << "PASS: " << checks << " regression checks\n";
    return 0;
}
