/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/types.h"

#include <string>

#include <gtest/gtest.h>

namespace facebook {
namespace fboss {

class ManagerTestBase : public ::testing::Test {
 public:
  void SetUp() override;

  sai_object_id_t
  addPort(const PortID& swId, const std::string& name, bool enabled);

  std::shared_ptr<FakeSai> fs;
  std::unique_ptr<SaiApiTable> saiApiTable;
  std::unique_ptr<SaiManagerTable> saiManagerTable;
};

} // namespace fboss
} // namespace facebook
