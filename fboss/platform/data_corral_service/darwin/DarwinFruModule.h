// Copyright 2014-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/platform/data_corral_service/FruModule.h>

namespace facebook::fboss::platform::data_corral_service {

class DarwinFruModule : public FruModule {
 public:
  explicit DarwinFruModule(std::string id) : FruModule(id) {}
  void refresh() override;
  void init(std::vector<AttributeConfig>& attrs) override;

 private:
  std::string presentPath_;
};

} // namespace facebook::fboss::platform::data_corral_service
