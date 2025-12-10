package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/lib/phy/phy.thrift"

struct ShowInterfaceCountersFecTailModel {
  1: map<string, map<phy.PortComponent, i16>> fecTail;
  10: phy.Direction direction;
  11: bool hasXphy;
}
