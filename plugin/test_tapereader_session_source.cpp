// test_tapereader_session_source.cpp -- Tape Reader must reload when its session input path changes.
// Run from plugin/: g++ -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-pie -no-pie -I. test_tapereader_session_source.cpp -o t && ./t
#include "TapeReaderLogic.h"
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static int failed = 0, passed = 0;
#define CHECK(c, m) do { if (c) ++passed; else { ++failed; std::printf("FAIL %s (line %d)\n", m, __LINE__); } } while (0)

int main() {
    const std::string d = std::string("/tmp/tapereader-session-source-") + std::to_string((long long)getpid());
    const std::string first = d + "-20261022.csv", second = d + "-20261023.csv";
    { std::ofstream f(first.c_str()); f << "same-size\n"; }
    { std::ofstream f(second.c_str()); f << "same-size\n"; }
    struct timespec ts[2]; ts[0].tv_sec = ts[1].tv_sec = 1700000000; ts[0].tv_nsec = ts[1].tv_nsec = 123456789;
    CHECK(utimensat(AT_FDCWD, first.c_str(), ts, 0) == 0 && utimensat(AT_FDCWD, second.c_str(), ts, 0) == 0, "fixture timestamps set");
    CHECK(tr::fileSig(first) == tr::fileSig(second), "same size/time paths intentionally collide in fileSig");

    tr::InputSource s; s.path = first;
    CHECK(tr::checkInput(s, 1000, 1000), "first session is checked");
    s.good = true; s.note = "old record accepted"; s.res = tr::RD_OK;
    const long long oldSig = s.sig;
    s.path = second;
    CHECK(tr::checkInput(s, 3001, 1000), "changed session path is checked despite the matching signature");
    CHECK(s.seenPath == second && s.sig == oldSig, "new path becomes the source identity while its equal signature remains valid");
    CHECK(!s.good && s.res == tr::RD_MISSING && s.note == "path changed", "old session snapshot is not eligible while the new path is loaded");

    std::ifstream src("TapeReader.cpp", std::ios::binary); std::string text((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
    CHECK(src.good() || src.eof(), "TapeReader implementation source opened");
    CHECK(text.find("return tr::checkInput(s, nowMs, every);") != std::string::npos, "TapeReader::check delegates to the path-aware production helper");
    std::remove(first.c_str()); std::remove(second.c_str());
    std::printf("%d passed, %d failed\n", passed, failed);
    return failed != 0;
}
