// render_day.cpp -- runs lsCVD on real bars in the mock host and dumps the final frame's draw calls (render_day.py turns them into a PNG).
// g++ -std=c++17 -O2 -Imock -I../plugin render_day.cpp -o render_day && ./render_day <MKT> <root> <tick> <last bar END "YYYY-MM-DD HH:MM"> <bars shown> out.csv in.csv...
#include "CVD.cpp"
#include <fstream>
#include <sstream>
#include <unistd.h>
namespace mock { Host* current = nullptr; }
int main(int argc, char** argv) {
    if (argc < 8) return 2;
    char tmpl[] = "/tmp/cvdrXXXXXX"; setenv("USERPROFILE", mkdtemp(tmpl), 1);
    int Y, M, D, h, m; std::sscanf(argv[4], "%d-%d-%d %d:%d", &Y, &M, &D, &h, &m);
    const long long last = (long long)svl::daysFromCivil(Y, M, D) * 86400LL + h * 3600 + m * 60;
    std::map<long long, mock::Bar> rows;
    for (int a = 7; a < argc; ++a) {
        std::ifstream in(argv[a]); std::string line; std::getline(in, line);
        while (std::getline(in, line)) {
            std::stringstream ss(line); std::string c[9]; for (int i = 0; i < 9; ++i) std::getline(ss, c[i], ',');
            int y, mo, d, hh, mi, se; if (std::sscanf(c[0].c_str(), "%d-%d-%d %d:%d:%d", &y, &mo, &d, &hh, &mi, &se) != 6) continue;
            const long long en = (long long)svl::daysFromCivil(y, mo, d) * 86400LL + hh * 3600 + mi * 60 + se + 180;
            if (en > last + 180) continue;                                 // + one forming bar
            mock::Bar b; b.time = (unsigned long)en; b.open = std::stof(c[1]); b.high = std::stof(c[2]); b.low = std::stof(c[3]); b.close = std::stof(c[4]);
            b.volume = (unsigned long)std::stod(c[5]); const long by = c[6].empty() ? 0 : (long)std::stod(c[6]), sl = c[7].empty() ? 0 : (long)std::stod(c[7]);
            if (c[6].empty()) b.failStats = 1;
            b.rows.push_back(mock::Price(b.close, by, sl, by + sl)); rows[en] = b;
        }
    }
    mock::Host H; mock::current = &H; H.root = argv[2]; H.symbol = std::string(argv[2]) + "Z6"; H.tick = (float)std::atof(argv[3]); H.seconds = 180;
    for (auto& kv : rows) H.bars.push_back(kv.second);
    const int shown = std::atoi(argv[5]);
    H.paneLeft = 0; H.paneTop = 0; H.paneRight = 1500; H.paneBottom = 470; H.barPx = 1460 / shown; H.firstBar = (int)H.bars.size() - shown;
    CVD ind; ind.calc(0); H.draws.clear(); ind.draw();
    std::ofstream o(argv[6]); o << "type,x,y,r,b,color,text\n";
    for (const mock::Draw& d : H.draws) { std::string t = d.text; for (char& ch : t) if (ch == ',') ch = ';'; o << d.type << ',' << d.x << ',' << d.y << ',' << d.r << ',' << d.b << ',' << d.color << ',' << t << '\n'; }
    RuntimeState* s = ind.stateFor();
    std::fprintf(stderr, "bars %zu state %s marks %zu levels %zu\n", H.bars.size(), s->state.c_str(), s->feed.marks.size(), s->feed.levels().size() / 2);
    for (const cvd::Level& l : s->feed.levels()) if (l.support) std::fprintf(stderr, "  level %s %.2f\n", l.id.c_str(), l.price);
    return 0;
}
