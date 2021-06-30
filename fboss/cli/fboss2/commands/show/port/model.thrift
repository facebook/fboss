namespace cpp2 facebook.fboss.cli

struct ShowPortModel {
  1: list<PortEntry> portEntries;
}

struct PortEntry {
  1: i32 id;
  2: string name;
  3: string adminState;
  4: string operState;
  5: string speed;
  6: string profileId;
}
