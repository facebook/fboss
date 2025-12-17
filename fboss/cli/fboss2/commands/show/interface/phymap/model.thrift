package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

include "fboss/mka_service/if/mka_structs.thrift"

struct InterfacePhymapModel {
  1: mka_structs.MacsecPortPhyMap portsPhyMap;
}
