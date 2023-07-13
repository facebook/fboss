namespace cpp2 facebook.fboss.led_service
namespace py neteng.fboss.led.led_service
namespace py3 neteng.fboss.led
namespace py.asyncio neteng.fboss.asyncio.led_service
namespace go neteng.fboss.led.led_service

include "fboss/agent/if/fboss.thrift"
include "fboss/agent/if/ctrl.thrift"

service LedService {
  void setExternalLedState(
    1: i32 portNum,
    2: ctrl.PortLedExternalState ledState,
  ) throws (1: fboss.FbossBaseError error);
}
