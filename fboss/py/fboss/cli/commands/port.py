#!/usr/bin/env python3
#
#  Copyright (c) 2004-present, Facebook, Inc.
#  All rights reserved.
#
#  This source code is licensed under the BSD-style license found in the
#  LICENSE file in the root directory of this source tree. An additional grant
#  of patent rights can be found in the PATENTS file in the same directory.
#

import collections
import time

from fboss.cli.utils import utils
from fboss.cli.commands import commands as cmds
from math import log10
from neteng.fboss.transceiver import ttypes as transceiver_ttypes
from neteng.fboss.ttypes import FbossBaseError
from thrift.Thrift import TApplicationException
from thrift.transport.TTransport import TTransportException


class PortDetailsCmd(cmds.FbossCmd):
    def run(self, ports, show_down=True):

        with self._create_agent_client() as client:
            try:
                resp = client.getAllPortInfo()

            except FbossBaseError as e:
                raise SystemExit('Fboss Error: ' + str(e))
            except Exception as e:
                raise Exception('Error: ' + str(e))

            def use_port(port):
                return (show_down or port.operState == 1) and \
                       (not ports or port.portId in ports or port.name in ports)

            port_details = {k: v for k, v in resp.items() if use_port(v)}

            if not port_details:
                print("No ports found")

            for port in sorted(port_details.values(), key=utils.port_sort_fn):
                self._print_port_details(port)
                self._print_port_counters(client, port)

    def _convert_bps(self, bps):
        ''' convert bps to human readable form

            :var bps int: port speed in bps
            :return bps_per_unit float: bps divided by factor of the unit found
            :return suffix string: human readable format
        '''

        # TODO: Make this more accurate.
        bps_per_unit = suffix = None
        value = bps
        # expand to 'T' and beyond by adding in the proper unit
        for factor, unit in [(0, ''), (3, 'K'), (6, 'M'), (9, 'G')]:
            if value < 1000:
                bps_per_unit = bps / 10 ** factor
                suffix = '{}bps'.format(unit)
                break
            value /= 1000

        assert bps_per_unit is not None and suffix, (
            'Unable to convert bps to human readable format')

        return bps_per_unit, suffix

    def _print_port_details(self, port_info):
        ''' Print out port details

            :var port_info PortInfoThrift: port information
        '''

        admin_status = "ENABLED" if port_info.adminState else "DISABLED"
        oper_status = "UP" if port_info.operState else "DOWN"

        speed, suffix = self._convert_bps(port_info.speedMbps * (10 ** 6))
        vlans = ' '.join(str(vlan) for vlan in (port_info.vlans or []))

        if not hasattr(port_info, 'fecEnabled'):
            fec_status = "N/A"  # many ports don't implement FEC
        elif port_info.fecEnabled:
            fec_status = "ENABLED"
        else:
            fec_status = "DISABLED"

        pause = ''
        if port_info.txPause:
            pause = 'TX '
        if port_info.rxPause:
            pause = pause + 'RX'
        if pause == '':
            pause = 'None'
        fmt = '{:.<50}{}'
        lines = [
            ('Name', port_info.name.strip()),
            ('ID', str(port_info.portId)),
            ('Description', port_info.description or ""),
            ('Admin State', admin_status),
            ('Link State', oper_status),
            ('Speed', '{:.0f} {}'.format(speed, suffix)),
            ('VLANs', vlans),
            ('Forward Error Correction', fec_status),
            ('Pause', pause),
        ]

        print()
        print('\n'.join(fmt.format(*l) for l in lines))

        if hasattr(port_info, 'portQueues'):
            print(fmt.format("Unicast queues", len(port_info.portQueues)))
            for queue in port_info.portQueues:
                name = "({})".format(queue.name) if queue.name != "" else ""
                attrs = [queue.mode]
                for val in ("weight", "reservedBytes", "scalingFactor"):
                    if hasattr(queue, val) and getattr(queue, val):
                        attrs.append("{}={}".format(val, getattr(queue, val)))
                print("    Queue {} {:29}{}".format(
                    queue.id, name, ",".join(attrs)))
                if hasattr(queue, "aqm") and getattr(queue, "aqm"):
                    aqm = getattr(queue, "aqm")
                    attrs1 = []
                    attrs1.append("{}={}".format("aqm", "enabled"))
                    if hasattr(aqm.detection, "linear") and getattr(
                            aqm.detection, "linear"):
                        attrs1.append("{}={}".format("detection", "linear"))
                        linear = getattr(aqm.detection, "linear")
                        attrs1.append("{}={}".format(
                            "minThresh",
                            linear.minimumLength))
                        attrs1.append("{}={}".format(
                            "maxThresh",
                            linear.maximumLength))
                    if aqm.behavior.earlyDrop:
                        earlyDrop = "ENABLED"
                    else:
                        earlyDrop = "DISABLED"
                    attrs1.append("{}={}".format("earlyDrop", earlyDrop))
                    if aqm.behavior.ecn:
                        ecn = "ENABLED"
                    else:
                        ecn = "DISABLED"
                    attrs1.append("{}={}".format("ecn", ecn))
                    print('{:<5}{}'.format('', ",".join(attrs1)))

    def _print_port_counters(self, client, port_info):
        pass


