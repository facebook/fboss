// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package "facebook.com/fboss/configs/platforms/generic/forwarding_stacks"

namespace cpp2 facebook.fboss.configgen

struct FeatureConditionValues {
  // An empty included set matches every value not listed in excluded.
  1: set<string> included = [];
  2: set<string> excluded = [];
}

struct FeatureEnableConditions {
  // Populated condition dimensions are ANDed together.
  1: optional FeatureConditionValues configProfiles;
  // Values must be cfg::AsicType enum names from agent/switch_config.thrift,
  // for example "ASIC_TYPE_TOMAHAWK5".
  2: optional FeatureConditionValues asicTypes;
  // Values must be PlatformType enum names from lib/if/fboss_common.thrift,
  // for example "PLATFORM_WEDGE800BACT".
  3: optional FeatureConditionValues platforms;
}

struct FeatureDefaultCommandArgs {
  1: map<string, string> args = {};
  // When absent, this feature is explicit-only and is not automatically
  // enabled. When present, at least one condition must be configured.
  2: optional FeatureEnableConditions autoEnableWhen;
}

struct FeatureDefaultCommandArgsConfig {
  1: map<string, map<string, string>> profileDefaultArgs = {};
  2: map<string, FeatureDefaultCommandArgs> features = {};
}
