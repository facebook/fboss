// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>

#include "fboss/platform/platform_manager/CpldManager.h"
#include "fboss/platform/platform_manager/uapi/fbcpld-ioctl.h"

using namespace facebook::fboss::platform::platform_manager;

namespace {

CpldSysfsAttr makeCpldSysfsAttr(
    const std::string& name,
    const std::string& mode,
    const std::string& reg,
    int32_t bitOffset,
    int32_t numBits,
    const std::vector<std::string>& flags,
    const std::string& description) {
  CpldSysfsAttr attr;
  attr.name() = name;
  attr.mode() = mode;
  attr.regAddr() = reg;
  attr.bitOffset() = bitOffset;
  attr.numBits() = numBits;
  attr.flags() = flags;
  attr.description() = description;
  return attr;
}

} // namespace

TEST(CpldManagerTest, GetCpldCharDevPath) {
  EXPECT_EQ(getCpldCharDevPath(5, I2cAddr("0x3e")), "/dev/fbcpld-5-003e");
  EXPECT_EQ(getCpldCharDevPath(0, I2cAddr("0x10")), "/dev/fbcpld-0-0010");
}

TEST(CpldManagerTest, BuildIoctlRequestBasicFields) {
  auto attrs = {makeCpldSysfsAttr(
      "board_id", "ro", "0x10", 0, 4, {}, "Board identifier")};

  auto request = buildCpldIoctlRequest(attrs);

  EXPECT_EQ(request.num_attrs, 1);
  EXPECT_STREQ(request.attrs[0].name, "board_id");
  EXPECT_EQ(request.attrs[0].mode, 0444);
  EXPECT_EQ(request.attrs[0].reg_addr, 0x10);
  EXPECT_EQ(request.attrs[0].bit_offset, 0);
  EXPECT_EQ(request.attrs[0].num_bits, 4);
  EXPECT_EQ(request.attrs[0].flags, 0);
  EXPECT_STREQ(request.attrs[0].help_str, "Board identifier");
}

TEST(CpldManagerTest, BuildIoctlRequestModeRw) {
  auto attrs = {makeCpldSysfsAttr("led", "rw", "0x20", 0, 1, {}, "LED ctrl")};

  auto request = buildCpldIoctlRequest(attrs);

  EXPECT_EQ(request.attrs[0].mode, 0644);
}

TEST(CpldManagerTest, BuildIoctlRequestModeWo) {
  auto attrs = {makeCpldSysfsAttr("reset", "wo", "0x30", 0, 1, {}, "Reset")};

  auto request = buildCpldIoctlRequest(attrs);

  EXPECT_EQ(request.attrs[0].mode, 0200);
}

TEST(CpldManagerTest, BuildIoctlRequestFlags) {
  auto attrs = {makeCpldSysfsAttr(
      "psu_present",
      "ro",
      "0x40",
      2,
      1,
      {"negate", "log_write"},
      "PSU presence")};

  auto request = buildCpldIoctlRequest(attrs);

  EXPECT_EQ(
      request.attrs[0].flags, FBCPLD_FLAG_VAL_NEGATE | FBCPLD_FLAG_LOG_WRITE);
}

TEST(CpldManagerTest, BuildIoctlRequestAllFlags) {
  auto attrs = {makeCpldSysfsAttr(
      "test",
      "ro",
      "0x01",
      0,
      1,
      {"log_write", "show_notes", "decimal", "negate"},
      "all flags")};

  auto request = buildCpldIoctlRequest(attrs);

  EXPECT_EQ(
      request.attrs[0].flags,
      FBCPLD_FLAG_LOG_WRITE | FBCPLD_FLAG_SHOW_NOTES | FBCPLD_FLAG_SHOW_DEC |
          FBCPLD_FLAG_VAL_NEGATE);
}

TEST(CpldManagerTest, BuildIoctlRequestMultipleAttrs) {
  auto attrs = {
      makeCpldSysfsAttr("board_id", "ro", "0x00", 0, 8, {}, "Board ID"),
      makeCpldSysfsAttr(
          "cpld_ver", "ro", "0x01", 0, 8, {"decimal"}, "CPLD version"),
      makeCpldSysfsAttr(
          "psu_present", "ro", "0x10", 4, 1, {"negate"}, "PSU presence"),
  };

  auto request = buildCpldIoctlRequest(attrs);

  EXPECT_EQ(request.num_attrs, 3);

  EXPECT_STREQ(request.attrs[0].name, "board_id");
  EXPECT_EQ(request.attrs[0].reg_addr, 0x00);
  EXPECT_EQ(request.attrs[0].num_bits, 8);
  EXPECT_EQ(request.attrs[0].flags, 0);

  EXPECT_STREQ(request.attrs[1].name, "cpld_ver");
  EXPECT_EQ(request.attrs[1].reg_addr, 0x01);
  EXPECT_EQ(request.attrs[1].flags, FBCPLD_FLAG_SHOW_DEC);

  EXPECT_STREQ(request.attrs[2].name, "psu_present");
  EXPECT_EQ(request.attrs[2].reg_addr, 0x10);
  EXPECT_EQ(request.attrs[2].bit_offset, 4);
  EXPECT_EQ(request.attrs[2].num_bits, 1);
  EXPECT_EQ(request.attrs[2].flags, FBCPLD_FLAG_VAL_NEGATE);
}

TEST(CpldManagerTest, BuildIoctlRequestHexRegAddr) {
  auto attrs = {
      makeCpldSysfsAttr("low", "ro", "0x00", 0, 1, {}, ""),
      makeCpldSysfsAttr("mid", "ro", "0x7f", 0, 1, {}, ""),
      makeCpldSysfsAttr("high", "ro", "0xFF", 0, 1, {}, ""),
  };

  auto request = buildCpldIoctlRequest(attrs);

  EXPECT_EQ(request.attrs[0].reg_addr, 0x00);
  EXPECT_EQ(request.attrs[1].reg_addr, 0x7f);
  EXPECT_EQ(request.attrs[2].reg_addr, 0xFF);
}
