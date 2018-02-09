from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals


def get_interfaces_ids(switch_thrift):
    with switch_thrift() as client:
        interface_names = client.getInterfaceList()
    return convert_interfacename_to_id(interface_names)


def get_interfaces_mtu(switch_thrift, interfaces=None):
    if not interfaces:
        interfaces = []
    interfaces_mtu = {}
    if not interfaces:
        interfaces = get_interfaces_ids(switch_thrift)
    for interface in interfaces:
        with switch_thrift() as client:
            interface_info = client.getInterfaceDetail(interface)
        interfaces_mtu[interface] = interface_info.mtu
    return interfaces_mtu


def convert_interfacename_to_id(interface_names):
    interface_ids = []
    for name in interface_names:
        interface_ids.append(int(name[10:]))
    return interface_ids
