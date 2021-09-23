namespace cpp2 facebook.fboss.cli

include "fboss/mka_service/if/mka.thrift"

struct ShowMkaModel {
  1: map<string, MkaEntry> portToMkaEntry;
}

typedef mka.MKAPeerInfo MkaPeer

struct MkaProfile {
  1: string srcMac;
  2: string ckn;
  3: bool keyServerElected;
  4: string sakRxInstalledSince;
  5: string sakTxInstalledSince;
  6: list<MkaPeer> activePeers;
  7: list<MkaPeer> potentialPeers;
  8: string sakRotatedAt;
}

struct MkaEntry {
  1: MkaProfile primaryProfile;
  2: optional MkaProfile secondaryProfile;
  3: string encryptedSak;
}
