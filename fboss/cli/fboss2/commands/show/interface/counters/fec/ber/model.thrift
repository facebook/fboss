package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/lib/phy/phy.thrift"

struct ShowInterfaceCountersFecBerModel {
  1: map<string, map<phy.PortComponent, double>> fecBer;
  10: phy.Direction direction;
  11: bool hasXphy;
}
