namespace cpp2 facebook.fboss
namespace go neteng.fboss.multinode_test_ctrl
namespace php fboss
namespace py neteng.fboss.multinode_test_ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.multinode_test_ctrl

include "fboss/agent/if/ctrl.thrift"

service MultiNodeTestCtrl extends ctrl.FbossCtrl {
  void triggerGracefulRestart();
  void triggerUngracefulRestart();
}
