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

from fboss.cli import utils
from fboss.cli.commands import commands as cmds
from math import log10
from neteng.fboss.optic import ttypes as optic_ttypes
from neteng.fboss.ttypes import FbossBaseError
from thrift.Thrift import TApplicationException


class PortDetailsCmd(cmds.FbossCmd):
    def run(self, ports):
        try:
            self._client = self._create_ctrl_client()
            if ports:
                self._port_details(ports)
            else:
                self._all_port_details()
        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + e)
        except Exception as e:
            raise Exception('Error: ' + e)

    def _port_details(self, ports):
        ''' Print details only for ports given

            :var ports list: of ports to print details for '''

        for port in ports:
            print('\nDetails for {}'.format(port))
            self._print_port_details(port)

    def _all_port_details(self):
        ''' Print all details for every port on the switch '''

        resp = self._client.getAllPortInfo()
        if not resp:
            print("No Ports Found")
            return
        for port, entry in resp.items():
            if entry.operState == 1:
                self._print_port_details(port, entry)

    def _convert_bps(self, bps):
        ''' convert bps to human readable form

            :var bps int: port speed in bps
            :return bps_per_unit float: bps divided by factor of the unit found
            :return suffix string: human readable format
        '''

        bps_per_unit = suffix = None
        value = bps
        # expand to 'T' and beyond by adding in the proper unit
        for factor, unit in [(1, ''), (3, 'K'), (6, 'M'), (9, 'G')]:
            if value < 1000:
                bps_per_unit = bps / 10 ** factor
                suffix = '{}bps'.format(unit)
                break
            value /= 1000

        assert bps_per_unit is not None and suffix, (
                'Unable to convert bps to human readable format')

        return bps_per_unit, suffix

    def _get_port_counters(self, counters, port_id):
        ''' get 1min, 5min and 1hr port counters

            :var counters list: of counter names to get from switch
            :var port_id int: port id
            :return port_counters
        '''

        full_counter_list = []
        for name in counters:
            for suffix in ('60', '600', '3600'):
                full_counter_list.append('port{}.{}.{}'.format(
                                            port_id, name, suffix))

        return self._client.getSelectedCounters(full_counter_list)

    def _bps(self, interval, bytes_per_minute):
        value, suffix = self._convert_bps((bytes_per_minute * 8) / interval)
        return None, '{:.02f}{}'.format(value, suffix)

    def _error_ctr(self, interval, value):
        color = utils.COLOR_RED if value > 0 else utils.COLOR_GREEN
        return color, value

    def _fmt_args(self, values, name, transform):
        ''' format argments for printing

            :var values zip obj: { interval, value_at_interval }
            :var name string: Counter Name
            :var transform func_obj: function to be used as transform
            :return list: list of the argument values
        '''
        fmt_args = [name]
        for interval, value in values:
            color, value_str = transform(interval, value)
            color_start = color_end = ''
            if color:
                color_start = color
                color_end = utils.COLOR_RESET
            fmt_args.extend([color_start, value_str, color_end])

        return fmt_args

    def _print_port_counters(self, port_id):
        ''' Display port counters with errors

            :var port_id int: port id
        '''

        counters = collections.OrderedDict()
        counters['in_bytes.sum'] = {'name': 'Ingress', 'transform': self._bps}
        counters['out_bytes.sum'] = {'name': 'Egress', 'transform': self._bps}
        counters['in_errors.sum'] = {
                        'name': 'In Errors', 'transform': self._error_ctr}
        counters['in_discards.sum'] = {
                        'name': 'In Discards', 'transform': self._error_ctr}
        counters['out_errors.sum'] = {
                        'name': 'Out Errors', 'transform': self._error_ctr}
        counters['out_discards.sum'] = {
                        'name': 'Out Discards', 'transform': self._error_ctr}

        port_counters = self._get_port_counters(counters.keys(), port_id)

        # extra arguments are for the ASCII color codes
        fmt = '    {:<20} {}{:>12}{} {}{:>14}{} {}{:>12}{}'
        print(fmt.format('Time Interval:',
                         '', '1 minute', '',
                         '', '10 minutes', '',
                         '', '1 hour', ''))

        for counter, attrs in counters.items():
            # get all counter values (1min, 5min and 1hr) for 'counter'
            # using tuples to avoid muatation of the data
            intervals = (60, 600, 3600)
            counter_values = tuple(port_counters['port{}.{}.{}'.format(
                        port_id, counter, interval)] for interval in intervals)
            values = zip(intervals, counter_values)
            data = self._fmt_args(values, attrs['name'], attrs['transform'])
            print(fmt.format(*data))

    def _print_port_details(self, port_id, port_info=None):
        ''' Print out port details

            :var port_id int: port identifier
            :var port_info PortInfoThrift: port information
        '''

        if not port_info:
            port_info = self._client.getPortInfo(port_id)

        admin_status = "ENABLED" if port_info.adminState else "DISABLED"
        oper_status = "UP" if port_info.operState else "DOWN"

        speed, suffix = self._convert_bps(port_info.speedMbps * (10 ** 6))
        vlans = ' '.join(str(vlan) for vlan in port_info.vlans)

        fmt = '{:.<50}{}'
        lines = [
            ('Interface', port_info.name.strip()),
            ('Port ID', str(port_info.portId)),
            ('Admin State', admin_status),
            ('Link State', oper_status),
            ('Speed', '{:.0f} {}'.format(speed, suffix)),
            ('VLANs', vlans)
        ]

        print()
        print('\n'.join(fmt.format(*l) for l in lines))
        print('Description'.ljust(20, '.') + port_info.description)

        self._print_port_counters(port_id)