class PortFlapCmd(cmds.FbossCmd):
    FLAP_TIME = 1

    def run(self, ports, all):
        try:
            if all:
                try:
                    self._qsfp_client = self._create_qsfp_client()
                except TTransportException:
                    self._qsfp_client = None
                self.flap_all_ports()
            elif not ports:
                print("Hmm, how did we get here?")
            else:
                self.flap_ports(ports)
        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + e)

        if hasattr(self, '_qsfp_client') and self._qsfp_client:
            self._qsfp_client.__exit__(exec, None, None)

    def flap_ports(self, ports, flap_time=FLAP_TIME):
        with self._create_agent_client() as client:
            resp = client.getPortStatus(ports)
            for port, status in resp.items():
                if not status.enabled:
                    print("Port %d is disabled by configuration, cannot flap" %
                          (port))
                    continue
                print("Disabling port %d" % (port))
                client.setPortState(port, False)
            time.sleep(flap_time)
            for port, status in resp.items():
                if status.enabled:
                    print("Enabling port %d" % (port))
                    client.setPortState(port, True)

    def flap_all_ports(self, flap_time=FLAP_TIME):
        with self._create_agent_client() as client:
            qsfp_info_map = utils.get_qsfp_info_map(
                self._qsfp_client, None, continue_on_error=True)
            resp = client.getPortStatus()
            flapped_ports = []
            for port, status in resp.items():
                if status.enabled and not status.up:
                    qsfp_present = False
                    if status.transceiverIdx and self._qsfp_client:
                        qsfp_info = qsfp_info_map.get(
                            status.transceiverIdx.transceiverId)
                        qsfp_present = qsfp_info.present if qsfp_info else False
                    if qsfp_present:
                        print("Disabling port %d" % (port))
                        client.setPortState(port, False)
                        flapped_ports.append(port)
            time.sleep(flap_time)
            for port in flapped_ports:
                print("Enabling port %d" % (port))
                client.setPortState(port, True)


class PortSetStatusCmd(cmds.FbossCmd):
    def run(self, ports, status):
        try:
            self.set_status(ports, status)
        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + e)

    def set_status(self, ports, status):
        with self._create_agent_client() as client:
            for port in ports:
                status_str = 'Enabling' if status else 'Disabling'
                print("{} port {}".format(status_str, port))
                client.setPortState(port, status)


