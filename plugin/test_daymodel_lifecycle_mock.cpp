// Mock-only lifecycle regression for DayModel.cpp.  This is not an Investor/RT runtime test.
// It proves that one DayModel object uses the active SDK user-data slot for each
// simulated chart and deletes only the active slot on done()/destroy().
#include <cstdio>

#include "DayModel.cpp"

static int passes = 0;
static int failures = 0;
#define CHECK(condition, message) do { \
    if (condition) { ++passes; std::printf("  ok    %s\n", message); } \
    else { ++failures; std::printf("  FAIL  %s (line %d)\n", message, __LINE__); } \
} while (0)

int main()
{
    std::printf("test_daymodel_lifecycle_mock — one object, two simulated chart contexts\n");
    DayModel model;
    void* chartA = 0;
    void* chartB = 0;

    model.mockSetCurrentUserDataSlot(&chartA);
    DayModelState* a = model.getState(true);
    CHECK(a != 0 && chartA == a, "chart A receives a state through setUserData");
    a->endStamped = true;
    a->cfg.width = 77;
    a->expC.valid = true;

    model.mockSetCurrentUserDataSlot(&chartB);
    DayModelState* b = model.getState(true);
    CHECK(b != 0 && chartB == b && b != a, "chart B receives a distinct state from the same DayModel object");
    CHECK(!b->endStamped && b->cfg.width == 46 && !b->expC.valid,
          "chart B starts with defaults, not chart A stamp/configuration/candle data");
    b->endStamped = false;
    b->cfg.width = 91;
    b->actC.valid = true;

    model.mockSetCurrentUserDataSlot(&chartA);
    CHECK(model.getState(false) == a && a->endStamped && a->cfg.width == 77 && a->expC.valid,
          "returning to chart A restores only chart A state");
    CHECK(model.done() == RTX_OK && chartA == 0,
          "done deletes and clears chart A state on a symbol or periodicity transition");
    CHECK(model.destroy() == RTX_OK && chartA == 0,
          "destroy after done is idempotent for an already-cleared chart A slot");

    model.mockSetCurrentUserDataSlot(&chartB);
    CHECK(model.getState(false) == b && !b->endStamped && b->cfg.width == 91 && b->actC.valid,
          "chart B survives chart A cleanup unchanged");
    CHECK(model.destroy() == RTX_OK && chartB == 0,
          "destroy deletes and clears chart B state");
    CHECK(model.getState(false) == 0,
          "no stale pointer remains after chart B destroy");

    std::printf("\n%d passed, %d failed\n", passes, failures);
    return failures;
}
