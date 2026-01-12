include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

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
  # Same for Minipack / Fuji PIM, which uses a different scratchpad counter.
  PIM_MINIPACK_SCRATCHPAD_COUNTER_NOT_CLEARED = 1,
  # PIM fails getPimType() check. This is a good indicator that communication with PIM is not working as expected.
  PIM_GET_TYPE_FAILED = 2,
}

struct PimState {
  1: list<PimError> errors;
}
