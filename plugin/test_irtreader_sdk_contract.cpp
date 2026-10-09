// Compile this source through windows_sdk_check.py against the supplied actual SDK.
#include <type_traits>
#include <utility>
#include "irtsdk.h"

static_assert(std::is_same<decltype(cppExtension::VOLPROFILE().buyVolume), long>::value,
              "VOLPROFILE buyVolume is signed long");
static_assert(std::is_same<decltype(cppExtension::VOLPROFILE().sellVolume), long>::value,
              "VOLPROFILE sellVolume is signed long");
static_assert(std::is_same<decltype(cppExtension::VOLPROFILE().totalVolume), long>::value,
              "VOLPROFILE totalVolume is signed long");
static_assert(std::is_same<decltype(std::declval<cppExtension::RTARRAYI&>()[0]), unsigned long&>::value,
              "RTARRAYI volume elements are unsigned long");
int main() { return 0; }
