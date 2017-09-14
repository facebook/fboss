#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#
import sys
# Main Fboss py dir
sys.path.insert(1, '../../')

# Common dirs
sys.path.insert(2, '../../../agent/if/gen-py')

import click

from fboss.cli.commands import arp
from fboss.cli.commands import config
from fboss.cli.commands import commands as cmds
from fboss.cli.commands import interface
from fboss.cli.commands import info
from fboss.cli.commands import ip
from fboss.cli.commands import l2
from fboss.cli.commands import lldp
from fboss.cli.commands import ndp
from fboss.cli.commands import nic
from fboss.cli.commands import port
from fboss.cli.commands import route
from fboss.cli.utils.utils import AGENT_KEYWORD
from thrift.Thrift import TApplicationException
from thrift.transport.TTransport import TTransportException
from neteng.fboss.ttypes import FbossBaseError
from fboss.thrift_clients import FbossAgentClient

DEFAULT_CLIENTID = 1


class AliasedGroup(click.Group):
    """
    For command abbreviation
        http://click.pocoo.org/5/advanced/#command-aliases
    """

    def get_command(self, ctx, cmd_name):
        rv = click.Group.get_command(self, ctx, cmd_name)
        if rv is not None:
            return rv

        matches = [
            x for x in self.list_commands(ctx)
            if x.startswith(cmd_name)
        ]
        if not matches:
            return None
        elif len(matches) == 1:
            return click.Group.get_command(self, ctx, matches[0])
        ctx.fail('Too many matches: %s' % ', '.join(sorted(matches)))


class CliOptions(object):
    ''' Object for holding CLI state information '''
    def __init__(self, hostname, file, port, timeout):
        self.hostname = hostname
        self.port = port
        self.timeout = timeout
        self.snapshot_file = file


class ArpCli(object):
    ''' ARP Cli sub-commands '''

    def __init__(self):
        self.arp.add_command(self._table, name='table')
        self.arp.add_command(self._flush, name='flush')

    @click.group(cls=AliasedGroup)
    def arp():
        ''' Show ARP Information '''
        pass

    @click.command()
    @click.pass_obj
    def _table(cli_opts):
        ''' Show the ARP table '''
        arp.ArpTableCmd(cli_opts).run()

    @click.command()
    @click.option('-V', '--vlan', type=int, default=0,
                    help='Only flush the IP from the specified VLAN')
    @click.argument('ip')
    @click.pass_obj
    def _flush(cli_opts, ip, vlan):
        ''' Flush an ARP entry by [IP]'''
        cmds.NeighborFlushCmd(cli_opts).run(ip, vlan)


class NicCli(object):
    '''NIC Cli sub-commands '''

    def __init__(self):
        self.nic.add_command(self._vendor, name='vendor')

    @click.group(cls=AliasedGroup)
    def nic():
        ''' Show NIC Information on connected hosts'''
        pass

    @click.command()
    @click.option('--detail', is_flag=True, help='Display detailed port status')
    @click.option('--verbose', is_flag=True, help='Display detailed port status')
    @click.pass_obj
    def _vendor(cli_opts, detail, verbose):
        '''Show NIC vendor information on hosts'''
        nic.NicCmd(cli_opts).run(detail, verbose)


class GetConfigCli(object):
    ''' Get running config sub-commands '''

    def __init__(self):
        self.config.add_command(self._agent, name=AGENT_KEYWORD)

    @click.group(cls=AliasedGroup)
    def config():
        ''' Show running config '''
        pass

    @click.command()
    @click.pass_obj
    def _agent(cli_opts):
        ''' Show controller running config '''
        config.GetConfigCmd(cli_opts).run(AGENT_KEYWORD)


class IpCli(object):
    ''' IP sub-commands '''
    @click.command()
    @click.option("-i", "--interface", type=int, required=True,
            help='Show Ip address of the interface')
    @click.pass_obj
    def ip(cli_opts, interface):
        ''' Show IP information for an interface '''
        ip.IpCmd(cli_opts).run(interface)


class InterfaceCli(object):
    ''' Interface sub-commands '''
    @click.command()
    @click.argument('interfaces', type=int, nargs=-1)
    @click.pass_obj
    def interface(cli_opts, interfaces):
        ''' Show interface information for Interface(s); Outputs a list of
            interfaces on host if no interfaces are specified '''
        interface.InterfaceCmd(cli_opts).run(interfaces)


class L2Cli(object):
    ''' Show L2 sub-commands '''

    def __init__(self):
        self.l2.add_command(self._table, name='table')
        self.l2.add_command(self._flush, name='flush')

    @click.group(cls=AliasedGroup)
    def l2():
        ''' Show L2 information '''
        pass

    @click.command()
    @click.pass_obj
    def _table(cli_opts):
        ''' Show the L2 table '''
        l2.L2TableCmd(cli_opts).run()

    @click.command()
    @click.option('-V', '--vlan', type=int, default=0,
                    help='Only flush the IP from the specified VLAN')
    @click.argument('ip')
    @click.pass_obj
    def _flush(cli_opts, ip, vlan):
        ''' Flush an ARP entry by [IP]'''
        cmds.NeighborFlushCmd(cli_opts).run(ip, vlan)


