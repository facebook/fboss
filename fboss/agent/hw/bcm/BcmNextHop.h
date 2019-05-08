// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

extern "C" {
#include <opennsl/l3.h>
#include <opennsl/types.h>
}

#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/lib/RefMap.h"

namespace facebook {
namespace fboss {

class BcmSwitch;
class BcmHostReference;

class BcmNextHop {
 public:
  virtual ~BcmNextHop() {}
  virtual opennsl_if_t getEgressId() const = 0;
};

class BcmL3NextHop : public BcmNextHop {
 public:
  BcmL3NextHop(BcmSwitch* hw, BcmHostKey key);

  ~BcmL3NextHop() override {}

  opennsl_if_t getEgressId() const override;

 private:
  BcmSwitch* hw_;
  BcmHostKey key_;
  std::unique_ptr<BcmHostReference> hostReference_;
};

class BcmMplsNextHop : public BcmNextHop {
 public:
  BcmMplsNextHop(BcmSwitch* hw, BcmLabeledHostKey key);

  opennsl_if_t getEgressId() const override;

 private:
  BcmSwitch* hw_;
  BcmLabeledHostKey key_;
  std::unique_ptr<BcmHostReference> hostReference_;
};

template <typename NextHopKeyT, typename NextHopT>
class BcmNextHopTable {
 public:
  using MapT = FlatRefMap<NextHopKeyT, NextHopT>;
  explicit BcmNextHopTable(BcmSwitch* hw) : hw_(hw) {}
  const NextHopT* getNextHopIf(const NextHopKeyT& key) const;
  const NextHopT* getNextHop(const NextHopKeyT& key) const;
  std::shared_ptr<NextHopT> referenceOrEmplaceNextHop(const NextHopKeyT& key);

 private:
  BcmSwitch* hw_;
  MapT nexthops_;
};

using BcmL3NextHopTable = BcmNextHopTable<BcmHostKey, BcmL3NextHop>;
using BcmMplsNextHopTable = BcmNextHopTable<BcmLabeledHostKey, BcmMplsNextHop>;
} // namespace fboss
} // namespace facebook
