namespace cpp2 facebook.fboss.led
namespace py neteng.fboss.led.led_structs
namespace py3 neteng.fboss.led
namespace py.asyncio neteng.fboss.asyncio.led_structs
namespace go neteng.fboss.led.led_structs

enum LedColor {
  UNKNOWN = 0x0,
  OFF = 0x1,
  WHITE = 0x2,
  RED = 0x3,
  ORANGE = 0x4,
  PINK = 0x5,
  BLUE = 0x6,
  MAGENTA = 0x7,
  CYAN = 0x8,
  GREEN = 0x9,
  YELLOW = 0xA,
}

enum Blink {
  UNKNOWN = 0x0,
  OFF = 0x1,
  SLOW = 0x2,
  FAST = 0x3,
}

struct LedState {
  1: LedColor ledColor;
  2: Blink blink;
}

struct PortLedState {
  1: i16 swPortId;
  2: string swPortName;
  3: LedState currentLedState;
  4: bool forcedOnState;
  5: bool forcedOffState;
}
