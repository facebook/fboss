# TODO: Opensource jargon here
# @lint-avoid-python-3-compatibility-imports

import binascii
import json
import string
import time


from collections import defaultdict
# TODO: Are these thrift structs open-sourced?
from facebook.network.Address.ttypes import Address, AddressType
# TODO: Can we use click to get the colored output here?
# TODO: This is a duplicate of neteng/fboss/tools/cli/utils
from fboss.cli import utils
# TODO: move code into this module
from fboss.cli import cli_common
from fboss.thrift_clients import FbossAgentClient
from neteng.fboss.optic import ttypes as optic_ttypes
from neteng.fboss.ttypes import FbossBaseError
from thrift.Thrift import TApplicationException


class FbossCmd(object):
    def __init__(self, cli_opts):
        ''' initialize; client will be created in subclasses with the specific
            client they need '''

        self._hostname = cli_opts.hostname
        self._port = cli_opts.port
        self._timeout = cli_opts.timeout
        self._client = None

    def _create_ctrl_client(self):
        args = [self._hostname, self._port]
        if self._timeout:
            args.append(self._timeout)

        return FbossAgentClient(*args)


class InterfaceCmd(FbossCmd):
    ''' Show interface information '''
    def run(self, interfaces):
        try:
            self._client = self._create_ctrl_client()
            if not interfaces:
                self._all_interface_info()
            else:
                for interface in interfaces:
                    self._interface_details(interface)
        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + e)

    def _all_interface_info(self):
        resp = self._client.getInterfaceList()
        if not resp:
            print("No Interfaces Found")
            return
        for entry in resp:
            print(entry)

    def _interface_details(self, interface):
        resp = self._client.getInterfaceDetail(interface)
        if not resp:
            print("No interface details found for interface")
            return

        print("%s\tInterface ID: %d" %
                            (resp.interfaceName, resp.interfaceId))
        print("  Vlan: %d\t\t\tRouter Id: %d" % (resp.vlanId, resp.routerId))
        print("  Mac Address: %s" % resp.mac)
        print("  IP Address:")
        for addr in resp.address:
            print("\t%s/%d" % (utils.ip_ntop(addr.ip.addr), addr.prefixLength))


class IpCmd(FbossCmd):
    def run(self, interface):
        self._client = self._create_ctrl_client()
        resp = self._client.getInterfaceDetail(interface)
        if not resp:
            print("No interface details found for interface")
            return
        print("\nAddress:")
        for addr in resp.address:
            print("\t%s/%d" % (utils.ip_ntop(addr.ip.addr), addr.prefixLength))


