PressRead.h 1.0.0 test (lsRevRead 0.45.0). ES 2026-11-27 3-min bars (IRT store, bar end keys), the RevRead order record (book net, icebergs) and the SR journal (runs, bursts).
Build + run (Linux):  g++ -std=c++17 -fsanitize=address,undefined -Wall -Wextra -Werror t_press.cpp -o t_press && ./t_press
Expected (mockup DeltaChange-mockup.html v41, Rassul 2026-11-27): 9:09 "Cause: Big bids built 3.3x => Effect: Big move by limit orders"; 11:57 Big bullish absorption;
8:33 Huge bearish absorption; 11:51 stalling (Huge bid icebergs 4.3x); 11:54 Huge flip up; 10:18 Strong bids absorbing; 10:36 Big move by limit orders;
context 9:09 "Moderate stop run below HVL with 2.2x burst"; 10:18 "Moderate local stop run below the 9:54 low with 0.8x burst".
