// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include <string>

#include "fboss/agent/platforms/common/utils/BcmYamlConfig.h"

namespace facebook::fboss {
namespace {

// Number of non-overlapping occurrences of needle in haystack.
int countOccurrences(const std::string& haystack, const std::string& needle) {
  int count = 0;
  for (auto pos = haystack.find(needle); pos != std::string::npos;
       pos = haystack.find(needle, pos + needle.size())) {
    ++count;
  }
  return count;
}

// Renders `config` and reparses it into a fresh BcmYamlConfig so a test can
// assert -- through the public getters -- that a property really landed under
// bcm_device.0.global and survives the emit/parse round trip.
bool renderedHas128ByteIpv6(BcmYamlConfig& config) {
  BcmYamlConfig reloaded;
  reloaded.setBaseConfig(config.getConfig());
  return reloaded.is128ByteIpv6Enabled();
}

// A global property with a public getter (is128ByteIpv6Enabled), used to verify
// setGlobalProperty round trips regardless of which branch created the section.
constexpr auto kGlobalProp = "ipv6_lpm_128b_enable";

} // namespace

// Branch A: base config already has a bcm_device.0.global section. The property
// is added under it and pre-existing globals are preserved.
TEST(BcmYamlConfigTest, setGlobalPropertyWithExistingGlobalSection) {
  BcmYamlConfig config;
  config.setBaseConfig(
      "bcm_device:\n"
      "  0:\n"
      "    global:\n"
      "      l3_alpm_template: 2\n");

  config.setGlobalProperty(kGlobalProp, "1");

  EXPECT_TRUE(renderedHas128ByteIpv6(config));
  // Pre-existing global (l3_alpm_template) is retained.
  EXPECT_TRUE(config.isAlpmEnabled());
  EXPECT_EQ(countOccurrences(config.getConfig(), "bcm_device"), 1);
}

// Branch B: base config has a bcm_device doc but no global section. A global
// section is created under the existing doc without duplicating bcm_device.
TEST(BcmYamlConfigTest, setGlobalPropertyWithDeviceButNoGlobalSection) {
  BcmYamlConfig config;
  config.setBaseConfig(
      "bcm_device:\n"
      "  0:\n"
      "    N3248TE: 1\n");

  config.setGlobalProperty(kGlobalProp, "1");

  EXPECT_TRUE(renderedHas128ByteIpv6(config));
  // Existing bcm_device doc is reused (not duplicated) and its content kept.
  EXPECT_EQ(countOccurrences(config.getConfig(), "bcm_device"), 1);
  EXPECT_EQ(countOccurrences(config.getConfig(), "N3248TE"), 1);
}

// Branch C: base config has no bcm_device doc at all. A new document holding
// bcm_device.0.global is appended while the existing document is preserved.
TEST(BcmYamlConfigTest, setGlobalPropertyWithNoDeviceDoc) {
  BcmYamlConfig config;
  config.setBaseConfig(
      "device:\n"
      "  0:\n"
      "    TM_THD_CONFIG:\n"
      "      THRESHOLD_MODE: LOSSY\n");

  config.setGlobalProperty(kGlobalProp, "1");

  EXPECT_TRUE(renderedHas128ByteIpv6(config));
  EXPECT_EQ(countOccurrences(config.getConfig(), "bcm_device"), 1);
  // Original device document is retained.
  EXPECT_EQ(config.getMmuState(), "LOSSY");
}

// Re-setting a property to its current value is a no-op: the rendered config is
// byte-for-byte unchanged (dirty_ is not tripped by a redundant write), while a
// real change re-emits and takes effect.
TEST(BcmYamlConfigTest, setGlobalPropertyOnlyReemitsOnChange) {
  BcmYamlConfig config;
  config.setBaseConfig(
      "bcm_device:\n"
      "  0:\n"
      "    global:\n"
      "      l3_alpm_template: 2\n");

  config.setGlobalProperty(kGlobalProp, "1");
  const auto rendered = config.getConfig();

  config.setGlobalProperty(kGlobalProp, "1");
  EXPECT_EQ(config.getConfig(), rendered);

  config.setGlobalProperty(kGlobalProp, "0");
  EXPECT_NE(config.getConfig(), rendered);
  EXPECT_FALSE(renderedHas128ByteIpv6(config));
}

// Mirrors the actual --bcm_sdk_log_file use: a string path value is injected
// verbatim into the rendered config.
TEST(BcmYamlConfigTest, setGlobalPropertyInjectsStringPathValue) {
  BcmYamlConfig config;
  config.setBaseConfig(
      "bcm_device:\n"
      "  0:\n"
      "    global:\n"
      "      l3_alpm_template: 2\n");

  config.setGlobalProperty(
      "sai_preinit_cmd_file", "/tmp/enable_bcm_debugs.soc");

  const auto rendered = config.getConfig();
  EXPECT_NE(rendered.find("sai_preinit_cmd_file"), std::string::npos);
  EXPECT_NE(rendered.find("/tmp/enable_bcm_debugs.soc"), std::string::npos);
}

} // namespace facebook::fboss
