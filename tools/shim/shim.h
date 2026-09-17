#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstdint>
typedef uint64_t RT_LONG;
static inline int sprintf_s(char* b, size_t n, const char* f, ...){ va_list a; va_start(a,f); int r=vsnprintf(b,n,f,a); va_end(a); return r; }
static inline int localtime_s(struct tm* t, const time_t* tt){ struct tm* r=localtime(tt); if(r) *t=*r; return r?0:1; }
