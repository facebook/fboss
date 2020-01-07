/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/lib/usb/PCA9548MuxedBus.h"

namespace facebook::fboss {
void MuxChannel::select() {
  mux->select(channel);
}

void QsfpMux::clear(bool force) {
  // factor in selected channel
  auto selected = mux_.selected();
  if (force) {
    // clear was called with force=true. Let's clear everything in
    // this case.
    selected = 0b11111111;
  }

  if (!selected) {
    return;
  }

  for (uint8_t channel = 0; selected; selected >>= 1, ++channel) {
    if (selected % 2 == 1) {
      if (children(channel).size() > 0) {
        if (!mux_.isSelected(channel)) {
          select(channel);
        }
        pca9548_helpers::clearLayer(children(channel), force);
      }
    }
  }

  mux_.unselectAll();
}

void QsfpMux::registerChildMux(
    CP2112Intf* dev,
    uint8_t channel,
    uint8_t address) {
  children_[channel].push_back(
      std::make_unique<QsfpMux>(dev, address, this, channel));
}

namespace pca9548_helpers {

void clearLayer(MuxLayer& layer, bool force) {
  for (auto& mux : layer) {
    mux->clear(force);
  }
}

void selectPath(Path::iterator begin, Path::iterator end) {
  for (auto it = begin; it != end; ++it) {
    (*it)->select();
  }
}

} // namespace pca9548_helpers

} // namespace facebook::fboss