class LldpCmd(FbossCmd):
    def run(self, lldp_port, verbosity):
        self._client = self._create_ctrl_client()
        resp = self._client.getLldpNeighbors()
        if not resp:
            print("No neighbors found")
            return

        headers = {
            'local_port': 'Local Port',
            'local_vlan': 'Local VLAN',
            'mac': 'MAC',
            'chassis': 'Chassis ID',
            'raw_chassis': 'Raw Chassis ID',
            'chassis_id_type': 'Chassis ID Type',
            'name': 'Name',
            'system': 'System Name',
            'port': 'Port',
            'raw_port': 'Raw Port ID',
            'port_id_type': 'Port ID Type',
            'system_desc': 'System Description',
            'port_desc': 'Port Description',
            'ttl': 'TTL',
            'ttl_left': 'TTL Left',
        }
        max_widths = dict((key, len(value) + 1)
                          for key, value in headers.items())

        entries = []
        for neighbor in resp:
            if (lldp_port and not neighbor.localPort == lldp_port):
                continue

            fields = self._get_fields(neighbor)
            for key, value in fields.items():
                if len(str(value)) > max_widths[key]:
                    max_widths[key] = len(value)
            entries.append(fields)

        if not verbosity:
            selected = ['local_port', 'name', 'port']
            self._print_fields(selected, entries, headers, max_widths)
        elif verbosity == 1:
            selected = ['local_port', 'chassis', 'system', 'port', 'ttl',
                        'ttl_left']
            self._print_fields(selected, entries, headers, max_widths)
        else:
            self._print_verbose(entries, headers)

    def _print_fields(self, selected, fields, headers, max_widths):
        fmt = ' '.join('{{{}:<{}}}'.format(key, max_widths[key])
                       for key in selected)

        header = fmt.format(**headers)
        print(header)
        print('-' * len(header))

        for entry in fields:
            line = fmt.format(**entry)
            print(line)

    def _print_verbose(self, entries, headers):
        for entry in entries:
            print('Neighbor:')
            print('  Local Port: {}'.format(entry['local_port']))
            print('  Local VLAN: {}'.format(entry['local_vlan']))
            print('  Original TTL: {}'.format(entry['ttl']))
            print('  TTL Left: {}'.format(entry['ttl_left']))
            print('  Source MAC: {}'.format(entry['mac']))
            print('  Chassis ID Type: {}'.format(entry['chassis_id_type']))

            raw_chassis = entry['raw_chassis']
            if self.is_printable(raw_chassis):
                print('  Chassis ID: {}'.format(raw_chassis))
            else:
                rc = binascii.hexlify(raw_chassis)
                print('  Raw Chassis ID: hex({})'.format(rc))
                print('  Chassis ID: {}'.format(entry['chassis']))

            raw_port = entry['raw_port']
            if self.is_printable(raw_port):
                print('  Port ID: {}'.format(raw_port))
            else:
                rc = binascii.hexlify(raw_port)
                print('  Raw Port ID: hex({})'.format(rc))
                print('  Port ID: {}'.format(entry['port']))

            print('  System Name: {}'.format(entry['system']))

            desc = entry['system_desc']
            if '\n' in desc:
                desc = '\n    '.join(desc.splitlines())
                print('  System Description:\n    {}'.format(desc))
            else:
                print('  System Description: {}'.format(desc))
            print('  Port Description: {}'.format(entry['port_desc']))
            print()

    def is_printable(self, value):
        return all(c in string.printable for c in value)

    def _get_fields(self, neighbor):
        fields = {}
        fields['local_port'] = neighbor.localPort
        fields['local_vlan'] = neighbor.localVlan
        fields['mac'] = neighbor.srcMac
        fields['chassis'] = neighbor.printableChassisId
        fields['raw_chassis'] = neighbor.chassisId
        fields['chassis_id_type'] = neighbor.chassisIdType
        fields['system'] = neighbor.systemName
        fields['port'] = neighbor.printablePortId
        fields['raw_port'] = neighbor.portId
        fields['port_id_type'] = neighbor.portIdType
        if neighbor.systemName is None:
            fields['name'] = neighbor.printableChassisId
        else:
            fields['name'] = neighbor.systemName
        fields['system_desc'] = neighbor.systemDescription or ''
        fields['port_desc'] = neighbor.portDescription or ''
        fields['ttl'] = neighbor.originalTTL
        fields['ttl_left'] = neighbor.ttlSecondsLeft
        return fields


class RouteIpCmd(FbossCmd):
    def run(self, ip, vrf):
        addr = Address(addr=ip, type=AddressType.V4)
        if not addr.addr:
            print('No ip address provided')
            return
        self._client = self._create_ctrl_client()
        resp = self._client.getIpRoute(addr, vrf)
        print('Route to ' + addr.addr + ', Vrf: %d' % vrf)
        netAddr = utils.ip_ntop(resp.dest.ip.addr)
        prefix = resp.dest.prefixLength
        if netAddr == '0.0.0.0' or netAddr == '::':
            print('No Route to destination')
        else:
            print('N/w: %s/%d' % (netAddr, prefix))
            for nexthops in resp.nextHopAddrs:
                print('\t\tvia: ' + utils.ip_ntop(nexthops.addr))


class RouteTableCmd(FbossCmd):
    def run(self):
        self._client = self._create_ctrl_client()
        resp = self._client.getRouteTable()
        if not resp:
            print("No Route Table Entries Found")
            return
        for entry in resp:
            print("Network Address: %s/%d" %
                                (utils.ip_ntop(entry.dest.ip.addr),
                                            entry.dest.prefixLength))
            # Need to check the nextHopAddresses
            for nhop in entry.nextHopAddrs:
                print("\tvia %s" % utils.ip_ntop(nhop.addr))


class ArpTableCmd(FbossCmd):
    def run(self):
        self._client = self._create_ctrl_client()
        resp = self._client.getArpTable()
        _print_neighbor_table(resp, 'ARP', 16)


