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
from neteng.fboss.optic import ttypes as optic_ttypes
from neteng.fboss.ttypes import FbossBaseError
from thrift.Thrift import TApplicationException

class PortDetailsCmd(cmds.FbossCmd):
    def run(self, ports):
        try:
            self._client = self._create_ctrl_client()
            # No ports specified, get all ports
            if not ports:
                resp = self._client.getAllPortInfo()

        except FbossBaseError as e:
            raise SystemExit('Fboss Error: ' + e)

        except Exception as e:
            raise Exception('Error: ' + e)

        if ports:
            for port in ports:
                self._print_port_details(port)
        elif resp:
            all_ports = sorted(resp.values(), key=utils.port_sort_fn)
            all_ports = [port for port in all_ports if port.operState == 1]
            for port in all_ports:
                self._print_port_details(port.portId, port)
        else:
            print("No Ports Found")

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
        vlans = ' '.join(str(vlan) for vlan in (port_info.vlans or []))

        fmt = '{:.<50}{}'
        lines = [
            ('Name', port_info.name.strip()),
            ('Port ID', str(port_info.portId)),
            ('Admin State', admin_status),
            ('Link State', oper_status),
            ('Speed', '{:.0f} {}'.format(speed, suffix)),
            ('VLANs', vlans)
        ]

        print()
        print('\n'.join(fmt.format(*l) for l in lines))
        print('Description'.ljust(20, '.') + (port_info.description or ""))


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
        self._client = self._create_ctrl_client()
        if detail or verbose:
            PortStatusDetailCmd(
                    self._client, ports, verbose).get_detail_status()
        else:
            self.list_ports(ports)

    def list_ports(self, ports):
        try:
            field_fmt = '{:>10}  {:>12}  {}{:>10}  {:>12}  {:>6}'
            print(field_fmt.format('Port', 'Admin State', '', 'Link State',
                                   'Transceiver', 'Speed'))
            print('-' * 59)
            resp = self._client.getPortStatus(ports)
            port_info = self._client.getAllPortInfo()

            for port_data in sorted(port_info.values(), key=utils.port_sort_fn):
                port = port_data.portId
                if port not in resp:
                    continue
                status = resp[port]
                attrs = utils.get_status_strs(status)
                if status.enabled:
                    name = port_data.name if port_data.name else port
                    print(field_fmt.format(
                        name, attrs['admin_status'], attrs['color_align'],
                        attrs['link_status'], attrs['present'], attrs['speed']))

        except KeyError as e:
            print("Invalid port", e)

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

        attrs = utils.get_status_strs(status)
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
            attrs = utils.get_status_strs(self._status_resp[port])
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

    def _print_settings_details(self, info):
        ''' print setting details'''
        print("CDR Tx: {}\tCDR Rx: {}".format(
            optic_ttypes.FeatureState._VALUES_TO_NAMES[info.settings.cdrTx],
            optic_ttypes.FeatureState._VALUES_TO_NAMES[info.settings.cdrRx]))
        print("Rate select: {}".format(
              optic_ttypes.RateSelectState._VALUES_TO_NAMES[
                  info.settings.rateSelect]))
        print("\tOptimised for: {}".format(
              optic_ttypes.RateSelectSetting._VALUES_TO_NAMES[
                  info.settings.rateSelectSetting]))
        print("Power measurement: {}".format(
            optic_ttypes.FeatureState._VALUES_TO_NAMES[
                info.settings.powerMeasurement]))
        print("Power control: {}".format(
            optic_ttypes.PowerControlState._VALUES_TO_NAMES[
                info.settings.powerControl]))

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
                attrs = utils.get_status_strs(self._status_resp[port])
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
                attrs = utils.get_status_strs(self._status_resp[port])
                print("Port: {:>2}  Status: {:<8}  Link: {:<4}  Transceiver: {}"
                      .format(port, attrs['admin_status'], attrs['link_status'],
                                attrs['present']))

    def get_detail_status(self):
        ''' Get port detail port status '''

        for port, status in sorted(self._status_resp.items()):
            if status.transceiverIdx:
                self._get_channel_detail(port, status)

        if not self._transceiver:
            self._list_ports_detail_sfpdom()
            return

        try:
            self._info_resp = self._client.getTransceiverInfo(self._transceiver)
        except TApplicationException:
            self._list_ports_detail_sfpdom()
            return

        self._get_dummy_status()
        self._print_port_detail()