class LldpCli(object):
    ''' Lldp sub-commands '''

    @click.command()
    @click.option('-p', '--port', type=int, default=None,
                        help='Show only neighbors for the specified lldp port')
    @click.option('-v', '--verbose', count=True,
                        help='Level of verbosity indicated by count, i.e -vvv')
    @click.pass_obj
    def lldp(cli_opts, port, verbose):
        ''' Show LLDP neighbors '''
        lldp.LldpCmd(cli_opts).run(port, verbose)


class NdpCli(object):
    ''' Show NDP sub-commands '''

    def __init__(self):
        self.ndp.add_command(self._table, name='table')
        self.ndp.add_command(self._flush, name='flush')

    @click.group(cls=AliasedGroup)
    def ndp():
        ''' Show NDP information '''
        pass

    @click.command()
    @click.pass_obj
    def _table(cli_opts):
        ''' Show the NDP table '''
        ndp.NdpTableCmd(cli_opts).run()

    @click.command()
    @click.option('-V', '--vlan', type=int, default=0,
                    help='Only flush the IP from the specified VLAN')
    @click.argument('ip')
    @click.pass_obj
    def _flush(cli_opts, ip, vlan):
        ''' Flush an NDP entry '''
        cmds.NeighborFlushCmd(cli_opts).run(ip, vlan)

class PortType(click.ParamType):
    port_info_map = None

    def convert(self, value, param, ctx):
        try:
            if self.port_info_map is None:
                client = FbossAgentClient(ctx.obj.hostname)
                self.port_info_map = client.getAllPortInfo()
            if value.isdigit():
                port = self.port_info_map[int(value)]
                return port.portId
            for port_id, port_info in self.port_info_map.items():
                if port_info.name == value:
                    return port_id
            raise ValueError("No port found with that name")

        except (ValueError, KeyError):
            self.fail('%s is not a valid Port' % value, param, ctx)

class PortCli(object):
    ''' Port sub-commands '''

    def __init__(self):
        self.port.add_command(self._details, name='details')
        self.port.add_command(self._flap, name='flap')
        self.port.add_command(self._status, name='status')
        self.port.add_command(self._enable, name='enable')
        self.port.add_command(self._disable, name='disable')
        self.port.add_command(self._stats, name='stats')

    @click.group(cls=AliasedGroup)
    def port():
        ''' Show port information '''
        pass

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.pass_obj
    def _details(cli_opts, ports):
        ''' Show port details for given [port(s)] '''
        port.PortDetailsCmd(cli_opts).run(ports)

    @click.command()
    @click.argument('ports', nargs=-1, required=True, type=PortType())
    @click.pass_obj
    def _flap(cli_opts, ports):
        ''' Flap given [port(s)] '''
        port.PortFlapCmd(cli_opts).run(ports)

    @click.command()
    @click.argument('ports', nargs=-1, required=True, type=PortType())
    @click.pass_obj
    def _enable(cli_opts, ports):
        ''' Enable given [port(s)] '''
        port.PortSetStatusCmd(cli_opts).run(ports, True)

    @click.command()
    @click.argument('ports', nargs=-1, required=True, type=PortType())
    @click.pass_obj
    def _disable(cli_opts, ports):
        ''' Disable given [port(s)] '''
        port.PortSetStatusCmd(cli_opts).run(ports, False)

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.option('--detail', is_flag=True, help='Display detailed port status')
    @click.option('--internal', is_flag=True,
                  help='Display all ports info with internal ID')
    @click.option('-v', '--verbose', is_flag=True,
                    help='Show flags and thresholds as well as details')
    @click.pass_obj
    def _status(cli_opts, detail, ports, verbose, internal):
        ''' Show port status '''
        port.PortStatusCmd(cli_opts).run(detail, ports, verbose, internal)

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.option('--detail',
                  is_flag=True,
                  help='Display detailed port stats with lldp')
    @click.pass_obj
    def _stats(cli_opts, detail, ports):
        ''' Show port statistics '''
        port.PortStatsCmd(cli_opts).run(detail, ports)


class ProductInfoCli(object):
    ''' Product Info sub-commands '''

    @click.command()
    @click.option('-d', '--detail', is_flag=True,
                        help='Display detailed product information')
    @click.pass_obj
    def product(cli_opts, detail):
        ''' Show product information '''
        info.ProductInfoCmd(cli_opts).run(detail)


class ReloadConfigCli(object):
    ''' Reload config sub-commands '''

    @click.command()
    @click.pass_obj
    def reloadconfig(cli_opts):
        ''' Reload agent configuration file  '''
        config.ReloadConfigCmd(cli_opts).run()