def _print_neighbor_table(resp, name, width):
    if not resp:
        print("No {} Entries Found".format(name))
        return

    tmpl = "{:" + str(width) + "} {:18} {:>4}  {}"
    print(tmpl.format("IP Address", "MAC Address", "Port", "VLAN"))
    for entry in resp:
        ip = utils.ip_ntop(entry.ip.addr)
        vlan_field = '{} ({})'.format(entry.vlanName, entry.vlanID)
        print(tmpl.format(ip, entry.mac, entry.port, vlan_field))


class NeighborFlushCmd(FbossCmd):
    def run(self, ip, vlan):
        bin_ip = utils.ip_to_binary(ip)
        vlan_id = vlan
        client = self._create_ctrl_client()
        num_entries = client.flushNeighborEntry(bin_ip, vlan_id)
        print('Flushed {} entries'.format(num_entries))


class PortDetailsCmd(FbossCmd):
    def run(self, ports):
        try:
            self._client = self._create_ctrl_client()
            if not ports:
                self.all_port_details(ports)
            else:
                for port in ports:
                    cli_common.print_port_details(self._client, port)
        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + e)
        except Exception as e:
            raise Exception('Error: ' + e)

    def all_port_details(self, ports):
        resp = self._client.getAllPortInfo()
        if not resp:
            print("No Ports Found")
            return
        for port, entry in resp.items():
            if entry.operState == 1:
                cli_common.print_port_details(self._client, port, entry)


class PortFlapCmd(FbossCmd):
    def run(self, ports):
        try:
            if not ports:
                raise FbossBaseError()
            else:
                self.flap_ports(ports)
        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + e)

    def flap_ports(self, ports):
        self._client = self._create_ctrl_client()
        resp = self._client.getPortStatus(ports)
        for port, status in resp.items():
            if not status.enabled:
                print("Port %d is disabled by configuration, cannot flap" %
                      (port))
                continue
            print("Disabling port %d" % (port))
            self._client.setPortState(port, False)
        time.sleep(1)
        for port, status in resp.items():
            if status.enabled:
                print("Enabling port %d" % (port))
                self._client.setPortState(port, True)


class PortStatusCmd(FbossCmd):
    def run(self, detail, ports, verbose):
        if detail or verbose:
            self._client = self._create_ctrl_client()
            PortStatusDetailCmd(
                    self._client, ports, verbose).get_detail_status()
        else:
            self.list_ports(ports)

    def list_ports(self, ports):
        self._client = self._create_ctrl_client()
        field_fmt = '{:>6}  {:<15} {:<12}{} {:<12}'
        print(field_fmt.format('Port', 'Admin Status', 'Link Status', '',
              'Transceiver Present'))
        print('-' * 56)
        resp = self._client.getPortStatus(ports)
        for port, status in sorted(resp.items()):
            attrs = self._get_status_strs(status)
            print(field_fmt.format(port, attrs['admin_status'],
                attrs['link_status'], attrs['color_align'], attrs['present']))

    @staticmethod
    def _get_status_strs(status):
        ''' Get port status attributes '''

        attrs = {}
        admin_status = "Enabled"
        link_status = "Up"
        present = "Present"

        color_start = utils.COLOR_GREEN
        color_end = utils.COLOR_RESET
        if not status.enabled:
            admin_status = "Disabled"
        if not status.up:
            link_status = "Down"
            if status.enabled and status.present:
                color_start = utils.COLOR_RED
            else:
                color_start = ''
                color_end = ''
        if status.present is None:
            present = "Unknown"
        elif not status.present:
            present = "Not Present"
        link_status = color_start + link_status + color_end
        color_align = ' ' * (len(color_start) + len(color_end))

        attrs['admin_status'] = admin_status
        attrs['link_status'] = link_status
        attrs['color_align'] = color_align
        attrs['present'] = present

        return attrs


