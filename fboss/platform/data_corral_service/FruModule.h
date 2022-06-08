// Copyright 2014-present Facebook. All Rights Reserved.

namespace facebook::fboss::platform::data_corral_service {

class FruModule {
 public:
  explicit FruModule(int id) {
    id = id_;
  }

  virtual void refresh() {}

  bool isPresent() {
    return isPresent_;
  }

  int getFruId() {
    return id_;
  }

  virtual ~FruModule() {}

 protected:
  int id_;
  bool isPresent_{false};
};

} // namespace facebook::fboss::platform::data_corral_service
