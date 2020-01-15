// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <bcm/l3.h>
#include <bcm/types.h>
}

#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/state/PortDescriptor.h"
#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

class BcmEgress;
class BcmSwitch;
class BcmHost;
class BcmLabeledEgress;
class BcmLabeledTunnelEgress;

class BcmNextHop {
 public:
  virtual ~BcmNextHop() {}
  virtual bcm_if_t getEgressId() const = 0;
  virtual void programToCPU(bcm_if_t intf) = 0;
  virtual bool isProgrammed() const = 0;
};

class BcmL3NextHop : public BcmNextHop {
 public:
  BcmL3NextHop(BcmSwitch* hw, BcmHostKey key);

  bcm_if_t getEgressId() const override;

  void programToCPU(bcm_if_t intf) override;

  bool isProgrammed() const override;

 private:
  BcmSwitch* hw_;
  BcmHostKey key_;
  std::shared_ptr<BcmHost> host_;
};

class BcmMplsNextHop : public BcmNextHop {
 public:
  BcmMplsNextHop(BcmSwitch* hw, BcmLabeledHostKey key);

  ~BcmMplsNextHop() override;

  bcm_if_t getEgressId() const override;

  void programToCPU(bcm_if_t intf) override;

  bool isProgrammed() const override;

  void program(BcmHostKey bcmHostKey);

  BcmHostKey getBcmHostKey() {
    return BcmHostKey(key_.getVrf(), key_.addr(), key_.intfID());
  }

  BcmLabeledEgress* getBcmLabeledEgress() const;

  BcmLabeledTunnelEgress* getBcmLabeledTunnelEgress() const;

  bcm_gport_t getGPort();

 private:
  std::unique_ptr<BcmEgress> createEgress();
  void setPort(bcm_port_t port);
  void setTrunk(bcm_trunk_t trunk);

  BcmSwitch* hw_;
  BcmLabeledHostKey key_;
  std::optional<PortDescriptor> egressPort_;
  std::unique_ptr<BcmEgress> mplsEgress_;
};

template <typename NextHopKeyT, typename NextHopT>
class BcmNextHopTable {
 public:
  using MapT = FlatRefMap<NextHopKeyT, NextHopT>;

  explicit BcmNextHopTable(BcmSwitch* hw) : hw_(hw) {}

  const NextHopT* getNextHopIf(const NextHopKeyT& key) const {
    return nexthops_.get(key);
  }

  const NextHopT* getNextHop(const NextHopKeyT& key) const;

  std::shared_ptr<NextHopT> referenceOrEmplaceNextHop(const NextHopKeyT& key);

  const MapT& getNextHops() const {
    return nexthops_;
  }

  BcmSwitch* getBcmSwitch() {
    return hw_;
  }

 private:
  BcmSwitch* hw_;
  MapT nexthops_;
};

using BcmL3NextHopTable = BcmNextHopTable<BcmHostKey, BcmL3NextHop>;
using BcmMplsNextHopTable = BcmNextHopTable<BcmLabeledHostKey, BcmMplsNextHop>;

} // namespace facebook::fboss
