/********************************************************************************
 * DealerProfileAuditLogic.h -- small, SDK-free boundaries owned by DealerProfile.
 *
 * This is deliberately not part of DealerLogic.h: MenthorQ/Touch/Levels inputs and
 * export temporary naming are used only by DealerProfile.
 ********************************************************************************/
#pragma once

#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>

namespace dpaudit {

inline bool finite(float v) { return std::isfinite((double)v) != 0; }
inline bool finite(double v) { return std::isfinite(v) != 0; }

// CSV fields may have surrounding whitespace, but no partial, non-finite,
// underflowed/overflowed, zero, or negative price is meaningful to drawing.
inline bool parsePositiveFinite(const std::string& text, float& out)
{
    const char* first = text.c_str();
    while (*first && std::isspace((unsigned char)*first)) ++first;
    if (!*first) return false;
    errno = 0;
    char* end = 0;
    const float value = std::strtof(first, &end);
    if (end == first || errno == ERANGE || !finite(value)) return false;
    while (*end && std::isspace((unsigned char)*end)) ++end;
    if (*end || value <= 0.0f) return false;
    out = value;
    return true;
}

// User data is scoped by the IRT host to the current chart. Its identity keeps
// two simultaneous chart exports from sharing one fixed ".tmp" pathname.
inline std::string exportTempPath(const std::string& target, std::uintptr_t stateIdentity)
{
    char suffix[5 + sizeof(std::uintptr_t) * 2 + 1] = {0};
    std::snprintf(suffix, sizeof(suffix), ".tmp.%llX", static_cast<unsigned long long>(stateIdentity));
    return target + suffix;
}

} // namespace dpaudit
