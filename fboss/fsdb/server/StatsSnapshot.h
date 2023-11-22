// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <string>
#include "fboss/fsdb/common/Utils.h"
#include "fboss/fsdb/if/FsdbModel.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/utils/Utils.h"

namespace facebook::fboss::fsdb {

class StatsSnapshot {
 public:
  explicit StatsSnapshot(const FqMetric& fqMetric);
  explicit StatsSnapshot(
      const FqMetric& fqMetric,
      const int64_t timestampSec,
      const std::string& value); // called when reading from rocksdb
  explicit StatsSnapshot(
      const FqMetric& fqMetric,
      bool publisherConnected); // control // TODO: add schema as arg
  explicit StatsSnapshot(const FqMetric& fqMetric,
                         Vectors&& vectors); // data

  StatsSnapshot(const StatsSnapshot& other) = default; // copy ctor
  StatsSnapshot& operator=(const StatsSnapshot& other) =
      default; // copy assignment

  ~StatsSnapshot() = default;

  const FqMetric& fqMetric() const {
    return fqMetric_;
  }

  int64_t timestampSec() const {
    return timestampSec_;
  }

  const StatsSubUnitPayload& payload() const {
    return payload_;
  }

  const EchoBackTag& echoBackTag() const {
    return echoBackTag_;
  }

  // TODO: we do not set echoBackTag_ via ctor as there are too many ctor
  // overloads already. These need to be simplified.
  void setEchoBackTag(const EchoBackTag& arg) {
    echoBackTag_ = arg;
  }

  std::string key() const;
  std::string value() const;
  static StatsSubUnitPayload fromValue(const std::string& value);

  bool operator==(const StatsSnapshot& other) const;
  bool operator!=(const StatsSnapshot& other) const {
    return !(*this == other);
  }

  bool isControl() const {
    return payload_.getType() == StatsSubUnitPayload::Type::available ||
        payload_.getType() == StatsSubUnitPayload::Type::unavailable;
  }

  bool getPublisherConnected() const;

  const Vectors& getVectors() const;

  void decompress();

 protected:
  FqMetric fqMetric_;
  int64_t timestampSec_;
  StatsSubUnitPayload payload_;
  EchoBackTag echoBackTag_;
};

} // namespace facebook::fboss::fsdb
