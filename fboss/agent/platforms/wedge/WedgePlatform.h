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
#include "fboss/agent/platforms/wedge/WedgeProductInfo.h"
#include "fboss/agent/platforms/wedge/WedgeI2CBusLock.h"

#include <folly/MacAddress.h>
#include <folly/Range.h>
#include <boost/container/flat_map.hpp>
#include <memory>
#include <unordered_map>

DECLARE_bool(enable_routes_in_host_table);

namespace facebook { namespace fboss {

class BcmSwitch;
class WedgePort;

class WedgePlatform : public BcmPlatform {
 public:
  ~WedgePlatform() override;

  void init();

  HwSwitch* getHwSwitch() const override;
  virtual void onHwInitialized(SwSwitch* sw) override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  folly::MacAddress getLocalMac() const override;
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;

  void onUnitAttach(int unit) override;
  void getProductInfo(ProductInfo& info) override;

  bool canUseHostTableForHostRoutes() const override {
    // This should be enabled only with SDK versions 6.4.6 or later (with
    // wedge_agent, and not with wedge_ctrl).
    return FLAGS_enable_routes_in_host_table;
  }
  WedgePort* getPort(PortID port);

 protected:
  explicit WedgePlatform(std::unique_ptr<WedgeProductInfo> productInfo);
  WedgePlatform(std::unique_ptr<WedgeProductInfo> productInfo,
                uint8_t numQsfps);
  typedef boost::container::flat_map<PortID, std::unique_ptr<WedgePort>>
    WedgePortMap;

  WedgePlatformMode getMode() const;

  WedgePortMap ports_;
  std::unique_ptr<WedgeI2CBusLock> wedgeI2CBusLock_;

 private:
  // Forbidden copy constructor and assignment operator
  WedgePlatform(WedgePlatform const &) = delete;
  WedgePlatform& operator=(WedgePlatform const &) = delete;

  void initLocalMac();
  virtual std::map<std::string, std::string> loadConfig();
  virtual std::unique_ptr<BaseWedgeI2CBus> getI2CBus();
  virtual PortID fbossPortForQsfpChannel(int transceiver, int channel);

  void initTransceiverMap(SwSwitch* sw);
  virtual folly::ByteRange defaultLed0Code() = 0;
  virtual folly::ByteRange defaultLed1Code() = 0;

  folly::MacAddress localMac_;
  std::unique_ptr<BcmSwitch> hw_;

  const std::unique_ptr<WedgeProductInfo> productInfo_;
  uint8_t numQsfpModules_{0};
};

}} // namespace facebook::fboss
