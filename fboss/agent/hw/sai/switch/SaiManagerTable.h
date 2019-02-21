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

#include <memory>

namespace facebook {
namespace fboss {

class SaiApiTable;
class SaiBridgeManager;
class SaiPortManager;

class SaiManagerTable {
 public:
  explicit SaiManagerTable(SaiApiTable* apiTable);
  ~SaiManagerTable();
  SaiBridgeManager& bridgeManager();
  const SaiBridgeManager& bridgeManager() const;
  SaiPortManager& portManager();
  const SaiPortManager& portManager() const;

 private:
  std::unique_ptr<SaiBridgeManager> bridgeManager_;
  std::unique_ptr<SaiPortManager> portManager_;
  SaiApiTable* apiTable_;
};

} // namespace fboss
} // namespace facebook
