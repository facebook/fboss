// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package "facebook.com/fboss/qsfp_service/test/hal_test"

namespace cpp2 facebook.fboss

struct HalTestTransceiverEntry {
  1: i32 id;
  2: string name;
  // Optional path overrides (used only when no platform is specified)
  3: optional string i2cDevicePath;
  4: optional string presentPath;
  5: optional string resetPath;
}

struct HalTestConfig {
  1: list<HalTestTransceiverEntry> transceivers;
}
