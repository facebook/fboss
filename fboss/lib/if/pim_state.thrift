namespace cpp2 facebook.fboss
namespace go neteng.fboss.pim_state
namespace php fboss_common
namespace py neteng.fboss.pim_state
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.pim_state

enum PimError {
  # SCD counter is explicitly cleared by SW during PIM initialization at service startup.
  # So if the counter is detected as not cleared, it means PIM was reseated without restarting the service
  PIM_SCD_COUNTER_NOT_CLEARED = 0,
}

struct PimState {
  1: list<PimError> errors;
}
