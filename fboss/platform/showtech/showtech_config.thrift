namespace cpp2 facebook.fboss.platform.showtech_config

// Configuration for the showtech utility
struct ShowtechConfig {
  1: set<string> i2cBusIgnore;
  2: list<string> psus;
  3: list<Gpio> gpios;
  4: list<Pem> pems;
  5: list<FanSpinnerDevice> fanspinners;
  6: optional SwitchCardPowerGoodStatus sc_powergood;
}

struct Gpio {
  1: string path;
  2: list<GpioLine> lines;
}

struct GpioLine {
  1: string name;
  2: i32 lineIndex;
}

struct Pem {
  1: string name;
  2: string presenceSysfsPath;
  3: string inputOkSysfsPath;
  4: string statusSysfsPath;
}

struct FanSpinnerDevice {
  1: string path;
  2: list<SysfsAttribute> sysfsAttributes;
}

struct SwitchCardPowerGoodStatus {
  1: optional SysfsAttribute sysfsAttribute;
  2: optional GpioAttribute gpioAttribute;
}

struct SysfsAttribute {
  1: string name;
  2: string path;
}

struct GpioAttribute {
  1: string name;
  2: string path;
  3: i32 lineIndex;
}
