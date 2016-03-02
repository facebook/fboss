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
#include "fboss/agent/types.h"

extern "C" {
#include "saitypes.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;
class SaiVrf;

class SaiVrfTable {
public:
  explicit SaiVrfTable(const SaiSwitch *hw);
  virtual ~SaiVrfTable();

  /**
   * Given fbossVrfId gets Sai Vrf ID of sai_object_id_t type. 
   * Throws an exception if there is no such Vrf found.
   *  
   * @return Sai Vrf ID of sai_object_id_t type
   */
  sai_object_id_t GetSaiVrfId(RouterID fbossVrfId) const;

  /*
   * The following functions will modify the object. They rely on the global
   * HW update lock in SaiSwitch::lock_ for the protection.
   *
   * SaiVrfTable maintains a reference counter for each SaiVrf
   * entry allocated.
   */

  /**
   * Allocates a new SaiVrf if no such one exists. For the existing
   * entry, incRefOrCreateSaiVrf() will increase the reference counter by 1. 
   *  
   * @return The SaiVrf pointer just created or found.
   */
  SaiVrf* IncRefOrCreateSaiVrf(RouterID fbossVrfId);

  /**
   * Decrease an existing SaiVrf entry's reference counter by 1.
   * Only until the reference counter is 0, the SaiVrf entry
   * is deleted.
   *
   * @return nullptr, if the SaiVrf entry is deleted
   * @retrun the SaiVrf object that has reference counter
   *         decreased by 1, but the object is still valid as it is
   *         still referred in somewhere else
   */
  SaiVrf *DerefSaiVrf(RouterID fbossVrfId) noexcept;

private:
  typedef std::pair<std::unique_ptr<SaiVrf>, uint32_t> VrfMapNode; 
  typedef boost::container::flat_map<RouterID, VrfMapNode> VrfMap;

  const SaiSwitch *hw_;
  VrfMap vrfs_;
};

}} // facebook::fboss
