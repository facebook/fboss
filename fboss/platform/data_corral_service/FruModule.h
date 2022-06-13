// Copyright 2014-present Facebook. All Rights Reserved.

#pragma once

#include <string>

namespace facebook::fboss::platform::data_corral_service {

class FruModule {
 public:
  explicit FruModule(int id) {
    id_ = id;
  }

  virtual void init() {}

  virtual void refresh() {}

  bool isPresent() {
    return isPresent_;
  }

  int getFruId() {
    return id_;
  }

  virtual std::string getName() {
    return "FruModule-" + std::to_string(id_);
  }

  virtual ~FruModule() {}

 protected:
  int id_;
  bool isPresent_{false};
};

} // namespace facebook::fboss::platform::data_corral_service