class PortStatusDetailCmd(object):
    ''' Print detailed/verbose port status '''

    def __init__(self, client, ports, verbose):
        self._client = client
        self._ports = ports
        self._port_speeds = self._get_port_speeds()
        self._info_resp = None
        self._status_resp = self._client.getPortStatus(ports)
        # map of { transceiver_id -> { channel_id -> port } }
        self._t_to_p = defaultdict(dict)
        self._transceiver = []
        self._verbose = verbose

    def _get_port_speeds(self):
        ''' Get speeds for all ports '''

        all_info = self._client.getAllPortInfo()
        return dict((p, info.speedMbps) for p, info in all_info.items())

    def _get_port_channels(self, port, xcvr_info):
        '''  This function handles figuring out correct channel info even for
             older controllers that don't return full channel info. '''

        start_channel = xcvr_info.channelId
        speed = self._port_speeds[port]

        # speed == 1000 and N/A are one channel
        channels = [start_channel]
        if speed == 20000:
            channels = range(start_channel, start_channel + 2)
        elif speed == 40000:
            channels = range(start_channel, start_channel + 4)

        return channels

    def _get_channel_detail(self, port, status):
        ''' Get channel detail for port '''

        channels = status.transceiverIdx.channels
        if not channels:
            channels = self._get_port_channels(
                        port, status.transceiverIdx)

        tid = status.transceiverIdx.transceiverId
        for ch in channels:
            self._t_to_p[tid][ch] = port

        if tid not in self._transceiver:
            self._transceiver.append(tid)

    def _print_port_details_sfpdom(self, port, dom):
        status = self._status_resp[port]
        print("Port %d: %s" % (port, dom.name))

        attrs = PortStatusCmd._get_status_strs(status)
        admin_status = attrs['admin_status']
        link_status = attrs['link_status']

        print("  Admin Status: %s" % admin_status)
        print("  Oper Status: %s" % link_status)

        print("  Module Present: %s" % dom.sfpPresent)

        if dom.sfpPresent and dom.vendor is not None:
            print("  Vendor Name: %s" % dom.vendor.name)
            print("  Part Number: %s" % dom.vendor.partNumber)
            print("  Revision: %s" % dom.vendor.rev)
            print("  Serial Number: %s" % dom.vendor.serialNumber)
            print("  Date Code: %s" % dom.vendor.dateCode)

        print("  Monitoring Information:")
        if not dom.domSupported:
            print("    DOM Not Supported")
            return

        print("    Values:")
        print("      {:<15} {:0.4}".format("Temperature", dom.value.temp))
        print("      {:<15} {:0.4}".format("Vcc", dom.value.vcc))
        print("      {:<15} {:0.4}".format("Tx Bias", dom.value.txBias))
        print("      {:<15} {:0.4}".format("Tx Power(dBm)",
                utils.mw_to_dbm(dom.value.txPwr)))
        print("      {:<15} {:0.4}".format("Rx Power(dBm)",
                utils.mw_to_dbm(dom.value.rxPwr)))

        print("    {:<14}   {:>15} {:>15} {:>15} {:>15}".format(
                'Flags:',
                'Alarm Low', 'Warning Low', 'Warning High', 'Alarm High'))
        print("      {:<14} {:>15} {:>15} {:>15} {:>15}".format(
                'Temperature:',
                dom.flags.tempAlarmLow, dom.flags.tempWarnLow,
                dom.flags.tempWarnHigh, dom.flags.tempAlarmHigh))
        print("      {:<14} {:>15} {:>15} {:>15} {:>15}".format(
                'Vcc:',
                dom.flags.vccAlarmLow, dom.flags.vccWarnLow,
                dom.flags.vccWarnHigh, dom.flags.vccAlarmHigh))
        print("      {:<14} {:>15} {:>15} {:>15} {:>15}".format(
                'Tx Bias:',
                dom.flags.txBiasAlarmLow, dom.flags.txBiasWarnLow,
                dom.flags.txBiasWarnHigh, dom.flags.txBiasAlarmHigh))
        print("      {:<14} {:>15} {:>15} {:>15} {:>15}".format(
                'Tx Power(dBm):',
                utils.mw_to_dbm(dom.flags.txPwrAlarmLow),
                utils.mw_to_dbm(dom.flags.txPwrWarnLow),
                utils.mw_to_dbm(dom.flags.txPwrWarnHigh),
                utils.mw_to_dbm(dom.flags.txPwrAlarmHigh)))
        print("      {:<14} {:>15} {:>15} {:>15} {:>15}".format(
                'Rx Power(dBm):',
                utils.mw_to_dbm(dom.flags.rxPwrAlarmLow),
                utils.mw_to_dbm(dom.flags.rxPwrWarnLow),
                utils.mw_to_dbm(dom.flags.rxPwrWarnHigh),
                utils.mw_to_dbm(dom.flags.rxPwrAlarmHigh)))

        thresh = dom.threshValue
        print("  {:<16}   {:>15} {:>15} {:>15} {:>15}".format(
                'Thresholds:',
                'Alarm Low', 'Warning Low', 'Warning High', 'Alarm High'))
        print("      {:<14} {:>15.4} {:>15.4} {:>15.4} {:>15.4}".format(
                'Temperature:',
                thresh.tempAlarmLow, thresh.tempWarnLow,
                thresh.tempWarnHigh, thresh.tempAlarmHigh))
        print("      {:<14} {:>15.4} {:>15.4} {:>15.4} {:>15.4}".format(
                'Vcc:',
                thresh.vccAlarmLow, thresh.vccWarnLow,
                thresh.vccWarnHigh, thresh.vccAlarmHigh))
        print("      {:<14} {:>15.4} {:>15.4} {:>15.4} {:>15.4}".format(
                'Tx Bias:',
                thresh.txBiasAlarmLow, thresh.txBiasWarnLow,
                thresh.txBiasWarnHigh, thresh.txBiasAlarmHigh))
        print("      {:<14} {:>15.4} {:>15.4} {:>15.4} {:>15.4}".format(
                'Tx Power(dBm):',
                utils.mw_to_dbm(thresh.txPwrAlarmLow),
                utils.mw_to_dbm(thresh.txPwrWarnLow),
                utils.mw_to_dbm(thresh.txPwrWarnHigh),
                utils.mw_to_dbm(thresh.txPwrAlarmHigh)))
        print("      {:<14} {:>15.4} {:>15.4} {:>15.4} {:>15.4}".format(
                'Rx Power(dBm):',
                utils.mw_to_dbm(thresh.rxPwrAlarmLow),
                utils.mw_to_dbm(thresh.rxPwrWarnLow),
                utils.mw_to_dbm(thresh.rxPwrWarnHigh),
                utils.mw_to_dbm(thresh.rxPwrAlarmHigh)))

    def _list_ports_detail_sfpdom(self):
        ''' Print ports detail based on Sfp DOM info '''

        dom_resp = self._client.getSfpDomInfo(self._ports)
        for port in self._status_resp.keys():
            if port not in dom_resp:
                sfp_dom = optic_ttypes.SfpDom()
                sfp_dom.name = 'Ethernet%d' % port
                sfp_dom.sfpPresent = False
                sfp_dom.domSupported = False
                dom_resp[port] = sfp_dom

        for port in self._status_resp.keys():
            self._print_port_details_sfpdom(port, dom_resp[port])

    def _get_dummy_status(self):
        ''' Get dummy status for ports without data '''

        for port, status in sorted(self._status_resp.items()):
            if status.transceiverIdx:
                tid = status.transceiverIdx.transceiverId
                if tid not in self._info_resp.keys():
                    info = optic_ttypes.TransceiverInfo()
                    info.port = port
                    info.present = False
                    self._info_resp[port] = info

    def _print_transceiver_ports(self, ch_to_port, info):
        # Print port info if the transceiver doesn't have any
        for port in ch_to_port.values():
            attrs = PortStatusCmd._get_status_strs(self._status_resp[port])
            print("Port: {:>2}  Status: {:<8}  Link: {:<4}  Transceiver: {}"
              .format(port, attrs['admin_status'], attrs['link_status'],
                        attrs['present']))

    def _print_vendor_details(self, info):
        ''' print vendor details '''

        print("Vendor:  {:<16}  Part Number:  {:<16}".format(
              info.vendor.name, info.vendor.partNumber))
        print("Serial:  {:<16}  ".format(info.vendor.serialNumber), end="")
        print("Date Code:  {:<8}  Revision: {:<2}".format(
              info.vendor.dateCode, info.vendor.rev))

    def _print_cable_details(self, info):
        ''' print cable details '''

        print("Cable:", end=""),
        if info.cable.singleModeKm:
            print("Single Mode:  {}km".format(
                  info.cable.singleModeKm % 1000), end=""),
        if info.cable.singleMode:
            print("Single Mode:  {}m".format(
                  info.cable.singleMode), end=""),
        if info.cable.om3:
            print("OM3:  {}m".format(info.cable.om3), end=""),
        if info.cable.om2:
            print("OM2:  {}m".format(info.cable.om2), end=""),
        if info.cable.om1:
            print("OM1:  {}m".format(info.cable.om1), end=""),
        if info.cable.copper:
            print("Copper:  {}m".format(info.cable.copper), end="")
        print("")

    def _print_thresholds(self, thresh):
        ''' print threshold details '''

        print("  {:<16}   {:>10} {:>15} {:>15} {:>10}".format(
                'Thresholds:',
                'Alarm Low', 'Warning Low', 'Warning High', 'Alarm High'))
        print("    {:<14} {:>9.4}C {:>14.4}C {:>14.4}C {:>9.4}C".format(
                'Temp:',
                thresh.temp.alarm.low, thresh.temp.warn.low,
                thresh.temp.warn.high, thresh.temp.alarm.high))
        print("    {:<14} {:>10.4} {:>15.4} {:>15.4} {:>10.4}".format(
                'Vcc:',
                thresh.vcc.alarm.low, thresh.vcc.warn.low,
                thresh.vcc.warn.high, thresh.vcc.alarm.high))
        print("    {:<14} {:>10.4} {:>15.4} {:>15.4} {:>10.4}".format(
                'Tx Bias:',
                thresh.txBias.alarm.low, thresh.txBias.warn.low,
                thresh.txBias.warn.high, thresh.txBias.alarm.high))
        if thresh.txPwr:
            print("    {:<14} {:>10.4} {:>15.4} {:>15.4} {:>10.4}".format(
                    'Tx Power(dBm):',
                    utils.mw_to_dbm(thresh.txPwr.alarm.low),
                    utils.mw_to_dbm(thresh.txPwr.warn.low),
                    utils.mw_to_dbm(thresh.txPwr.warn.high),
                    utils.mw_to_dbm(thresh.txPwr.alarm.high)))
        print("    {:<14} {:>10.4} {:>15.4} {:>15.4} {:>10.4}".format(
                'Rx Power(dBm):',
                utils.mw_to_dbm(thresh.rxPwr.alarm.low),
                utils.mw_to_dbm(thresh.rxPwr.warn.low),
                utils.mw_to_dbm(thresh.rxPwr.warn.high),
                utils.mw_to_dbm(thresh.rxPwr.alarm.high)))

    def _print_sensor_flags(self, sensor):
        ''' print details about sensor flags '''

        # header
        print("  {:<12}   {:>10} {:>15} {:>15} {:>10}".format(
            'Flags:', 'Alarm Low', 'Warning Low', 'Warning High', 'Alarm High'))

        sensor_tmpl = "    {:<12} {:>10} {:>15} {:>15} {:>10}"
        # temp
        print(sensor_tmpl.format('Temp:',
              sensor.temp.flags.alarm.low,
              sensor.temp.flags.warn.low,
              sensor.temp.flags.warn.high,
              sensor.temp.flags.alarm.high))

        # vcc
        print(sensor_tmpl.format('Vcc:',
              sensor.vcc.flags.alarm.low,
              sensor.vcc.flags.warn.low,
              sensor.vcc.flags.warn.high,
              sensor.vcc.flags.alarm.high))

    def _print_port_channel(self, channel):
        # per-channel output:
        print("  {:<15} {:0.4}  ".format("Tx Bias(mA)",
              channel.sensors.txBias.value), end="")
        if channel.sensors.txPwr:
            print("  {:<15} {:0.4}  ".format("Tx Power(dBm)",
                  utils.mw_to_dbm(channel.sensors.txPwr.value)), end="")
        print("  {:<15} {:0.4}  ".format("Rx Power(dBm)",
              utils.mw_to_dbm(channel.sensors.rxPwr.value)))

        if not self._verbose:
            return

        print("  {:<14}   {:>10} {:>15} {:>15} {:>10}".format(
                'Flags:',
                'Alarm Low', 'Warning Low', 'Warning High', 'Alarm High'))

        print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
                'Tx Bias(mA):',
                channel.sensors.txBias.flags.alarm.low,
                channel.sensors.txBias.flags.warn.low,
                channel.sensors.txBias.flags.warn.high,
                channel.sensors.txBias.flags.alarm.high))

        if channel.sensors.txPwr:
            print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
                    'Tx Power(dBm):',
                    utils.mw_to_dbm(channel.sensors.txPwr.flags.alarm.low),
                    utils.mw_to_dbm(channel.sensors.txPwr.flags.warn.low),
                    utils.mw_to_dbm(channel.sensors.txPwr.flags.warn.high),
                    utils.mw_to_dbm(channel.sensors.txPwr.flags.alarm.high)))

        print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
                'Rx Power(dBm):',
                utils.mw_to_dbm(channel.sensors.rxPwr.flags.alarm.low),
                utils.mw_to_dbm(channel.sensors.rxPwr.flags.warn.low),
                utils.mw_to_dbm(channel.sensors.rxPwr.flags.warn.high),
                utils.mw_to_dbm(channel.sensors.rxPwr.flags.alarm.high)))

    def _print_transceiver_details(self, tid):
        ''' Print details about transceiver '''

        info = self._info_resp[tid]
        ch_to_port = self._t_to_p[tid]
        if info.present is False:
            self._print_transceiver_ports(ch_to_port, info)
            return

        print("Transceiver:  {:>2}".format(info.port))
        if info.vendor:
            self._print_vendor_details(info)

        if info.cable:
            self._print_cable_details(info)

        if info.sensor or (info.thresholds and self._verbose) or info.channels:
            print("Monitoring Information:")

        if info.sensor:
            print("  {:<15} {:0.4}   {:<4} {:0.4}".format("Temperature",
                  info.sensor.temp.value, "Vcc", info.sensor.vcc.value))

        if self._verbose and info.thresholds:
            self._print_thresholds(info.thresholds)

        if self._verbose and info.sensor:
            if info.sensor.temp.flags and info.sensor.vcc.flags:
                self._print_sensor_flags(info.sensor)

        for channel in info.channels:
            port = ch_to_port.get(channel.channel, None)
            if port:
                attrs = PortStatusCmd._get_status_strs(self._status_resp[port])
                print("  Channel: {}  Port: {:>2}  Status: {:<8}  Link: {:<4}"
                        .format(channel.channel, port, attrs['admin_status'],
                                attrs['link_status']))
            else:
                # This is a channel for a port we weren't asked to display.
                #
                # (It would probably be nicer to clean up the CLI syntax so it
                # is a bit less ambiguous about what we should do here when we
                # were only asked to display info for some ports in a given
                # transceiver.)
                print("  Channel: {}".format(channel.channel))

            self._print_port_channel(channel)

        # If we didn't print any channel info, print something useful
        if not info.channels:
            self._print_transceiver_ports(ch_to_port, info)

    def _print_port_detail(self):
        ''' print port details '''

        # If a port does not have a mapping to a transceiver, we should
        # still print it, lest we skip ports in the detail display.
        transceiver_printed = []
        for port, status in sorted(self._status_resp.items()):
            if status.transceiverIdx:
                tid = status.transceiverIdx.transceiverId
                if tid not in transceiver_printed:
                    self._print_transceiver_details(tid)
                transceiver_printed.append(tid)
            else:
                attrs = PortStatusCmd._get_status_strs(self._status_resp[port])
                print("Port: {:>2}  Status: {:<8}  Link: {:<4}  Transceiver: {}"
                      .format(port, attrs['admin_status'], attrs['link_status'],
                                attrs['present']))

    def get_detail_status(self):
        ''' Get port detail port status '''

        for port, status in sorted(self._status_resp.items()):
            if status.transceiverIdx:
                self._get_channel_detail(port, status)

        # TODO: Until the updated getPortStatus is deployed for all platforms,
        # we need cleanly handle getting no mappings at all:
        if not self._transceiver:
            self._list_ports_detail_sfpdom()
            return

        # TODO: Until getTransceiverInfo is deployed for all platforms,
        # automatically revert to using getSfpDom on failure:
        try:
            self._info_resp = self._client.getTransceiverInfo(self._transceiver)
        except TApplicationException:
            self._list_ports_detail_sfpdom()
            return

        self._get_dummy_status()
        self._print_port_detail()


