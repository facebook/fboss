namespace cpp2 facebook.fboss
namespace go neteng.fboss.hw_ctrl
namespace php fboss
namespace py neteng.fboss.hw_ctrl
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.hw_ctrl

service FbossHwCtrl {
  // an api  to test hw switch handler in hardware agnostic way
  i32 echoI32(i32 input);
}
