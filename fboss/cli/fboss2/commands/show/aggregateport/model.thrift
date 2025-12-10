package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

struct ShowAggregatePortModel {
  1: list<AggregatePortEntry> aggregatePortEntries;
}

struct AggregatePortEntry {
  1: string name;
  2: string description;
  3: i32 activeMembers;
  4: i32 configuredMembers;
  5: i32 minMembers;
  6: list<AggregateMemberPortEntry> members;
  7: optional i32 minMembersToUp;
}

struct AggregateMemberPortEntry {
  1: string name;
  2: i32 id;
  3: bool isUp;
  4: string lacpRate;
}