class ProductInfoCmd(FbossCmd):
    def _print_product_info(self, productInfo):
        print("Product: %s" % (productInfo.product))
        print("OEM: %s" % (productInfo.oem))
        print("Serial: %s" % (productInfo.serial))

    def _print_product_details(self, productInfo):
        print("Product: %s" % (productInfo.product))
        print("OEM: %s" % (productInfo.oem))
        print("Serial: %s" % (productInfo.serial))
        print("Management MAC Address: %s" % (productInfo.mgmtMac))
        print("BMC MAC Address: %s" % (productInfo.bmcMac))
        print("Extended MAC Start: %s" % (productInfo.macRangeStart))
        print("Extended MAC Size: %s" % (productInfo.macRangeSize))
        print("Assembled At: %s" % (productInfo.assembledAt))
        print("Product Asset Tag: %s" % (productInfo.assetTag))
        print("Product Part Number: %s" % (productInfo.partNumber))
        print("Product Production State: %s" % (productInfo.productionState))
        print("Product Sub-Version: %s" % (productInfo.subVersion))
        print("Product Version: %s" % (productInfo.productVersion))
        print("System Assembly Part Number: %s" %
                                            (productInfo.systemPartNumber))
        print("System Manufacturing Date: %s" % (productInfo.mfgDate))
        print("PCB Manufacturer: %s" % (productInfo.pcbManufacturer))
        print("Facebook PCBA Part Number: %s" % (productInfo.fbPcbaPartNumber))
        print("Facebook PCB Part Number: %s" % (productInfo.fbPcbPartNumber))
        print("ODM PCBA Part Number: %s" % (productInfo.odmPcbaPartNumber))
        print("ODM PCBA Serial Number: %s" % (productInfo.odmPcbaSerial))
        print("Version: %s" % (productInfo.version))

    def run(self, detail):
        self._client = self._create_ctrl_client()
        resp = self._client.getProductInfo()
        if detail:
            self._print_product_details(resp)
        else:
            self._print_product_info(resp)


