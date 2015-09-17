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

#include <folly/MacAddress.h>
#include <boost/container/flat_map.hpp>
#include <memory>
#include <unordered_map>

namespace facebook { namespace fboss {

class BcmSwitch;
class WedgePort;


class WedgePlatform : public BcmPlatform {
 public:
  enum Mode {
    WEDGE,
    LC,
    FC
  };

  WedgePlatform();
  ~WedgePlatform() override;

  HwSwitch* getHwSwitch() const override;
  void onHwInitialized(SwSwitch* sw) override;
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch* sw) override;

  folly::MacAddress getLocalMac() const override;
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;

  void onUnitAttach() override;
  InitPortMap initPorts() override;
  void getProductInfo(ProductInfo& info) override;

 private:
  typedef boost::container::flat_map<PortID, std::unique_ptr<WedgePort>>
    WedgePortMap;

  // Forbidden copy constructor and assignment operator
  WedgePlatform(WedgePlatform const &) = delete;
  WedgePlatform& operator=(WedgePlatform const &) = delete;

  void initLocalMac();
  std::map<std::string, std::string> loadConfig();
  void initMode();

  Mode mode_;
  folly::MacAddress localMac_;
  std::unique_ptr<BcmSwitch> hw_;
  WedgePortMap ports_;
  WedgeProductInfo productInfo_;
};

}} // namespace facebook::fboss
