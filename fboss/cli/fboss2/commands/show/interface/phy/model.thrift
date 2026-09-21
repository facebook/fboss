package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/lib/phy/phy.thrift"

struct ShowInterfacePhyModel {
  1: map<string, map<phy.DataPlanePhyChipType, phy.PhyInfo>> phyInfo;
}
