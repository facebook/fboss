namespace cpp facebook.fboss
namespace cpp2 facebook.fboss
namespace py neteng.fboss.hw_test_ctrl
namespace py3 neteng.fboss

include "common/fb303/if/fb303.thrift"
include "fboss/agent/if/ctrl.thrift"
include "fboss/agent/if/fboss.thrift"

service HwTestCtrl extends fb303.FacebookService {
  ctrl.fbstring diagCmd(
      1: ctrl.fbstring cmd,
      2: ctrl.ClientInformation client,
      3: i16 serverTimeoutMsecs = 0
  ) throws (1: fboss.FbossBaseError error)
}
