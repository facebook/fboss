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

#include "fboss/agent/state/Interface.h"
#include "fboss/agent/types.h"

extern "C" {
#include "saitypes.h"
}

namespace facebook { namespace fboss {

class SaiSwitch;
class SaiIntf;

class SaiIntfTable {
public:
  explicit SaiIntfTable(const SaiSwitch *hw);
  virtual ~SaiIntfTable();

  // throw an error if not found
  SaiIntf *GetIntf(InterfaceID id) const;
  SaiIntf *GetIntf(sai_object_id_t id) const;

  // return nullptr if not found
  SaiIntf *GetIntfIf(InterfaceID id) const;
  SaiIntf *GetIntfIf(sai_object_id_t id) const;
  SaiIntf *GetFirstIntfIf() const;
  SaiIntf *GetNextIntfIf(const SaiIntf *intf) const;

  // The following functions will modify the object. They rely on the global
  void AddIntf(const std::shared_ptr<Interface> &intf);
  void ProgramIntf(const std::shared_ptr<Interface> &intf);
  void DeleteIntf(const std::shared_ptr<Interface> &intf);

private:
  const SaiSwitch *hw_;
  // There are two mapping tables with different index types.
  // Both are mapped to the SaiIntf. The SaiIntf object's life is
  // controlled by table with InterfaceID as the index (intfs_).
  boost::container::flat_map<InterfaceID, std::unique_ptr<SaiIntf>> intfs_;
  boost::container::flat_map<sai_object_id_t, SaiIntf *> saiIntfs_;
};

}} // facebook::fboss
