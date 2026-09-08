// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package "facebook.com/fboss/lib/platforms"

include "fboss/agent/switch_config.thrift"
include "fboss/lib/if/fboss_common.thrift"

namespace cpp2 facebook.fboss
namespace py neteng.fboss.platform_descriptor
namespace py3 neteng.fboss

// Matches a hardware revision by the chassis EEPROM version fields
// (Meta EEPROM v6: Production State Type 8, Production Sub-State Type 9,
// Re-Spin/Variant Indicator Type 10). An unset field matches any value.
// Same semantics as platform_manager's VersionedPmUnitConfig.pmUnitVersions,
// applied to platform descriptors.
struct PmUnitVersionMatch {
  1: optional i16 productionState;
  2: optional i16 productionSubState;
  3: optional i16 respinVariantIndicator;
}

struct PlatformDescriptor {
  1: fboss_common.PlatformType platformType;
  2: list<string> productNamePrefixes;
  3: list<string> modeNames;
  4: switch_config.AsicType asicType;
  5: map<string, bool> variantAttributes;
  // Number of physical switching ASICs in the platform.
  6: i16 numSwitchAsics = 1;
  // When set, this descriptor applies only to systems whose chassis EEPROM
  // matches one of the listed versions. Descriptors without any selector act
  // as the default for their platformType.
  7: optional list<PmUnitVersionMatch> pmUnitVersions;
}
