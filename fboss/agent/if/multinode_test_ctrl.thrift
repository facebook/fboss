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

  // When Agent is stopped, MultiNodeTest server will exit.
  // Impleneting Agent start will then require the client to login to the
  // device or need a separate thrift server.
  // We avoid it by implementing restartWithDelay API as below:
  //     - Schedule a restart of Agent after delayInSeconds.
  //     - Stop Agent
  void restartWithDelay(i32 delayInSeconds);
}
