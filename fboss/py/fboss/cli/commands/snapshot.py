#!/usr/bin/env python3
#
# Copyright 2004-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#


class AgentSnapshotClient:
    """
    This class's snapshots are backwards compatible
    iff features are added or deleted only. What this means
    is that adding a new field or function to this class will
    NOT allow old snapshots to exhibit the new feature.
    Modifying existing functions and member variables
    will result in the CLI unable to parse
    the snapshot file correctly.
    """

    def __init__(
        self,
        agent_config,
        ndp_table,
        lldp_neighbors,
        all_port_info,
        l2_info,
        route_table,
        route_table_details,
        port_status,
        interfaces,
        interface_names,
        arp_table,
        product_info,
        build_info,
    ):
        self.agent_config = agent_config
        self.ndp_table = ndp_table
        self.lldp_neighbors = lldp_neighbors
        self.all_port_info = all_port_info
        self.l2_info = l2_info
        self.route_table = route_table
        self.route_table_details = route_table_details
        self.port_status = port_status
        self.interfaces = interfaces
        self.interface_names = interface_names
        self.arp_table = arp_table
        self.product_info = product_info
        self.build_info = build_info
        self.is_snapshot = True

    def get_build_info(self):
        return self.build_info

    def __enter__(self):
        return self

    def __exit__(self, type, value, traceback):
        pass

    def getRunningConfig(self):
        return self.agent_config

    def getNdpTable(self):
        return self.ndp_table

    def getL2Table(self):
        return self.l2_info

    def getAllPortInfo(self):
        return self.all_port_info

    def getAllPortStats(self):
        return self.all_port_info

    def getPortInfo(self, port_id):
        return self.all_port_info[port_id]

    def getPortStatus(self, ports=None):
        if ports is None:
            return self.port_status
        else:
            build = {}
            for p in ports:
                build[p] = self.port_status[p]
            return build

    def getSelectedCounters(self, counters):
        raise Exception("This command is not supported in snapshot")

    def getProductInfo(self):
        return self.product_info

    def getInterfaceDetail(self, interface):
        return self.interfaces[interface]

    def getArpTable(self):
        return self.arp_table

    def getLldpNeighbors(self):
        return self.lldp_neighbors

    def getRouteTable(self):
        return self.route_table

    def getRouteTableDetails(self):
        return self.route_table_details

    def getRouteTableByClient(self, client):
        raise Exception("This command is not supported in snapshot")

    def getIpRouteDetails(self, addr, vrf):
        raise Exception("This command is not supported in snapshot")
