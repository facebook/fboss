// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace facebook {
namespace fboss {

class QsfpUtilTest : public ::testing::Test {
 public:
  QsfpUtilTest() = default;
  ~QsfpUtilTest() override = default;

  void SetUp() override;
  void TearDown() override;

 private:
  // Forbidden copy constructor and assignment operator
  QsfpUtilTest(QsfpUtilTest const&) = delete;
  QsfpUtilTest& operator=(QsfpUtilTest const&) = delete;
};

} // namespace fboss
} // namespace facebook
