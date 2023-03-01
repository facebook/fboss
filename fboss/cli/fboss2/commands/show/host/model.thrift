namespace cpp2 facebook.fboss.cli

struct ShowHostModel {
  1: list<ShowHostModelEntry> hostEntries;
}

struct ShowHostModelEntry {
  1: string portName;
  2: i32 portID;
  3: string queueID;
  4: string hostName;
  5: string adminState;
  6: string linkState;
  7: string speed;
  8: string fecMode;
  9: i64 inErrors;
  10: i64 inDiscards;
  11: i64 outErrors;
  12: i64 outDiscards;
}
