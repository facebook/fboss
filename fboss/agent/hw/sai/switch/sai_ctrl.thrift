namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace py neteng.fboss.sai_ctrl

include "fboss/agent/if/ctrl.thrift"

typedef string (cpp.type = "::folly::fbstring") fbstring
typedef string (cpp.type = "::folly::fbstring") fbbinary

struct ClientInformation {
  1: optional fbstring username,
  2: optional fbstring hostname,
}

service SaiCtrl extends ctrl.FbossCtrl {
  string, stream<string> startDiagShell()
    throws (1: fboss.FbossBaseError error)
  void produceDiagShellInput(1: string input, 2: ClientInformation client)
    throws (1: fboss.FbossBaseError error)
}
