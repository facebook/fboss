namespace cpp2 facebook.fboss
namespace go neteng.fboss.hw_ctrl
namespace php fboss
namespace py neteng.fboss.hw_ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.hw_ctrl

include "fboss/agent/if/common.thrift"
include "fboss/agent/if/fboss.thrift"

service FbossHwCtrl {
  /*
   * Enables submitting diag cmds to the switch
   */
  common.fbstring diagCmd(
    1: common.fbstring cmd,
    2: common.ClientInformation client,
    3: i16 serverTimeoutMsecs = 0,
    4: bool bypassFilter = false,
  );

  /*
   * Get formatted string for diag cmd filters configuration
   */
  common.fbstring cmdFiltersAsString() throws (1: fboss.FbossBaseError error);

  // an api  to test hw switch handler in hardware agnostic way
  common.SwitchRunState getHwSwitchRunState();
}
