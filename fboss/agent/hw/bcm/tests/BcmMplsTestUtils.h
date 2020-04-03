// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/agent/state/LabelForwardingAction.h"

extern "C" {
#include <bcm/l3.h>
#include <bcm/mpls.h>
#include <bcm/port.h>
}

namespace facebook::fboss::utility {

void verifyLabeledEgress(bcm_if_t egressId, bcm_mpls_label_t label);

void verifyTunnel(
    bcm_if_t tunnel,
    const LabelForwardingAction::LabelStack& stack);
void verifyTunneledEgress(
    bcm_if_t egressId,
    bcm_mpls_label_t tunnelLabel,
    const LabelForwardingAction::LabelStack& tunnelStack);

void verifyTunneledEgress(
    bcm_if_t egressId,
    const LabelForwardingAction::LabelStack& tunnelStack);

void verifyTunneledEgressToCPU(
    bcm_if_t egressId,
    bcm_mpls_label_t tunnelLabel,
    const LabelForwardingAction::LabelStack& tunnelStack);

void verifyTunneledEgressToDrop(
    bcm_if_t egressId,
    bcm_mpls_label_t tunnelLabel,
    const LabelForwardingAction::LabelStack& tunnelStack);

void verifyTunneledEgressToDrop(
    bcm_if_t egressId,
    const LabelForwardingAction::LabelStack& tunnelStack);

void verifyLabeledMultiPathEgress(
    uint32_t unLabeled,
    uint32_t labeled,
    bcm_if_t egressId,
    const std::map<
        bcm_port_t,
        std::pair<bcm_mpls_label_t, LabelForwardingAction::LabelStack>>&
        stacks);

void verifyLabeledMultiPathEgress(
    uint32_t unLabeled,
    uint32_t labeled,
    bcm_if_t egressId,
    const std::map<bcm_port_t, LabelForwardingAction::LabelStack>& stacks);

void verifyEgress(
    bcm_if_t egressId,
    bcm_port_t port,
    const bcm_mac_t& mac,
    bcm_if_t intf);

void verifyMultiPathEgress(
    bcm_if_t egressId,
    const std::vector<bcm_port_t>& ports,
    const std::vector<bcm_mac_t>& macs,
    const std::vector<bcm_if_t>& intfs);

std::pair<bcm_mpls_label_t, LabelForwardingAction::LabelStack>
getEgressLabelAndTunnelStackFromPushStack(
    const LabelForwardingAction::LabelStack& pushStack);

} // namespace facebook::fboss::utility
