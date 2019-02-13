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
import ipaddress

from fboss.cli.commands import arp
from fboss.cli.commands import aggregate_port
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
from fboss.cli.commands import agent
from fboss.cli.commands.commands import FlushType
from fboss.cli.utils.utils import AGENT_KEYWORD
from fboss.cli.utils.utils import KEYWORD_CONFIG_SHOW, KEYWORD_CONFIG_RELOAD
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
        ''' Flush an ARP entry by [IP] or [subnet] or flush [all]'''
        if ip == 'all':
            ip = '0.0.0.0/0'
        cmds.NeighborFlushSubnetCmd(cli_opts).run(FlushType.arp,
                ipaddress.IPv4Network(ip), vlan)


class AggregatePortCli(object):
    ''' Aggregate Port sub-commands '''
    @click.command()
    @click.argument('port', required=False, default="")
    @click.pass_obj
    def aggregate_port(cli_opts, port):
        ''' Show aggregate port information; Outputs a list of
            aggregate port and the subports that are part of the
            aggregate port.'''
        aggregate_port.AggregatePortCmd(cli_opts).run(port)


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
        raise Exception(''' D13645809 reorganized commands:
            fboss config agent => fboss agent config show ''')


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

    def __init__(self):
        self.interface.add_command(self._show, name='show')
        self.interface.add_command(self._summary, name='summary')

    @click.group(cls=AliasedGroup)
    def interface():
        ''' Show Interface Information '''
        pass

    @click.command()
    @click.argument('interfaces', type=int, nargs=-1)
    @click.pass_obj
    def _show(cli_opts, interfaces):
        ''' Show interface information for Interface(s); Outputs a list of
            interfaces on host if no interfaces are specified '''
        interface.InterfaceShowCmd(cli_opts).run(interfaces)

    @click.command()
    @click.pass_obj
    def _summary(cli_opts):
        ''' Show interface summary '''
        interface.InterfaceSummaryCmd(cli_opts).run()


class L2Cli(object):
    ''' Show L2 sub-commands '''

    def __init__(self):
        self.l2.add_command(self._table, name='table')

    @click.group(cls=AliasedGroup)
    def l2():
        ''' Show L2 information '''
        pass

    @click.command()
    @click.pass_obj
    def _table(cli_opts):
        ''' Show the L2 table '''
        l2.L2TableCmd(cli_opts).run()


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
        ''' Flush an NDP entry by [IP] or [subnet] or flush [all]'''
        if ip == 'all':
            ip = '::/0'
        cmds.NeighborFlushSubnetCmd(cli_opts).run(FlushType.ndp,
                ipaddress.IPv6Network(ip), vlan)


class PortType(click.ParamType):
    port_info_map = None

    def convert(self, value, param, ctx):
        try:
            if self.port_info_map is None:
                with FbossAgentClient(ctx.obj.hostname) as client:
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


class Stats(object):
    ''' Port stats sub-commands '''

    def __init__(self):
        self.stats.add_command(self._show, name='show')
        self.stats.add_command(self._clear, name='clear')

    @click.group(cls=AliasedGroup)
    def stats():
        ''' Show/clear Port stats '''
        pass

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.option('--detail',
                  is_flag=True,
                  help='Display detailed port stats with lldp')
    @click.pass_obj
    def _show(cli_opts, detail, ports):
        ''' Show port statistics '''
        port.PortStatsCmd(cli_opts).run(detail, ports)

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.pass_obj
    def _clear(cli_opts, ports):
        ''' Clear stats'''
        port.PortStatsClearCmd(cli_opts).run(ports)


class PortState(object):
    ''' Port state sub-commands '''

    def __init__(self):
        self.state.add_command(self._show, name='show')
        self.state.add_command(self._enable, name='enable')
        self.state.add_command(self._disable, name='disable')
        self.state.add_command(self._flap, name='flap')

    @click.group(cls=AliasedGroup)  # noqa: B902
    def state():
        ''' Port state commands '''
        pass

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.option('--all', is_flag=True, help='Display Disabled ports')
    @click.pass_obj
    def _show(cli_opts, ports, all):  # noqa: B902
        ''' Show port state for given [port(s)] '''
        port.PortStatusCmd(cli_opts).run(detail=False, ports=ports,
                verbose=False, internal=True, all=all)

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.pass_obj
    def _enable(cli_opts, ports):  # noqa: B902
        ''' Enable port state for given [port(s)] '''
        port.PortSetStatusCmd(cli_opts).run(ports, True)

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.pass_obj
    def _disable(cli_opts, ports):  # noqa: B902
        ''' Disable port state for given [port(s)] '''
        port.PortSetStatusCmd(cli_opts).run(ports, False)

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.option('--all', is_flag=True, help='Flap all Present but Down ports')
    @click.pass_obj
    def _flap(cli_opts, ports, all):  # noqa: B902
        ''' Flap port state for given [port(s)] '''
        port.PortFlapCmd(cli_opts).run(ports, all)


