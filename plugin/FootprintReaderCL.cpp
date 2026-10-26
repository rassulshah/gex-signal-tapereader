// FootprintReaderCL.dll - the Footprint Reader for CL only (0.5.1, 2026-10-05): one DLL per market, so IRT gives each its own object and timer.
// All the code is in IRTReader.cpp - edit that, never this file.
#define IR_FIXED "CL"
#define LSPERF_PLUGIN "FootprintReaderCL"   // (PerfGuard 1.0.0) this DLL's name in the perf records
#include "IRTReader.cpp"
