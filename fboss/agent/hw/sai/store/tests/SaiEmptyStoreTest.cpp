/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/store/tests/SaiStoreTest.h"

TEST_F(SaiStoreTest, ctor) {
  SaiStore s(0);
}

TEST_F(SaiStoreTest, loadEmpty) {
  SaiStore s(0);
  s.reload();
}

TEST_F(SaiStoreTest, singletonLoadEmpty) {
  auto store = SaiStore::getInstance();
  store->setSwitchId(0);
  store->reload();
}
