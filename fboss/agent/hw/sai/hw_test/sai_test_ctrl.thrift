namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace py neteng.fboss.sai_test_ctrl
namespace py3 neteng.fboss

include "fboss/agent/hw/hw_test_ctrl.thrift"
include "fboss/agent/if/ctrl.thrift"
include "fboss/agent/if/fboss.thrift"

service SaiTestCtrl extends hw_test_ctrl.HwTestCtrl {
  string, stream<string> startDiagShell()
    throws (1: fboss.FbossBaseError error)
  void produceDiagShellInput(1: string input,
      2: ctrl.ClientInformation client)
    throws (1: fboss.FbossBaseError error)
}
