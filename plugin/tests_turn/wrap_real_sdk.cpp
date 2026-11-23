// tests_turn/wrap_real_sdk.cpp -- syntax check of Turn.cpp against the REAL irtsdk.h (not the mock), C++14 as the PC's compiler default.
// g++ -std=c++14 -fsyntax-only -Wall -Wextra -I <sdk>/c++/include -I.. wrap_real_sdk.cpp
// On Linux the SDK header takes its GNU branch, which leaves RT_LONG undefined: defined here. (With MinGW: NOMINMAX + windows.h first.)
#if !defined(_WIN32)
typedef unsigned long long RT_LONG;
#endif
#include "../Turn.cpp"
int main() { return 0; }
