// syntax-check wrapper only: MinGW defines __GNUC__, whose irtsdk.h branch (Mac/XCode) lacks RT_LONG; take the Windows branch.
#define NOMINMAX
#include <windows.h>
#pragma push_macro("__GNUC__")
#undef __GNUC__
#include "irtsdk.h"
#pragma pop_macro("__GNUC__")
#include "../SweepRefill.cpp"
