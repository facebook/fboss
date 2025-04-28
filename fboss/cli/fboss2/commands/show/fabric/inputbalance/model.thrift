namespace cpp2 facebook.fboss.cli

struct ShowFabricInputBalanceModel {
  1: list<InputBalanceEntry> inputBalanceEntry;
}

struct InputBalanceEntry {
  1: string destinationSwitchName;
  2: bool balanced;
  3: InputBalanceScope scope;
  4: list<InputBalanceEntry> inputCapacity;
  5: list<string> outputCapacity;
}

enum InputBalanceScope {
  LOCAL = 0,
  GLOBAL = 1,
}

struct InputCapacityEntry {
  1: string switchName;
  2: list<string> ports;
}
