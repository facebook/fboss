namespace cpp2 facebook.fboss
namespace go neteng.fboss.test_ctrl
namespace php fboss
namespace py neteng.fboss.test_ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.test_ctrl

include "fboss/agent/if/ctrl.thrift"

service TestCtrl extends ctrl.FbossCtrl {
  void gracefullyRestartService(1: string serviceName);

  void ungracefullyRestartService(1: string serviceName);

  // When Agent is stopped, Test server will exit.
  // Impleneting Agent start will then require the client to login to the
  // device or need a separate thrift server.
  // We avoid it by implementing restartWithDelay API as below:
  //     - Schedule a restart of Agent after delayInSeconds.
  //     - Stop Agent
  void gracefullyRestartServiceWithDelay(
    1: string serviceName,
    2: i32 delayInSeconds,
  );
}
