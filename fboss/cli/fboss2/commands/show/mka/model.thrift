namespace cpp2 facebook.fboss.cli

struct ShowMkaModel {
  1: list<MkaEntry> mkaEntries;
}

struct MkaEntry {
  1: string localPort;
  2: string srcMac;
  3: string ckn;
  4: bool keyServerElected;
  5: string sakRxInstalledSince;
  6: string sakTxInstalledSince;
}
