// syntax-check wrapper only (MinGW + the REAL irtsdk.h, Windows branch)
#define NOMINMAX
#include <windows.h>
#pragma push_macro("__GNUC__")
#undef __GNUC__
#include "irtsdk.h"
#pragma pop_macro("__GNUC__")
#include "../Trapped.cpp"
