// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package "facebook.com/fboss/lib/platforms"

include "fboss/agent/switch_config.thrift"
include "fboss/lib/if/fboss_common.thrift"

namespace cpp2 facebook.fboss
namespace py neteng.fboss.platform_descriptor
namespace py3 neteng.fboss

struct PlatformDescriptor {
  1: fboss_common.PlatformType platformType;
  2: list<string> productNamePrefixes;
  3: list<string> modeNames;
  4: switch_config.AsicType asicType;
  5: map<string, bool> variantAttributes;
}
