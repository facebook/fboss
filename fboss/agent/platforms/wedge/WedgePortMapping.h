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
#include "fboss/agent/gen-cpp2/platform_config_types.h"
#include "fboss/agent/platforms/wedge/WedgePlatform.h"
#include "fboss/agent/platforms/wedge/WedgePort.h"

#include <boost/container/flat_map.hpp>
#include <optional>

namespace facebook::fboss {
/*
 * This class begins encapsulating all port mapping logic in a more
 * organized manner. The main mechanism for this is the
 * PortTransceiverMap type that we pass in to the constructor. This
 * map defines the controlling port of a set of 'count' contiguous related ports
 * that share a SerDes), and the corresponding front panel port (TransceiverID)
 * if it exists.
 * If the controlling port is a backplane port and is not associated w/ any
 * transceiver, the Transceiver and Channel value should be nullptr.
 */
class WedgePortMapping {
 public:
  enum : uint8_t { CHANNELS_IN_QSFP28 = 4 };

  using PortTransceiverMap = std::map<PortID, std::optional<TransceiverID>>;
  using PortFrontPanelResourceMap =
      std::map<PortID, std::optional<FrontPanelResources>>;
  using MappingIterator =
      boost::container::flat_map<PortID, std::unique_ptr<WedgePort>>::iterator;
  using ConstMappingIterator = boost::container::
      flat_map<PortID, std::unique_ptr<WedgePort>>::const_iterator;

  explicit WedgePortMapping(WedgePlatform* platform) : platform_(platform) {}
  virtual ~WedgePortMapping() = default;

  template <typename MappingT>
  static std::unique_ptr<WedgePortMapping> createFromConfig(
      WedgePlatform* platform) {
    // TODO(joseph5wu) Right now, the platformPorts is still optional.
    // Once we roll out the config everywhere, we should always use this
    // createFromConfig() to generate the platform port mapping.
    const auto& platformPorts = platform->getPlatformPorts();
    if (platformPorts.empty()) {
      throw FbossError("Can't find platformPorts from platform config");
    }
    XLOG(INFO) << "Create Platform Port Mapping from config";
    auto mapping = std::make_unique<MappingT>(platform);
    for (const auto& port : platformPorts) {
      // Will migrate the code to get front panel resource from the config
      mapping->addPort(PortID(port.first));
    }
    return std::move(mapping);
  }

  WedgePort* getPort(const PortID port) const {
    auto iter = ports_.find(port);
    if (iter == ports_.end()) {
      throw FbossError("Cannot find the port ID for port ", port);
    }
    return iter->second.get();
  }
  std::vector<WedgePort*> getPortsByTransceiverID(
      const TransceiverID fpPort) const {
    auto iter = transceiverMap_.find(fpPort);
    if (iter == transceiverMap_.end()) {
      throw FbossError("Cannot find the port ID for front panel port ", fpPort);
    }
    return iter->second;
  }

  MappingIterator begin() {
    return ports_.begin();
  }
  MappingIterator end() {
    return ports_.end();
  }
  ConstMappingIterator begin() const {
    return ports_.cbegin();
  }
  ConstMappingIterator end() const {
    return ports_.cend();
  }

 protected:
  WedgePlatform* platform_{nullptr};

 private:
  virtual std::unique_ptr<WedgePort> constructPort(const PortID port) = 0;

  void addPort(PortID port) {
    ports_[port] = constructPort(port);

    if (auto tcvrID = ports_[port]->getTransceiverID(); tcvrID) {
      if (transceiverMap_.find(*tcvrID) == transceiverMap_.end()) {
        transceiverMap_[*tcvrID] = {};
      }
      transceiverMap_[*tcvrID].push_back(ports_[port].get());
    }
  }

  boost::container::flat_map<PortID, std::unique_ptr<WedgePort>> ports_;
  boost::container::flat_map<TransceiverID, std::vector<WedgePort*>>
      transceiverMap_;
};

template <typename WedgePlatformT, typename WedgePortT>
class WedgePortMappingT : public WedgePortMapping {
 public:
  explicit WedgePortMappingT(WedgePlatform* platform)
      : WedgePortMapping(platform) {}

 private:
  std::unique_ptr<WedgePort> constructPort(PortID port) override {
    // TODO(joseph5wu) Will remove frontPanel later
    return std::make_unique<WedgePortT>(
        port, dynamic_cast<WedgePlatformT*>(platform_), std::nullopt);
  }
};

} // namespace facebook::fboss