class RouteCli(object):
    ''' Route sub-commands '''

    def __init__(self):
        self.route.add_command(self._ip, name='ip')
        self.route.add_command(self._table, name='table')
        self.route.add_command(self._details, name='details')
        self.route.add_command(self._add, name='add')
        self.route.add_command(self._delete, name='delete')
        self.route.add_command(self._flush, name='flush')

    @click.group(cls=AliasedGroup)
    def route():
        ''' Show route information '''
        pass

    @click.command()
    @click.argument('ip')
    @click.option('-v', '--vrf', type=int, default=0,
                    help='Show Route to the IP address in the vrf')
    @click.pass_obj
    def _ip(cli_opts, ip, vrf):
        ''' Show the route to a specific IP '''
        route.RouteIpCmd(cli_opts).run(ip, vrf)

    @click.command()
    @click.option('--client-id', type=int, default=None,
                  help='If pass, show all routes programmed by certain client')
    @click.option('-4', '--ipv4', is_flag=True, default=False,
                  help="Show IPv4 routes")
    @click.option('-6', '--ipv6', is_flag=True, default=False,
                  help="Show IPv6 routes")
    @click.pass_obj
    def _table(cli_opts, client_id, ipv4, ipv6):
        ''' Show the route table '''
        route.RouteTableCmd(cli_opts).run(client_id, ipv4, ipv6)

    @click.command()
    @click.option('-4', '--ipv4', is_flag=True, default=False,
                  help="Show IPv4 routes")
    @click.option('-6', '--ipv6', is_flag=True, default=False,
                  help="Show IPv6 routes")
    @click.pass_obj
    def _details(cli_opts, ipv4, ipv6):
        ''' Show details of the route table '''
        route.RouteTableDetailsCmd(cli_opts).run(ipv4, ipv6)

    @click.command()
    @click.argument('prefix', nargs=1, required=True)
    @click.argument('nexthop', nargs=-1, required=True)
    @click.option('-c', '--client-id', type=int, default=DEFAULT_CLIENTID,
                  help='The client ID used to manipulate the routes')
    @click.option('-d', '--admin-distance', type=int, default=None,
                  help='DIRECTLY_CONNECTED=0, STATIC_ROUTE=1, '
                        'EBGP=20, IBGP=200, '
                        'MAX_ADMIN_DISTANCE=255')
    @click.pass_obj
    def _add(cli_opts, client_id, admin_distance, prefix, nexthop):
        '''
        Add a new route or change an existing route

        PREFIX - The route prefix, i.e. "1.1.1.0/24" or "2001::0/64\n
        NEXTHOP - The nexthops of the route, i.e "10.1.1.1" or "2002::1"
        '''
        route.RouteAddCmd(cli_opts).run(client_id, admin_distance, prefix,
                                        nexthop)

    @click.command()
    @click.option('-c', '--client-id', type=int, default=DEFAULT_CLIENTID,
                  help='The client ID used to manipulate the routes')
    @click.argument('prefix')
    @click.pass_obj
    def _delete(cli_opt, client_id, prefix):
        '''
        Delete an existing route

        PREFIX - The route prefix, i.e. "1.1.1.0/24" or "2001::0/64"
        '''
        route.RouteDelCmd(cli_opt).run(client_id, prefix)

    @click.command()
    @click.option('-c', '--client-id', type=int, default=DEFAULT_CLIENTID,
                  help='The client ID used to manipulate the routes')
    @click.pass_obj
    def _flush(cli_opt, client_id):
        '''Flush all existing non-interface routes'''
        route.RouteFlushCmd(cli_opt).run(client_id)


# -- Main Command Group -- #
@click.group(cls=AliasedGroup)
@click.option('--hostname', '-H', default='::1',
        type=str, help='Host to connect to (default = ::1)')
@click.option('--file', '-F', default=None,
        type=str, help='Snapshot file to read from')
@click.option('--port', '-p', default=None,
        type=int, help='Thrift port to connect to')
@click.option('--timeout', '-t', default=None,
        type=int, help='Thrift client timeout in seconds')
@click.pass_context
def main(ctx, hostname, file, port, timeout):
    ''' Main CLI options, all options are passed to children via the context obj
        "ctx" and can be accessed accordingly '''
    ctx.obj = CliOptions(hostname, file, port, timeout)


def add_modules(main_func):
    ''' Add sub-commands to main '''

    main_func.add_command(ArpCli().arp)
    main_func.add_command(GetConfigCli().config)
    main_func.add_command(IpCli().ip)
    main_func.add_command(InterfaceCli().interface)
    main_func.add_command(L2Cli().l2)
    main_func.add_command(LldpCli().lldp)
    main_func.add_command(NdpCli().ndp)
    main_func.add_command(NicCli().nic)
    main_func.add_command(PortCli().port)
    main_func.add_command(ProductInfoCli().product)
    main_func.add_command(ReloadConfigCli().reloadconfig)
    main_func.add_command(RouteCli().route)

if __name__ == '__main__':

    # -- Add sub-commands to "Main" -- #
    add_modules(main)

    try:
        main()
    except FbossBaseError as e:
        raise SystemExit('Fboss Error: {}'.format(e))
    except TApplicationException:
        raise SystemExit("Command not available on host")
    except TTransportException:
        raise SystemExit('Failed connecting to host')
