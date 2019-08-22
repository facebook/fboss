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

#include "fboss/agent/AgentConfig.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/Platform.h"
#include "fboss/agent/ThriftHandler.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/platforms/common/PlatformProductInfo.h"
#include "fboss/agent/platforms/sai/SaiPlatformPort.h"

#include <memory>

namespace facebook {
namespace fboss {

class SaiPlatform : public Platform {
 public:
  explicit SaiPlatform(std::unique_ptr<PlatformProductInfo> productInfo);
  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  void onInitialConfigApplied(SwSwitch* sw) override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;
  folly::MacAddress getLocalMac() const override;
  std::string getPersistentStateDir() const override;
  void getProductInfo(ProductInfo& info) override;
  std::string getVolatileStateDir() const override;
  TransceiverIdxThrift getPortMapping(PortID port) const override;
  virtual sai_service_method_table_t* getServiceMethodTable() const {
    return nullptr;
  }
  virtual std::vector<int8_t> getConnectionHandle() const;
  virtual SaiPlatformPort* getPort(PortID id) const;
  virtual PlatformMode getMode() const;
  PlatformPort* getPlatformPort(PortID port) const override;
  virtual void initPorts();

 private:
  void initImpl() override;
  std::unique_ptr<SaiSwitch> saiSwitch_;
  const std::unique_ptr<PlatformProductInfo> productInfo_;
  std::unordered_map<PortID, std::unique_ptr<SaiPlatformPort>> portMapping_;
};

} // namespace fboss
} // namespace facebook
