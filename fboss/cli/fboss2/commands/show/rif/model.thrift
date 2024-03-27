namespace cpp2 facebook.fboss.cli

struct ShowRifModel {
  1: list<RifEntry> rifs;
}

struct RifEntry {
  1: string name;
}
