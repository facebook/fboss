// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/testing/TestUtil.h>
#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace facebook {
namespace fboss {

class CredoMacsecUtilTest : public ::testing::Test {
 public:
  CredoMacsecUtilTest() = default;
  ~CredoMacsecUtilTest() override = default;

  void SetUp() override;
  void TearDown() override;

  folly::test::TemporaryDirectory tmpDir = folly::test::TemporaryDirectory();

 private:
  // Forbidden copy constructor and assignment operator
  CredoMacsecUtilTest(CredoMacsecUtilTest const&) = delete;
  CredoMacsecUtilTest& operator=(CredoMacsecUtilTest const&) = delete;
};

} // namespace fboss
} // namespace facebook
