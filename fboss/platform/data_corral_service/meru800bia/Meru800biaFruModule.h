// Copyright 2023-present Facebook. All Rights Reserved.

#pragma once

#include <fboss/platform/data_corral_service/FruModule.h>

namespace facebook::fboss::platform::data_corral_service {

class Meru800biaFruModule : public FruModule {
 public:
  explicit Meru800biaFruModule(std::string id) : FruModule(id) {}
  void refresh() override;
  void init(std::vector<AttributeConfig>& attrs) override;

 private:
  std::string presentPath_;
};

} // namespace facebook::fboss::platform::data_corral_service
