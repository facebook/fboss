package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowFabricInputBalanceModel {
  1: list<InputBalanceEntry> inputBalanceEntry;
}

struct InputBalanceEntry {
  1: string destinationSwitchName;
  2: list<string> sourceSwitchName;
  3: i16 virtualDeviceID;
  4: bool balanced;
  5: list<string> inputCapacity;
  6: list<string> outputCapacity;
  7: list<string> inputLinkFailure;
  8: list<string> outputLinkFailure;
}
