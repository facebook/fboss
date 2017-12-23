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

#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/types.h"
#include "fboss/agent/StateObserver.h"
#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/agent/platforms/wedge/WedgePortMapping.h"
#include "fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <boost/container/flat_map.hpp>
#include <memory>
#include <unordered_map>

DECLARE_bool(enable_routes_in_host_table);

namespace facebook { namespace fboss {

class BcmSwitch;
class WedgePort;

class WedgePlatform : public BcmPlatform, public StateObserver {
 public:
  explicit WedgePlatform(std::unique_ptr<WedgeProductInfo> productInfo);
  ~WedgePlatform() override;

  void init();
  InitPortMap initPorts() override;

  void stateUpdated(const StateDelta& /*delta*/) override;

  virtual std::unique_ptr<WedgePortMapping> createPortMapping() = 0;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  folly::MacAddress getLocalMac() const override;
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;

  void onUnitAttach(int unit) override;
  void getProductInfo(ProductInfo& info) override;

  bool canUseHostTableForHostRoutes() const override {
    return FLAGS_enable_routes_in_host_table;
  }
  WedgePort* getPort(PortID id) const;
  WedgePort* getPort(TransceiverID id) const;
  TransceiverIdxThrift getPortMapping(PortID port) const override;
  PlatformPort* getPlatformPort(PortID id) const override;

  uint32_t getMMUBufferBytes() const override {
    // All wedge platforms have 16MB MMU buffer
    return 16 * 1024 * 1024;
  }
  uint32_t getMMUCellBytes() const override {
    // All wedge platforms have 208 byte cells
    return 208;
  }
  bool isCosSupported() const override {
// TODO: add support for CoS queue traverse in FakeSdk
// (see D5644650 that broke FakeSdk build)
#ifdef FAKE_SDK
    return false;
#else
    return true;
#endif
  }

  QsfpCache* getQsfpCache() const {
    return qsfpCache_.get();
  }

 protected:
  WedgePlatformMode getMode() const;
  std::unique_ptr<WedgePortMapping> portMapping_{nullptr};

 private:
  // TODO get rid of this once T21721301 is done
  bool isDu() const;
  // Forbidden copy constructor and assignment operator
  WedgePlatform(WedgePlatform const &) = delete;
  WedgePlatform& operator=(WedgePlatform const &) = delete;

  void initLocalMac();
  void initLEDs();
  virtual std::map<std::string, std::string> loadConfig();

  virtual std::unique_ptr<BaseWedgeI2CBus> getI2CBus();

  virtual folly::ByteRange defaultLed0Code() = 0;
  virtual folly::ByteRange defaultLed1Code() = 0;

  folly::MacAddress localMac_;
  std::unique_ptr<BcmSwitch> hw_;

  const std::unique_ptr<WedgeProductInfo> productInfo_;
  const std::unique_ptr<QsfpCache> qsfpCache_;
};

}} // namespace facebook::fboss
