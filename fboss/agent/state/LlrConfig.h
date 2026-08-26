#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/gen-cpp2/switch_state_types.h"
#include "fboss/agent/state/NodeBase.h"
#include "fboss/agent/state/Thrifty.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

class LlrConfig;
USE_THRIFT_COW(LlrConfig)
/*
 * LlrConfig stores a UEC Link Layer Retry profile (UE Spec 1.0.2 section 5.1.4,
 * Table 5-9), referenced per-port by name via Port::llrConfigName.
 */
class LlrConfig : public ThriftStructNode<LlrConfig, state::LlrFields> {
 public:
  using Base = ThriftStructNode<LlrConfig, state::LlrFields>;

  explicit LlrConfig(const std::string& id) {
    set<switch_state_tags::id>(id);
  }

  const std::string& getID() const {
    return cref<switch_state_tags::id>()->cref();
  }

  int32_t getOutstandingFramesMax() const {
    return get<switch_state_tags::outstandingFramesMax>()->cref();
  }
  void setOutstandingFramesMax(int32_t val) {
    set<switch_state_tags::outstandingFramesMax>(val);
  }

  int32_t getOutstandingBytesMax() const {
    return get<switch_state_tags::outstandingBytesMax>()->cref();
  }
  void setOutstandingBytesMax(int32_t val) {
    set<switch_state_tags::outstandingBytesMax>(val);
  }

  int32_t getReplayTimerMax() const {
    return get<switch_state_tags::replayTimerMax>()->cref();
  }
  void setReplayTimerMax(int32_t val) {
    set<switch_state_tags::replayTimerMax>(val);
  }

  int16_t getReplayCountMax() const {
    return get<switch_state_tags::replayCountMax>()->cref();
  }
  void setReplayCountMax(int16_t val) {
    set<switch_state_tags::replayCountMax>(val);
  }

  int32_t getPcsLostTimeout() const {
    return get<switch_state_tags::pcsLostTimeout>()->cref();
  }
  void setPcsLostTimeout(int32_t val) {
    set<switch_state_tags::pcsLostTimeout>(val);
  }

  int32_t getDataAgeTimeout() const {
    return get<switch_state_tags::dataAgeTimeout>()->cref();
  }
  void setDataAgeTimeout(int32_t val) {
    set<switch_state_tags::dataAgeTimeout>(val);
  }

  cfg::LlrFrameAction getInitFrameAction() const {
    return get<switch_state_tags::initFrameAction>()->cref();
  }
  void setInitFrameAction(cfg::LlrFrameAction val) {
    set<switch_state_tags::initFrameAction>(val);
  }

  cfg::LlrFrameAction getFlushFrameAction() const {
    return get<switch_state_tags::flushFrameAction>()->cref();
  }
  void setFlushFrameAction(cfg::LlrFrameAction val) {
    set<switch_state_tags::flushFrameAction>(val);
  }

  bool getReInitOnFlush() const {
    return get<switch_state_tags::reInitOnFlush>()->cref();
  }
  void setReInitOnFlush(bool val) {
    set<switch_state_tags::reInitOnFlush>(val);
  }

  int32_t getCtlosTargetSpacing() const {
    return get<switch_state_tags::ctlosTargetSpacing>()->cref();
  }
  void setCtlosTargetSpacing(int32_t val) {
    set<switch_state_tags::ctlosTargetSpacing>(val);
  }

 private:
  // Inherit the constructors required for clone()
  using Base::Base;
  friend class CloneAllocator;
};
} // namespace facebook::fboss
