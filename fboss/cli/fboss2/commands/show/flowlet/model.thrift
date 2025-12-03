package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowEcmpEntryModel {
  1: list<EcmpEntry> ecmpEntries;
}

struct EcmpEntry {
  1: i32 ecmpId;
  2: bool flowletEnabled;
  3: i16 flowletInterval;
  4: i32 flowletTableSize;
}
