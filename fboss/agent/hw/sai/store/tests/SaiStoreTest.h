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

#include "fboss/agent/hw/sai/fake/FakeSai.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"

#include <gtest/gtest.h>

namespace facebook::fboss {

class SaiStoreTest : public ::testing::Test {
 public:
  void SetUp() override {
    fs = FakeSai::getInstance();
    saiApiTable = SaiApiTable::getInstance();
    saiApiTable->queryApis(nullptr, saiApiTable->getFullApiList());
    saiStore = std::make_unique<SaiStore>();
  }
  template <typename Traits, typename... Args>
  SaiObject<Traits> createObj(Args&&... args) {
    return SaiObject<Traits>(std::forward<Args>(args)...);
  }
  std::shared_ptr<FakeSai> fs;
  std::shared_ptr<SaiApiTable> saiApiTable;
  std::unique_ptr<SaiStore> saiStore;
};

template <typename SaiObjectTraits>
void verifyAdapterKeySerDeser(
    const std::vector<typename SaiObjectTraits::AdapterKey>& keys,
    bool subsetOk = false) {
  SaiStore s(0);
  s.reload();
  auto json = s.adapterKeysFollyDynamic();
  auto gotAdaterKeys = keysForSaiObjStoreFromStoreJson<SaiObjectTraits>(json);
  EXPECT_TRUE(keys.size() == gotAdaterKeys.size() || subsetOk);
  for (auto key : keys) {
    auto itr = std::find(gotAdaterKeys.begin(), gotAdaterKeys.end(), key);
    EXPECT_TRUE(itr != gotAdaterKeys.end());
  }
}

template <typename SaiObjectTraits>
void verifyToStr() {
  SaiStore s(0);
  s.reload();
  auto& store = s.get<SaiObjectTraits>();
  auto str = fmt::format("{}", store);
  EXPECT_EQ(std::count(str.begin(), str.end(), '\n'), store.size() + 1);
}

} // namespace facebook::fboss
