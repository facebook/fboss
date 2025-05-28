namespace cpp2 facebook.fboss.cli

struct ShowFabricInputBalanceModel {
  1: list<InputBalanceEntry> inputBalanceEntry;
}

struct InputBalanceEntry {
  1: string destinationSwitchName;
  2: list<string> sourceSwitchName;
  3: bool balanced;
  4: list<string> inputCapacity;
  5: list<string> outputCapacity;
}