class PortTransceiver(object):
    ''' Port transceiver sub-commands '''

    def __init__(self):
        self.transceiver.add_command(self._transceiver, name='show')

    @click.group(cls=AliasedGroup)  # noqa: B902
    def transceiver():
        ''' Port transceiver commands '''
        pass

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.pass_obj
    def _transceiver(cli_opts, ports):  # noqa: B902
        ''' Show port transceiver for given [port(s)] '''
        port.PortStatusCmd(cli_opts).run(detail=False, ports=ports,
                verbose=True, internal=False, all=all)


def raise_exception_port_command_reorg():
    raise Exception(''' D13745157 reorganized/renamed commands:

        fboss port status [--all]       => fboss port state show [--all]
        fboss port enable               => fboss port state enable
        fboss port disable              => fboss port state disable
        fboss port flap [--all]         => fboss port state flap [--all]

        fboss port status --verbose      => fboss port transceiver show  ''')


class PortCli(object):
    ''' Port sub-commands '''

    def __init__(self):
        self.port.add_command(self._details, name='details')
        self.port.add_command(self._flap, name='flap')
        self.port.add_command(self._status, name='status')
        self.port.add_command(self._enable, name='enable')
        self.port.add_command(self._disable, name='disable')
        self.port.add_command(self._description, name='description')

        self.port.add_command(PortState().state)
        self.port.add_command(PortTransceiver().transceiver)
        self.port.add_command(Stats().stats)

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
    @click.argument('ports', nargs=-1, type=PortType())
    @click.option('--all', is_flag=True, help='Flap all Present but Down ports')
    @click.pass_obj
    def _flap(cli_opts, ports, all):
        ''' Flap given [port(s)] '''
        raise_exception_port_command_reorg()

    @click.command()
    @click.argument('ports', nargs=-1, required=True, type=PortType())
    @click.pass_obj
    def _enable(cli_opts, ports):
        ''' Enable given [port(s)] '''
        raise_exception_port_command_reorg()

    @click.command()
    @click.argument('ports', nargs=-1, required=True, type=PortType())
    @click.pass_obj
    def _disable(cli_opts, ports):
        ''' Disable given [port(s)] '''
        raise_exception_port_command_reorg()

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.option('--detail', is_flag=True, help='Display detailed port status')
    @click.option('--all', is_flag=True, help='Display Disabled ports')
    @click.option('--internal', is_flag=True,
                  help='Display all ports info with internal ID')
    @click.option('-v', '--verbose', is_flag=True,
                    help='Show flags and thresholds as well as details')
    @click.pass_obj
    def _status(cli_opts, detail, ports, verbose, internal, all):
        ''' Show port status '''
        raise_exception_port_command_reorg()

    @click.command()
    @click.argument('ports', nargs=-1, type=PortType())
    @click.pass_obj
    def _description(cli_opts, ports):
        ''' Show port description for given [port(s)] '''
        port.PortDescriptionCmd(cli_opts).run(ports)


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
        raise Exception(''' D13645809 reorganized commands:
            fboss reloadconfig => fboss agent config reload ''')


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


class VerbosityCli(object):
    '''Change the verbosity of the fboss agent'''

    @click.command(name='verbosity')
    @click.argument('verbosity')
    @click.pass_obj
    def set(cli_opt, verbosity):
        cmds.VerbosityCmd(cli_opt).run(verbosity)


class AgentConfig(object):
    ''' Agent config sub-commands '''

    def __init__(self):
        self.config.add_command(self._show, name='show')
        self.config.add_command(self._reload, name='reload')

    @click.group(cls=AliasedGroup)  # noqa: B902
    def config():
        ''' Agent config commands'''
        pass

    @click.command()
    @click.pass_obj
    def _show(cli_opts):  # noqa: B902
        ''' Show running config '''
        agent.AgentConfigCmd(cli_opts).run(KEYWORD_CONFIG_SHOW)

    @click.command()
    @click.pass_obj
    def _reload(cli_opts):  # noqa: B902
        ''' Reload agent configuration file '''
        agent.AgentConfigCmd(cli_opts).run(KEYWORD_CONFIG_RELOAD)


class AgentCli(object):
    ''' Agent sub-commands '''

    def __init__(self):
        self.agent.add_command(AgentConfig().config)

    @click.group(cls=AliasedGroup)  # noqa: B902
    def agent():
        ''' agent commands '''
        pass


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
    main_func.add_command(AggregatePortCli().aggregate_port)
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
    main_func.add_command(VerbosityCli().set)
    main_func.add_command(AgentCli().agent)

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
