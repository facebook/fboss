#!/usr/bin/env python3

import ipaddress

from libfb.py.decorators import retryable, memoize_forever
from fboss.system_tests.testutils.ip_conversion import ip_addr_to_str


class IntfAddr6Helper:
    def __init__(self, switch_thrift):
        self.switch_thrift = switch_thrift

    @memoize_forever
    @retryable(num_tries=3, sleep_time=1)
    def _get_all_intfs(self):
        return self.switch_thrift.getAllInterfaces().values()

    def _is_v6(self, ipstr):
        return isinstance(ipaddress.ip_address(ipstr), ipaddress.IPv6Address)

    def get_all_intf_subnets_and_ips(self):
        intf_subnets, intf_ips = ([], [])
        for intf in self._get_all_intfs():
            for addr in intf.address:
                ipstr = ip_addr_to_str(addr.ip)
                if not self._is_v6(ipstr):
                    continue

                intf_subnets.append(ipaddress.ip_network(
                    f'{ipstr}/{addr.prefixLength}', strict=False))
                intf_ips.append(ipaddress.ip_address(ipstr))
        return intf_subnets, intf_ips

    def get_unclaimed_intf_subnet_ip(self, skip_link_local=True):
        intf_subnets, intf_ips = self.get_all_intf_subnets_and_ips()
        for subnet in intf_subnets:
            if skip_link_local and subnet.is_link_local:
                continue
            for host in subnet.hosts():
                if host not in intf_ips:
                    return host

        return None