class PortStatsCmd(cmds.FbossCmd):
    def run(self, details, ports):
        try:
            self.show_stats(details, ports)
        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + e)

    def show_stats(self, details, ports):
        with self._create_agent_client() as client:
            if not ports:
                stats = client.getAllPortStats()
            else:
                stats = {}
                for port in ports:
                    stats[port] = client.getPortStats(port)
            neighbors = client.getLldpNeighbors()
        n2ports = {}
        # collect up the neighbors by port
        for neighbor in neighbors:
            n2ports.setdefault(neighbor.localPort, []).append(neighbor)
        # Port Name
        field_fmt = '{:<11} {:>3} {:>} {:<} {:>} {:<} {:<}'
        hosts = "Hosts" if details else ""

        print(field_fmt.format("Port Name", "+Id", "In",
                               self._get_counter_string(None),
                               "Out", self._get_counter_string(None), hosts))
        for port_id, port in stats.items():
            print(field_fmt.format(port.name, port_id, "In",
                                   self._get_counter_string(port.input),
                                   "Out", self._get_counter_string(port.output),
                                   self._get_lldp_string(port_id, n2ports, details)))

    def _get_counter_string(self, counters):
        # bytes uPkts mPkts bPkts errPkts discPkts
        field_fmt = '{:>15} {:>15} {:>10} {:>10} {:>10} {:>10}'
        if counters is None:
            return field_fmt.format("bytes", "uPkts", "mcPkts", "bcPkts",
                                    "errsPkts", "discPkts")
        else:
            return field_fmt.format(counters.bytes, counters.ucastPkts,
                                    counters.multicastPkts,
                                    counters.broadcastPkts,
                                    counters.errors.errors,
                                    counters.errors.discards)

    def _get_lldp_string(self, port_id, n2ports, details):
        ret = ""
        if details and port_id in n2ports:
            for n in n2ports[port_id]:
                ret += " {}".format(n.systemName)
        return ret


class PortStatsClearCmd(cmds.FbossCmd):
    def run(self, ports):
        client = self._create_agent_client()
        client.clearPortStats(ports if ports else
                client.getAllPortInfo().keys())


class PortStatusCmd(cmds.FbossCmd):
    def run(self, detail, ports, verbose, internal, all):
        with self._create_agent_client() as client:
            try:
                self._qsfp_client = self._create_qsfp_client()
            except TTransportException:
                self._qsfp_client = None
            if detail or verbose:
                PortStatusDetailCmd(
                    client, ports, self._qsfp_client, verbose
                ).get_detail_status()
            elif internal:
                self.list_ports(client, ports, internal_port=True)
            elif all:
                self.list_ports(client, ports, all=True)
            else:
                self.list_ports(client, ports)

        if hasattr(self, '_qsfp_client') and self._qsfp_client:
            self._qsfp_client.__exit__(exec, None, None)

    def _get_field_format(self, internal_port):
        if internal_port:
            field_fmt = '{:<6} {:>11} {:>12}  {}{:>10}  {:>12}  {:>6}'
            print(field_fmt.format('ID', 'Name', 'AdminState', '',
                                   'LinkState', 'Transceiver', 'Speed'))
            print('-' * 67)
        else:
            field_fmt = '{:<11} {:>12}  {}{:>10}  {:>12}  {:>6}'
            print(field_fmt.format('Name', 'Admin State', '', 'Link State',
                                   'Transceiver', 'Speed'))
            print('-' * 59)
        return field_fmt

    def list_ports(self, client, ports, internal_port=False, all=False):
        field_fmt = self._get_field_format(internal_port)
        port_status_map = client.getPortStatus(ports)
        qsfp_info_map = utils.get_qsfp_info_map(
            self._qsfp_client, None, continue_on_error=True)
        port_info_map = client.getAllPortInfo()
        missing_port_status = []
        for port_info in sorted(port_info_map.values(), key=utils.port_sort_fn):
            port_id = port_info.portId
            if ports and (port_id not in ports):
                continue
            status = port_status_map.get(port_id)
            if not status:
                missing_port_status.append(port_id)
                continue

            qsfp_present = False
            # For non QSFP ports (think Fabric port) qsfp_client
            # will not return any information.
            if status.transceiverIdx and self._qsfp_client:
                qsfp_info = qsfp_info_map.get(
                    status.transceiverIdx.transceiverId)
                qsfp_present = qsfp_info.present if qsfp_info else False

            attrs = utils.get_status_strs(status, qsfp_present)
            if internal_port:
                speed = attrs['speed']
                if not speed:
                    speed = '-'
                print(field_fmt.format(
                    port_id,
                    port_info.name,
                    attrs['admin_status'],
                    attrs['color_align'],
                    attrs['link_status'],
                    attrs['present'],
                    speed))
            elif all:
                name = port_info.name if port_info.name else port_id
                print(field_fmt.format(
                    name,
                    attrs['admin_status'],
                    attrs['color_align'],
                    attrs['link_status'],
                    attrs['present'],
                    attrs['speed']))
            elif status.enabled:
                name = port_info.name if port_info.name else port_id
                print(field_fmt.format(
                    name,
                    attrs['admin_status'],
                    attrs['color_align'],
                    attrs['link_status'],
                    attrs['present'],
                    attrs['speed']))
        if missing_port_status:
            print(utils.make_error_string(
                "Could not get status of ports {}".format(missing_port_status)))