class PortFlapCmd(cmds.FbossCmd):
    def run(self, ports):
        try:
            if not ports:
                print("Hmm, how did we get here?")
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


class PortStatusCmd(cmds.FbossCmd):
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

    def _mw_to_dbm(self, mw):
        if mw == 0:
            return 0.0
        else:
            return (10 * log10(mw))

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
                self._mw_to_dbm(dom.value.txPwr)))
        print("      {:<15} {:0.4}".format("Rx Power(dBm)",
                self._mw_to_dbm(dom.value.rxPwr)))

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
                self._mw_to_dbm(dom.flags.txPwrAlarmLow),
                self._mw_to_dbm(dom.flags.txPwrWarnLow),
                self._mw_to_dbm(dom.flags.txPwrWarnHigh),
                self._mw_to_dbm(dom.flags.txPwrAlarmHigh)))
        print("      {:<14} {:>15} {:>15} {:>15} {:>15}".format(
                'Rx Power(dBm):',
                self._mw_to_dbm(dom.flags.rxPwrAlarmLow),
                self._mw_to_dbm(dom.flags.rxPwrWarnLow),
                self._mw_to_dbm(dom.flags.rxPwrWarnHigh),
                self._mw_to_dbm(dom.flags.rxPwrAlarmHigh)))

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
                self._mw_to_dbm(thresh.txPwrAlarmLow),
                self._mw_to_dbm(thresh.txPwrWarnLow),
                self._mw_to_dbm(thresh.txPwrWarnHigh),
                self._mw_to_dbm(thresh.txPwrAlarmHigh)))
        print("      {:<14} {:>15.4} {:>15.4} {:>15.4} {:>15.4}".format(
                'Rx Power(dBm):',
                self._mw_to_dbm(thresh.rxPwrAlarmLow),
                self._mw_to_dbm(thresh.rxPwrWarnLow),
                self._mw_to_dbm(thresh.rxPwrWarnHigh),
                self._mw_to_dbm(thresh.rxPwrAlarmHigh)))

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
                    self._mw_to_dbm(thresh.txPwr.alarm.low),
                    self._mw_to_dbm(thresh.txPwr.warn.low),
                    self._mw_to_dbm(thresh.txPwr.warn.high),
                    self._mw_to_dbm(thresh.txPwr.alarm.high)))
        print("    {:<14} {:>10.4} {:>15.4} {:>15.4} {:>10.4}".format(
                'Rx Power(dBm):',
                self._mw_to_dbm(thresh.rxPwr.alarm.low),
                self._mw_to_dbm(thresh.rxPwr.warn.low),
                self._mw_to_dbm(thresh.rxPwr.warn.high),
                self._mw_to_dbm(thresh.rxPwr.alarm.high)))

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
                  self._mw_to_dbm(channel.sensors.txPwr.value)), end="")
        print("  {:<15} {:0.4}  ".format("Rx Power(dBm)",
              self._mw_to_dbm(channel.sensors.rxPwr.value)))

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
                    self._mw_to_dbm(channel.sensors.txPwr.flags.alarm.low),
                    self._mw_to_dbm(channel.sensors.txPwr.flags.warn.low),
                    self._mw_to_dbm(channel.sensors.txPwr.flags.warn.high),
                    self._mw_to_dbm(channel.sensors.txPwr.flags.alarm.high)))

        print("    {:<14} {:>10} {:>15} {:>15} {:>10}".format(
                'Rx Power(dBm):',
                self._mw_to_dbm(channel.sensors.rxPwr.flags.alarm.low),
                self._mw_to_dbm(channel.sensors.rxPwr.flags.warn.low),
                self._mw_to_dbm(channel.sensors.rxPwr.flags.warn.high),
                self._mw_to_dbm(channel.sensors.rxPwr.flags.alarm.high)))

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
