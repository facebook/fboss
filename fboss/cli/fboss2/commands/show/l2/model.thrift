namespace cpp2 facebook.fboss.cli

struct ShowL2Model {
  1: list<ShowL2ModelEntry> l2Entries;
}

struct ShowL2ModelEntry {
  1: string mac;
  2: i32 port;
  3: i32 vlanID;
  4: string trunk;
  5: string type;
  6: string classID;
}
