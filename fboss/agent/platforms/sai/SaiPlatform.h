/*
 * Copyright (c) 2004-present, Facebook, Inc.
 * Copyright (c) 2016, Cavium, Inc.
 * All rights reserved.
 * 
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 * 
 */
#pragma once

#include <boost/container/flat_map.hpp>
#include <folly/MacAddress.h>

#include "fboss/agent/types.h"
#include "fboss/agent/hw/sai/SaiPlatformBase.h"

namespace facebook { namespace fboss {

class SaiSwitch;
class SaiPort;
class HwSwitch;
class SwSwitch;

class SaiPlatform : public SaiPlatformBase {
public:
  typedef boost::container::flat_map<PortID, std::unique_ptr<SaiPort>> SaiPortMap;

  SaiPlatform();
  ~SaiPlatform();

  HwSwitch *getHwSwitch() const override;
  void initSwSwitch(SwSwitch *sw);
  std::unique_ptr<ThriftHandler> createHandler(SwSwitch *sw) override;

  folly::MacAddress getLocalMac() const override;
  std::string getVolatileStateDir() const override;
  std::string getPersistentStateDir() const override;

  InitPortMap initPorts() override;
  SaiPortMap &GetPortMap() {
    return ports_;
  }

  void onHwInitialized(SwSwitch *sw) override;
  void getProductInfo(ProductInfo& info) override;

private:
  // Forbidden copy constructor and assignment operator
  SaiPlatform(SaiPlatform const &) = delete;
  SaiPlatform &operator=(SaiPlatform const &) = delete;

  void initLocalMac();
  std::map<std::string, std::string> loadConfig();

  folly::MacAddress localMac_;
  std::unique_ptr<SaiSwitch> hw_;
  SaiPortMap ports_;
};

}} // facebook::fboss
