// Copyright 2021-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/qsfp_service/TransceiverManager.h"

#include <gtest/gtest.h>

namespace facebook {
namespace fboss {

class TransceiverManagerTest : public ::testing::Test {
 public:
  static constexpr int numModules = 4;
  static constexpr int numPortsPerModule = 4;

  void SetUp() override;
  void TearDown() override {}

 protected:
  std::unique_ptr<TransceiverManager> transceiverManager_;
};

} // namespace fboss
} // namespace facebook
