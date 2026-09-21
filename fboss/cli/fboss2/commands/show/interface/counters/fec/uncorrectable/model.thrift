package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/lib/phy/phy.thrift"

struct uncorrectableCount {
  1: i64 totalCount;
  2: i64 tenMinuteCount;
}

struct ShowInterfaceCountersFecUncorrectableModel {
  1: map<
    string,
    map<phy.PortComponent, uncorrectableCount>
  > uncorrectableFrames;
  10: phy.Direction direction;
}
