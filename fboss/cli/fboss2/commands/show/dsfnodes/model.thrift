namespace cpp2 facebook.fboss.cli

struct ShowDsfNodesModel {
  1: list<DsfNodeEntry> dsfNodes;
}

struct DsfNodeEntry {
  1: string name;
  2: i64 switchId;
  3: string type;
  4: string systemPortRange;
}
