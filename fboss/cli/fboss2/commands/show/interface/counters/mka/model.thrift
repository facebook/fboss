package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/agent/hw/hardware_stats.thrift"

struct InterfaceCountersMKAModel {
  1: map<string, hardware_stats.MacsecStats> intfMKAStats;
}
