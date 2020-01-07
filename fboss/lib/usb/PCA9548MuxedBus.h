/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <stdint.h>
#include <deque>
#include <memory>
#include <vector>

#include "fboss/lib/usb/BaseWedgeI2CBus.h"
#include "fboss/lib/usb/PCA9548.h"

#include <folly/Format.h>
#include <folly/String.h>

namespace facebook::fboss {
class QsfpMux;

struct MuxChannel {
  MuxChannel() {}
  MuxChannel(QsfpMux* mux_, uint8_t channel_) : mux(mux_), channel(channel_) {}

  std::string str() const {
    return folly::sformat("(mux={},channel={})", mux, channel);
  }

  QsfpMux* mux{nullptr};
  uint8_t channel{0};

  void select();
};

inline void toAppend(MuxChannel* value, std::string* result) {
  folly::toAppend(value->str(), result);
}

using Path = std::deque<MuxChannel*>;
using MuxLayer = std::vector<std::unique_ptr<QsfpMux>>;

namespace pca9548_helpers {

void clearLayer(MuxLayer& layer, bool force = false);
void selectPath(Path::iterator begin, Path::iterator end);

} // namespace pca9548_helpers

class QsfpMux {
 public:
  QsfpMux(CP2112Intf* dev, uint8_t address) : mux_(dev, address) {
    VLOG(3) << "Adding root mux at " << &mux_;
  }
  QsfpMux(
      CP2112Intf* dev,
      uint8_t address,
      QsfpMux* parent,
      uint8_t parentChannel)
      : mux_(dev, address),
        parent_(std::make_unique<MuxChannel>(parent, parentChannel)) {
    VLOG(3) << "Adding child mux " << &mux_ << " to " << parent->mux()
            << " on channel " << (int)parentChannel;
  }

  void clear(bool force = false);

  MuxChannel* parent() const {
    return parent_.get();
  }
  MuxLayer& children(uint8_t channel) {
    return children_.at(channel);
  }
  void select(uint8_t channel) {
    mux_.selectOne(channel);
  }

  void registerChildMux(CP2112Intf* dev, uint8_t channel, uint8_t address);

  PCA9548* mux() {
    return &mux_;
  }

 private:
  PCA9548 mux_;
  const std::unique_ptr<MuxChannel> parent_{nullptr};
  std::array<MuxLayer, PCA9548::WIDTH> children_;
};

template <int NUM_PORTS>
class PCA9548MuxedBus : public BaseWedgeI2CBus {
 public:
  explicit PCA9548MuxedBus(std::unique_ptr<CP2112Intf> dev = nullptr)
      : BaseWedgeI2CBus(std::move(dev)) {}

  constexpr int numPorts() {
    return NUM_PORTS;
  }

  void verifyBus(bool /* autoReset */) override {
    // Hacky bus verification for now that removes any assumptions
    // about what the currently selected path is. We should probably
    // extend this to do more bus health checking in the future.
    selectedPort_ = NO_PORT;
  }

 protected:
  using PortLeaves = std::array<MuxChannel, NUM_PORTS>;

  void connectPortsToMux(
      PortLeaves& leaves,
      QsfpMux* mux,
      int start,
      bool flip = false,
      bool reverse = false,
      uint8_t count = PCA9548::WIDTH) {
    for (uint8_t i = 0; i < count; ++i) {
      auto& leaf = leaves.at(start + i);
      leaf.mux = mux;
      // if reverse, channel should start with 7; otherwise start with 0
      int channel = (reverse) ? count - 1 - i : i;
      leaf.channel = (flip) ? channel ^ 0x1 : channel;
    }
  }

  void initBus() override {
    roots_ = createMuxes();
    wireUpPorts(leaves_);
    ensureNothingSelected();
  }

  void selectQsfpImpl(unsigned int port) override {
    VLOG(5) << "setting port to " << port << " from " << selectedPort_;
    auto newPath = calculatePath(port);
    auto existingPath = calculatePath(selectedPort_);

    // We assume that the tree is perfectly balanced right now.

    if (newPath.empty() && existingPath.empty()) {
      // should never happen in practice
      return;
    }

    if (newPath.empty()) {
      existingPath[0]->mux->clear();
    } else if (existingPath.empty()) {
      vlogPath(newPath, "new path");
      pca9548_helpers::selectPath(newPath.begin(), newPath.end());
    } else {
      changePath(existingPath, newPath);
    }

    selectedPort_ = port;
  }

  MuxLayer roots_;

 private:
  void changePath(Path existingPath, Path newPath) {
    vlogPath(existingPath, "existing path");
    vlogPath(newPath, "new path");

    for (int i = 0; i < newPath.size(); ++i) {
      auto desired = newPath[i];
      auto current = existingPath[i];
      if (current->mux == desired->mux &&
          current->channel == desired->channel) {
        // matches currently selected mux at this layer
        continue;
      }

      // something is different, time to rewire the path
      if (current->mux != desired->mux) {
        current->mux->clear();
      } else if (current->channel != desired->channel) {
        // make sure all muxes in the sub-channel are cleared
        pca9548_helpers::clearLayer(current->mux->children(current->channel));
      }

      pca9548_helpers::selectPath(newPath.begin() + i, newPath.end());
      break;
    }
  }

  Path calculatePath(unsigned int port) {
    Path path;
    if (port == NO_PORT) {
      return path;
    }

    // I don't know why we have this strange - 1 semantic, but
    // preserving for now.
    auto curr = &leaves_.at(port - 1);
    while (curr) {
      path.push_front(curr);
      curr = curr->mux->parent();
    }
    return path;
  }

  void vlogPath(const Path& path, std::string label, int level = 3) {
    VLOG(level) << label << ": " << folly::join(",", path);
  }

  /*
   * This function should return the roots of the mux topology. We
   * expect that all possible muxes are created in this function and
   * are wired up using registerChildMux.
   */
  virtual MuxLayer createMuxes() = 0;

  /*
   * This function should return an std::array of the correct width
   * for which MuxChannel a port is connected to. It is safe to assume
   * that all muxer are already created and the roots are in roots_.
   */
  virtual void wireUpPorts(PortLeaves& leaves) = 0;

  void ensureNothingSelected() {
    pca9548_helpers::clearLayer(roots_, true);
  }

  PortLeaves leaves_;
};

} // namespace facebook::fboss
