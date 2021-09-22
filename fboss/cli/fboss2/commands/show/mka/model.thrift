namespace cpp2 facebook.fboss.cli

struct ShowMkaModel {
  1: map<string, list<MkaEntry>> portToMkaEntries;
}

struct MkaEntry {
  1: string srcMac;
  2: string ckn;
  3: bool keyServerElected;
  4: string sakRxInstalledSince;
  5: string sakTxInstalledSince;
  6: bool isSecondary;
}
