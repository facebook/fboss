namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace py neteng.fboss.bcm_test

include "common/fb303/if/fb303.thrift"
include "fboss/agent/hw/sai/switch/sai_ctrl.thrift"

typedef string (cpp.type = "::folly::fbstring") fbstring
typedef string (cpp.type = "::folly::fbstring") fbbinary

service SaiTestCtrl extends fb303.FacebookService {
  string, stream<string> startDiagShell()
    throws (1: fboss.FbossBaseError error)
  void produceDiagShellInput(1: string input,
      2: sai_ctrl.ClientInformation client)
    throws (1: fboss.FbossBaseError error)
}