class VersionCmd(FbossCmd):
    def run(self, config_type):
        if config_type == 'ctrl':
            self._client = self._create_ctrl_client()
        resp = self._client.getExportedValues()
        _print_build_info(resp)


def _print_build_info(resp):
    if not resp:
        print("No Build Info Found")
        return
    print("Package Name: %s" % resp['build_package_name'])
    print("Package Info: %s" % resp['build_package_info'])
    print("Package Version: %s" % resp['build_package_version'])
    print("Build Details:\n")
    print("\tHost: %s" % resp['build_host'])
    print("\tTime: %s" % resp['build_time'])
    print("\tUser: %s" % resp['build_user'])
    print("\tPath: %s" % resp['build_path'])
    print("\tPlatform: %s" % resp['build_platform'])
    print("\tRevision: %s" % resp['build_revision'])


class ReloadConfigCmd(FbossCmd):
    """
    Command to instruct agent to reload its own config, as if it is restarting,
    but without restarting.
    """
    def run(self):
        try:
            self._client = self._create_ctrl_client()
            self._client.reloadConfig()
            print("Config reloaded")
            return
        except FbossBaseError as e:
            print('Fboss Error: ' + e)
            return 2


def _l2_table_print(resp):
    if not resp:
        print("No L2 Entries Found")
        return
    resp = sorted(resp, key=lambda x: (x.port, x.vlanID, x.mac))
    tmpl = "{:18} {:>4}  {}"
    print(tmpl.format("MAC Address", "Port", "VLAN"))
    for entry in resp:
        print(tmpl.format(entry.mac, entry.port, entry.vlanID))


class L2TableCmd(FbossCmd):
    def run(self):
        self._client = self._create_ctrl_client()
        resp = self._client.getL2Table()
        _l2_table_print(resp)


def _print_config_info(resp):
    if not resp:
        print("No Config Info Found")
        return
    parsed = json.loads(resp)
    print(json.dumps(parsed, indent=4, sort_keys=True,
                     separators=(',', ': ')))


class GetConfigCmd(FbossCmd):
    def run(self, config_type):
        if config_type == 'ctrl':
            self._client = self._create_ctrl_client()

        resp = self._client.getRunningConfig()
        _print_config_info(resp)
