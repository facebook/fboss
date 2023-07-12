namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace py neteng.fboss.sai_ctrl
namespace py3 neteng.fboss

include "fboss/agent/if/fboss.thrift"
include "fboss/agent/if/common.thrift"
include "fboss/agent/if/hw_ctrl.thrift"

service SaiCtrl extends hw_ctrl.FbossHwCtrl {
  string, stream<string> startDiagShell() throws (
    1: fboss.FbossBaseError error,
  );
  void produceDiagShellInput(
    1: string input,
    2: common.ClientInformation client,
  ) throws (1: fboss.FbossBaseError error);
}
