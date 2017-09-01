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

#include "fboss/agent/platforms/wedge/WedgePlatform.h"
#include "fboss/agent/platforms/wedge/WedgePort.h"
#include "fboss/agent/FbossError.h"

#include <boost/container/flat_map.hpp>
#include <folly/Optional.h>

namespace facebook { namespace fboss {

/*
 * This class begins encapsulating all port mapping logic in a more
 * organized manner. The main mechanism for this is the
 * Port2TransceiverAndXPEs type that we pass in to the constructor. This
 * map defines every quad in the system (set of 4 contiguous related
 * ports that share a SerDes), and the corresponding front panel port
 * (TransceiverID) if it exists. If a quad is a backplane port and is
 * not associated w/ any transceiver, the value should be nullptr.
 */

struct TransceiverAndXPEs {
  TransceiverAndXPEs(
      folly::Optional<TransceiverID> _transceiver,
      const BcmPlatformPort::XPEs&  _egressXPEs)
      : transceiver(std::move(_transceiver)),
        egressXPEs(_egressXPEs) {}

  folly::Optional<TransceiverID> transceiver;

  // TODO(aeckert): can we determine XPE programatically?
  BcmPlatformPort::XPEs egressXPEs;
};

class WedgePortMapping {
 public:
  enum : uint8_t { CHANNELS_IN_QSFP = 4 };

  typedef std::map<PortID, TransceiverAndXPEs> Port2TransceiverAndXPEs;
  typedef boost::container::flat_map<
    PortID, std::unique_ptr<WedgePort>>::iterator MappingIterator;
  typedef boost::container::flat_map<
    PortID, std::unique_ptr<WedgePort>>::const_iterator ConstMappingIterator;
  using XPEs = BcmPlatformPort::XPEs;

  explicit WedgePortMapping(WedgePlatform* platform) : platform_(platform) {}
  virtual ~WedgePortMapping() = default;

  template<typename MappingT>
  static std::unique_ptr<WedgePortMapping> create(
      WedgePlatform* platform, const Port2TransceiverAndXPEs& portMapping) {
    auto mapping = std::make_unique<MappingT>(platform);
    for (auto& kv : portMapping) {
      mapping->addQuad(kv.first, kv.second.egressXPEs, kv.second.transceiver);
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
  WedgePort* getPort(const TransceiverID fpPort) const {
    auto iter = frontPanelMap_.find(fpPort);
    if (iter == frontPanelMap_.end()) {
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
  virtual std::unique_ptr<WedgePort> constructPort (
    const PortID port,
    const folly::Optional<TransceiverID>,
    const folly::Optional<ChannelID>,
    const XPEs&) const = 0;

  /* Helper to add four associated ports to the port
   * mapping. Optionally specify a front panel port mapping if this
   * quad is associated w/ a front panel port. This is a common idiom
   * because QSFPs use four channels internally.
   */
  void addQuad(
      const PortID start,
      const XPEs& egressXPEs,
      const folly::Optional<TransceiverID> frontPanel = folly::none) {
    for (int num = 0; num < CHANNELS_IN_QSFP; ++num) {
      folly::Optional<ChannelID> channel;
      if (frontPanel) {
        channel = ChannelID(num);
      }
      PortID id(start + num);
      ports_[id] = constructPort(id, frontPanel, channel, egressXPEs);
    }
    if (frontPanel) {
      // register the controlling port in the transceiver map
      frontPanelMap_[*frontPanel] = ports_[start].get();
    }
  }

  boost::container::flat_map<PortID, std::unique_ptr<WedgePort>> ports_;
  boost::container::flat_map<TransceiverID, WedgePort*> frontPanelMap_;
};

template<typename WedgePortT>
class WedgePortMappingT : public WedgePortMapping {
 public:
  explicit WedgePortMappingT(WedgePlatform* platform) :
      WedgePortMapping(platform) {}

 private:
  std::unique_ptr<WedgePort> constructPort(
      PortID port,
      folly::Optional<TransceiverID> transceiver,
      folly::Optional<ChannelID> channel,
      const XPEs& egressXPEs) const override {
    return std::make_unique<WedgePortT>(
        port, platform_, transceiver, channel, egressXPEs);
  }

};

}} //facebook::fboss
