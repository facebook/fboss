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

#include <string>
#include <unordered_set>
#include <vector>

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

/*
 * Shared parsing + apply helpers for the IP-in-IP tunnel delete commands.
 *
 * The encap and decap delete subcommands differ only in which optional
 * attributes can be reset (termination-type is decap-only; source is
 * required for encap and so only resettable on decap) and the direction
 * they expect the target tunnel to have. The token parsing and the
 * delete/reset apply step are otherwise identical, so they live here.
 * Attribute-name constants and string helpers are shared with the config
 * side via TunnelIpInIpConfigUtils.h (tunnel_utils namespace).
 */
namespace facebook::fboss::tunnel_delete_utils {

/*
 * Parse "<tunnel-id> [<attr> ...]" into a tunnel ID and a list of attribute
 * names to reset. Only attributes in `resettableAttrs` are accepted; names in
 * `requiredAttrs` are explicitly rejected with a "cannot reset required
 * field" message (the per-direction required sets differ: source is required
 * for encap only). `resettableDisplay` provides a deterministic ordering for
 * the "valid attributes" error text. Throws std::invalid_argument on
 * malformed input.
 */
void parseTunnelDeleteArgs(
    const std::vector<std::string>& v,
    const std::unordered_set<std::string>& resettableAttrs,
    const std::unordered_set<std::string>& requiredAttrs,
    const std::vector<std::string>& resettableDisplay,
    std::string& tunnelId,
    std::vector<std::string>& attributes);

/*
 * Apply the delete/reset to the staged config. With no attributes the entire
 * tunnel is removed; otherwise the listed optional attributes are reset.
 * Deleting a non-existent tunnel is idempotent (no throw). Throws
 * std::invalid_argument if the target tunnel exists but with a different
 * direction than `tunnelType`, when resetting 'source' on an encap tunnel
 * (encap requires an outer source), or when resetting 'source' on a
 * p2p-terminated decap tunnel without also resetting 'termination-type'.
 * `changed` is set to true iff the staged config was actually mutated (so
 * the caller can skip saveConfig on a no-op). Returns a human-readable
 * result string.
 */
std::string applyTunnelDelete(
    cfg::SwitchConfig& swConfig,
    TunnelType tunnelType,
    const std::string& tunnelId,
    const std::vector<std::string>& attributes,
    bool& changed);

/*
 * Full handler body shared by the encap and decap delete subcommands: apply
 * the delete/reset to the active ConfigSession's staged agent config,
 * persist the session if anything changed, and return the result message.
 */
std::string deleteTunnel(
    TunnelType tunnelType,
    const std::string& tunnelId,
    const std::vector<std::string>& attributes);

/*
 * Common arg type for the encap/decap delete subcommands: a tunnel ID plus
 * an optional list of attribute names to reset. The direction-specific leaf
 * classes only supply their resettable/required attribute sets.
 */
class TunnelIpInIpDeleteArgsBase
    : public utils::BaseObjectArgType<std::string> {
 public:
  TunnelIpInIpDeleteArgsBase(
      std::vector<std::string> v,
      const std::unordered_set<std::string>& resettableAttrs,
      const std::unordered_set<std::string>& requiredAttrs,
      const std::vector<std::string>& resettableDisplay) {
    parseTunnelDeleteArgs(
        v,
        resettableAttrs,
        requiredAttrs,
        resettableDisplay,
        tunnelId_,
        attributes_);
    data_ = std::move(v);
  }

  const std::string& getTunnelId() const {
    return tunnelId_;
  }

  const std::vector<std::string>& getAttributes() const {
    return attributes_;
  }

  bool deleteEntireTunnel() const {
    return attributes_.empty();
  }

  const static utils::ObjectArgTypeId id =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;

 private:
  std::string tunnelId_;
  std::vector<std::string> attributes_;
};

} // namespace facebook::fboss::tunnel_delete_utils
