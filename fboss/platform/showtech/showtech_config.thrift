namespace cpp2 facebook.fboss.platform.showtech_config

// Configuration for the showtech utility
struct ShowtechConfig {
  1: set<string> i2cBusIgnore;
  2: list<string> psus;
  3: list<Gpio> gpios;
  4: list<Pem> pems;
}

struct Gpio {
  1: string path;
  2: list<GpioLine> lines;
}

struct GpioLine {
  1: string name;
  2: i32 lineIndex;
  3: list<Pem> pems;
}

struct Pem {
  1: string name;
  2: string presenceSysfsPath;
  3: string inputOkSysfsPath;
  4: string statusSysfsPath;
}