class PortStatusDetailCmd(object):
    ''' Print detailed/verbose port status '''

    def __init__(self, client, ports, qsfp_client, verbose):
        self._client = client
        self._qsfp_client = qsfp_client
        self._ports = ports
        self._port_speeds = self._get_port_speeds()
        self._info_resp = None
        self._status_resp = self._client.getPortStatus(ports)
        # map of { transceiver_id -> { channel_id -> port } }
        self._t_to_p = collections.defaultdict(dict)
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

        if speed in (20000, 40000):
            return range(start_channel, (start_channel + int(speed / 10000)))

        # Else, return one channel for all other cases.
        return [start_channel]

    def _get_channel_detail(self, port, status):
        ''' Get channel detail for port '''

        channels = status.transceiverIdx.channels
        if channels is None:
            # TODO(aeckert): rm after next agent push
            channels = self._get_port_channels(
                port, status.transceiverIdx)

        tid = status.transceiverIdx.transceiverId
        for ch in channels:
            self._t_to_p[tid][ch] = port

        if tid not in self._transceiver:
            self._transceiver.append(tid)

    def _mw_to_dbm(self, mw):
        if mw == 0:
            return 0.0
        else:
            return (10 * log10(mw))

    def _get_dummy_status(self):
        ''' Get dummy status for ports without data '''

        for port, status in sorted(self._status_resp.items()):
            if status.transceiverIdx:
                tid = status.transceiverIdx.transceiverId
                if tid not in self._info_resp.keys():
                    info = transceiver_ttypes.TransceiverInfo()
                    info.port = port
                    info.present = False
                    self._info_resp[port] = info

    def _print_transceiver_ports(self, ports, info):
        ''' Print port info if the transceiver doesn't have any'''

        present = info.present if info else None

        for port in ports:
            attrs = utils.get_status_strs(self._status_resp[port], present)
            print("Port: {:>2}  Status: {:<8}  Link: {:<4}  Transceiver: {}".
                    format(port, attrs['admin_status'], attrs['link_status'],
                            attrs['present']))

    def _print_vendor_details(self, info):
        ''' print vendor details '''

        print("Vendor:  {:<16}  Part Number:  {:<16}".format(
              info.vendor.name, info.vendor.partNumber))
        print("Serial:  {:<16}  ".format(info.vendor.serialNumber), end="")
        print("Date Code:  {:<8}  Revision: {:<2}".format(
              info.vendor.dateCode, info.vendor.rev))

    def _print_settings_details(self, info):
        ''' print setting details'''

        print("CDR Tx: {}\tCDR Rx: {}".format(
            transceiver_ttypes.FeatureState._VALUES_TO_NAMES[
                info.settings.cdrTx],
            transceiver_ttypes.FeatureState._VALUES_TO_NAMES[
                info.settings.cdrRx]))
        print("Rate select: {}".format(
              transceiver_ttypes.RateSelectState._VALUES_TO_NAMES[
                  info.settings.rateSelect]))
        print("\tOptimised for: {}".format(
              transceiver_ttypes.RateSelectSetting._VALUES_TO_NAMES[
                  info.settings.rateSelectSetting]))
        print("Power measurement: {}".format(
            transceiver_ttypes.FeatureState._VALUES_TO_NAMES[
                info.settings.powerMeasurement]))
        print("Power control: {}".format(
            transceiver_ttypes.PowerControlState._VALUES_TO_NAMES[
                info.settings.powerControl]))

    def _get_cable_details(self, info):
        ''' returns cable details '''

        # TODO(T20770659): Add support for AOC.
        # TODO(T20773909): Validate cable type based on part number.
        # TODO(T20773919): Validate cable length based on part number.

        Cable = collections.namedtuple('Cable', 'length type unit')
        cable_info_obtained = (
            Cable(length=info.cable.copper, type="Copper", unit='m'),
            Cable(length=info.cable.om1, type="OM1", unit='m'),
            Cable(length=info.cable.om2, type="OM2", unit='m'),
            Cable(length=info.cable.om3, type="OM3", unit='m'),
            Cable(length=info.cable.singleMode, type="singleMode", unit='m'),
            Cable(length=info.cable.singleModeKm, type="singleModeKm", unit='km'
                  ))

        cables_found = []
        for cable in cable_info_obtained:
            if cable.length:
                cables_found.append(cable)

        return cables_found

    def _print_cable_details(self, info):
        '''prints cable details'''

        cables_found = self._get_cable_details(info)

        if len(cables_found) == 1:
            print_string = "Cable:"
            print_string += cables_found[0].type
            print_string += " "
            print_string += str(cables_found[0].length)
            print_string += cables_found[0].unit

            print(print_string)

        # Bad DOM or unsupported conditions.
        elif len(cables_found) == 0:
            print("Cable:Unknown/unsupported/no cable type found.")
            print("\n       WARNING:Please verify DOM.")
        else:  # len(cables_found) > 1
            print("Cable:More than one cables found. Details: ")
            print("      ", end="")
            for cable_iterator in cables_found:
                print(cable_iterator.type + " " +
                      str(cable_iterator.length) +
                      cable_iterator.unit + "  ", end="")
            print("\n      WARNING: Please verify DOM.")

    def _print_thresholds(self, thresh):
        ''' print threshold details '''

        print("  {:<16}   {:>10} {:>15} {:>15} {:>10}".format(
            'Thresholds:', 'Alarm Low', 'Warning Low', 'Warning High',
            'Alarm High'))
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
                self._mw_to_dbm(thresh.txPwr.alarm.low),
                self._mw_to_dbm(thresh.txPwr.warn.low),
                self._mw_to_dbm(thresh.txPwr.warn.high),
                self._mw_to_dbm(thresh.txPwr.alarm.high)))
            print("    {:<14} {:>10.4} {:>15.4} {:>15.4} {:>10.4}".format(
                'Tx Power(mW):',
                thresh.txPwr.alarm.low,
                thresh.txPwr.warn.low,
                thresh.txPwr.warn.high,
                thresh.txPwr.alarm.high))
        print("    {:<14} {:>10.4} {:>15.4} {:>15.4} {:>10.4}".format(
            'Rx Power(dBm):',
            self._mw_to_dbm(thresh.rxPwr.alarm.low),
            self._mw_to_dbm(thresh.rxPwr.warn.low),
            self._mw_to_dbm(thresh.rxPwr.warn.high),
            self._mw_to_dbm(thresh.rxPwr.alarm.high)))
        print("    {:<14} {:>10.4} {:>15.4} {:>15.4} {:>10.4}".format(
            'Rx Power(mW):',
            thresh.rxPwr.alarm.low,
            thresh.rxPwr.warn.low,
            thresh.rxPwr.warn.high,
            thresh.rxPwr.alarm.high))

    def _print_sensor_flags(self, sensor):
        ''' print details about sensor flags '''

        # header
        print("  {:<12}   {:>10} {:>15} {:>15} {:>10}".format(
            'Flags:', 'Alarm Low', 'Warning Low', 'Warning High', 'Alarm High'))

        sensor_tmpl = "    {:<12} {:>10} {:>15} {:>15} {:>10}"

        # temperature
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
                  self._mw_to_dbm(channel.sensors.txPwr.value)), end="")
            print("  {:<15} {:0.4}  ".format("Tx Power(mW)",
                  channel.sensors.txPwr.value), end="")
        print("  {:<15} {:0.4}  ".format("Rx Power(dBm)",
              self._mw_to_dbm(channel.sensors.rxPwr.value)))
        print("  {:<15} {:0.4}  ".format("Rx Power(mW)",
              channel.sensors.rxPwr.value))

        if not self._verbose:
            return

        print("  {:<14}   {:>10} {:>15} {:>15} {:>10}".format(
            'Flags:', 'Alarm Low', 'Warning Low', 'Warning High', 'Alarm High'))

        print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
            'Tx Bias(mA):',
            channel.sensors.txBias.flags.alarm.low,
            channel.sensors.txBias.flags.warn.low,
            channel.sensors.txBias.flags.warn.high,
            channel.sensors.txBias.flags.alarm.high))

        if channel.sensors.txPwr:
            print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
                'Tx Power(dBm):',
                self._mw_to_dbm(channel.sensors.txPwr.flags.alarm.low),
                self._mw_to_dbm(channel.sensors.txPwr.flags.warn.low),
                self._mw_to_dbm(channel.sensors.txPwr.flags.warn.high),
                self._mw_to_dbm(channel.sensors.txPwr.flags.alarm.high)))
            print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
                'Tx Power(mW):',
                channel.sensors.txPwr.flags.alarm.low,
                channel.sensors.txPwr.flags.warn.low,
                channel.sensors.txPwr.flags.warn.high,
                channel.sensors.txPwr.flags.alarm.high))
        print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
            'Rx Power(dBm):',
            self._mw_to_dbm(channel.sensors.rxPwr.flags.alarm.low),
            self._mw_to_dbm(channel.sensors.rxPwr.flags.warn.low),
            self._mw_to_dbm(channel.sensors.rxPwr.flags.warn.high),
            self._mw_to_dbm(channel.sensors.rxPwr.flags.alarm.high)))
        print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
            'Rx Power(mW):',
            channel.sensors.rxPwr.flags.alarm.low,
            channel.sensors.rxPwr.flags.warn.low,
            channel.sensors.rxPwr.flags.warn.high,
            channel.sensors.rxPwr.flags.alarm.high))

    def _print_transceiver_details(self, tid):  # noqa
        ''' Print details about transceiver '''

        info = self._info_resp[tid]
        ch_to_port = self._t_to_p[tid]
        if info.present is False:
            self._print_transceiver_ports(ch_to_port.values(), info)
            return

        print("Transceiver:  {:>2}".format(info.port))
        if info.vendor:
            self._print_vendor_details(info)

        if info.cable:
            self._print_cable_details(info)

        if info.settings:
            self._print_settings_details(info)

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
                attrs = utils.get_status_strs(self._status_resp[port], None)
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
            self._print_transceiver_ports(ch_to_port.values(), info)

    def _print_port_detail(self):
        ''' print port details '''

        if not self._qsfp_client:
            self._print_transceiver_ports(self._status_resp.keys(), None)
            return

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
                attrs = utils.get_status_strs(self._status_resp[port],
                                              is_present=False)
                print("Port: {:>2}  Status: {:<8}  Link: {:<4}  Transceiver: {}"
                      .format(port, attrs['admin_status'], attrs['link_status'],
                                attrs['present']))

    def get_detail_status(self):
        ''' Get port detail port status '''

        for port, status in sorted(self._status_resp.items()):
            if status.transceiverIdx:
                self._get_channel_detail(port, status)

        if not self._transceiver:
            return

        if not self._qsfp_client:
            self._info_resp = {}
        else:
            try:
                self._info_resp = \
                    self._qsfp_client.getTransceiverInfo(self._transceiver)
            except TApplicationException:
                return

        self._get_dummy_status()
        self._print_port_detail()


class PortDescriptionCmd(cmds.FbossCmd):
    def run(self, ports, show_down=True):
        try:
            with self._create_agent_client() as client:
                resp = client.getAllPortInfo()

        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + str(e))
        except Exception as e:
            raise Exception('Error: ' + str(e))

        def use_port(port):
            return (show_down or port.operState == 1) and \
                   (not ports or port.portId in ports or port.name in ports)

        port_description = {k: v for k, v in resp.items() if use_port(v)}

        if not port_description:
            print("No ports found")

        tmpl = "{:18} {}"
        print(tmpl.format("Port", "Description"))
        for port in sorted(port_description.values(), key=utils.port_sort_fn):
            print(tmpl.format(port.name.strip(), port.description))
