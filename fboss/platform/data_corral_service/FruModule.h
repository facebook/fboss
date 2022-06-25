// Copyright 2014-present Facebook. All Rights Reserved.

#pragma once

#include <string>
#include "fboss/platform/data_corral_service/if/gen-cpp2/data_corral_service_types.h"

namespace facebook::fboss::platform::data_corral_service {

class FruModule {
 public:
  explicit FruModule(std::string id) {
    id_ = id;
  }

  virtual void init(std::vector<AttributeConfig>& /*attrs*/) {}

  virtual void refresh() {}

  bool isPresent() {
    return isPresent_;
  }

  std::string getFruId() {
    return id_;
  }

  virtual ~FruModule() {}

 protected:
  std::string id_;
  bool isPresent_{false};
};

} // namespace facebook::fboss::platform::data_corral_service
