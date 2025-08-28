// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/bsp/BspGenericSystemContainer.h"
#include <folly/FileUtil.h>
#include <folly/Singleton.h>
#include "fboss/lib/bsp/icecube800bc/Icecube800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/icetea800bc/Icetea800bcBspPlatformMapping.h"
#include "fboss/lib/bsp/janga800bic/Janga800bicBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400bia/Meru400biaBspPlatformMapping.h"
#include "fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.h"
#include "fboss/lib/bsp/meru800bfa/Meru800bfaBspPlatformMapping.h"
#include "fboss/lib/bsp/meru800bia/Meru800biaBspPlatformMapping.h"
#include "fboss/lib/bsp/minipack3n/Minipack3NBspPlatformMapping.h"
#include "fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.h"
#include "fboss/lib/bsp/morgan800cc/Morgan800ccBspPlatformMapping.h"
#include "fboss/lib/bsp/tahan800bc/Tahan800bcBspPlatformMapping.h"

DEFINE_string(
    bsp_platform_mapping_override_path,
    "",
    "The path to the BSP Platform Mapping JSON file");

namespace facebook {
namespace fboss {

template <typename T>
std::unique_ptr<BspPlatformMapping>
BspGenericSystemContainer<T>::initBspPlatformMapping() {
  std::string platformMappingStr;
  bool mappingOverride = !FLAGS_bsp_platform_mapping_override_path.empty();
  if (mappingOverride) {
    if (!folly::readFile(
            FLAGS_bsp_platform_mapping_override_path.data(),
            platformMappingStr)) {
      throw FbossError(
          "unable to read ", FLAGS_bsp_platform_mapping_override_path);
    }
    XLOG(INFO) << "Overriding BSP platform mapping from "
               << FLAGS_bsp_platform_mapping_override_path;
  }
  if (mappingOverride) {
    return std::make_unique<T>(platformMappingStr);
  }

  return std::make_unique<T>();
}

using Meru400bfuSystemContainer =
    BspGenericSystemContainer<Meru400bfuBspPlatformMapping>;
folly::Singleton<Meru400bfuSystemContainer> _meru400bfuSystemContainer;
template <>
std::shared_ptr<Meru400bfuSystemContainer>
Meru400bfuSystemContainer::getInstance() {
  return _meru400bfuSystemContainer.try_get();
}

using Meru400biaSystemContainer =
    BspGenericSystemContainer<Meru400biaBspPlatformMapping>;
folly::Singleton<Meru400biaSystemContainer> _meru400biaSystemContainer;
template <>
std::shared_ptr<Meru400biaSystemContainer>
Meru400biaSystemContainer::getInstance() {
  return _meru400biaSystemContainer.try_get();
}

using Meru400biuSystemContainer =
    BspGenericSystemContainer<Meru400biuBspPlatformMapping>;
folly::Singleton<Meru400biuSystemContainer> _meru400biuSystemContainer;
template <>
std::shared_ptr<Meru400biuSystemContainer>
Meru400biuSystemContainer::getInstance() {
  return _meru400biuSystemContainer.try_get();
}

using Meru800biaSystemContainer =
    BspGenericSystemContainer<Meru800biaBspPlatformMapping>;
folly::Singleton<Meru800biaSystemContainer> _meru800biaSystemContainer;
template <>
std::shared_ptr<Meru800biaSystemContainer>
Meru800biaSystemContainer::getInstance() {
  return _meru800biaSystemContainer.try_get();
}

using Meru800bfaSystemContainer =
    BspGenericSystemContainer<Meru800bfaBspPlatformMapping>;
folly::Singleton<Meru800bfaSystemContainer> _meru800bfaSystemContainer;
template <>
std::shared_ptr<Meru800bfaSystemContainer>
Meru800bfaSystemContainer::getInstance() {
  return _meru800bfaSystemContainer.try_get();
}

using MontblancSystemContainer =
    BspGenericSystemContainer<MontblancBspPlatformMapping>;
folly::Singleton<MontblancSystemContainer> _montblancSystemContainer;
template <>
std::shared_ptr<MontblancSystemContainer>
MontblancSystemContainer::getInstance() {
  return _montblancSystemContainer.try_get();
}

using Minipack3NSystemContainer =
    BspGenericSystemContainer<Minipack3NBspPlatformMapping>;
folly::Singleton<Minipack3NSystemContainer> _minipack3NSystemContainer;
template <>
std::shared_ptr<Minipack3NSystemContainer>
Minipack3NSystemContainer::getInstance() {
  return _minipack3NSystemContainer.try_get();
}

using Morgan800ccSystemContainer =
    BspGenericSystemContainer<Morgan800ccBspPlatformMapping>;
folly::Singleton<Morgan800ccSystemContainer> _morgan800ccSystemContainer;
template <>
std::shared_ptr<Morgan800ccSystemContainer>
Morgan800ccSystemContainer::getInstance() {
  return _morgan800ccSystemContainer.try_get();
}

using Janga800bicSystemContainer =
    BspGenericSystemContainer<Janga800bicBspPlatformMapping>;
folly::Singleton<Janga800bicSystemContainer> _janga800bicSystemContainer;
template <>
std::shared_ptr<Janga800bicSystemContainer>
Janga800bicSystemContainer::getInstance() {
  return _janga800bicSystemContainer.try_get();
}

using Tahan800bcSystemContainer =
    BspGenericSystemContainer<Tahan800bcBspPlatformMapping>;
folly::Singleton<Tahan800bcSystemContainer> _tahan800bcSystemContainer;
template <>
std::shared_ptr<Tahan800bcSystemContainer>
Tahan800bcSystemContainer::getInstance() {
  return _tahan800bcSystemContainer.try_get();
}

using Icecube800bcSystemContainer =
    BspGenericSystemContainer<Icecube800bcBspPlatformMapping>;
folly::Singleton<Icecube800bcSystemContainer> _icecube800bcSystemContainer;
template <>
std::shared_ptr<Icecube800bcSystemContainer>
Icecube800bcSystemContainer::getInstance() {
  return _icecube800bcSystemContainer.try_get();
}

using Icetea800bcSystemContainer =
    BspGenericSystemContainer<Icetea800bcBspPlatformMapping>;
folly::Singleton<Icetea800bcSystemContainer> _icetea800bcSystemContainer;
template <>
std::shared_ptr<Icetea800bcSystemContainer> Icetea800bcSystemContainer::getInstance() {
  return _icetea800bcSystemContainer.try_get();
}

} // namespace fboss
} // namespace facebook
