package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/agent/switch_config.thrift"

struct InterfaceCapabilitiesModel {
  1: map<string, list<switch_config.PortProfileID>> portProfiles;
}
