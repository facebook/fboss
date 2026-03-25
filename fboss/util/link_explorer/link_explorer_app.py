#!/usr/bin/env python3
"""
Link Explorer - Core logic for querying FBOSS switches.

Queries switches via Thrift (ServiceRouter) and joins
LLDP topology, port stats, and transceiver diagnostics data.
"""

import asyncio
import logging
import re
import typing as t

from libfb.py.asyncio.thrift import get_direct_client
from neteng.fboss.ctrl.clients import FbossCtrl
from neteng.fboss.ctrl.types import (
    DEFAULT_CTRL_PORT,
    LinkNeighborThrift,
    PortInfoThrift,
)
from neteng.fboss.lib.asyncio import hostnames
from neteng.fboss.qsfp.clients import QsfpService
from neteng.fboss.transceiver.types import TransceiverInfo
from servicerouter.py3 import get_client


logger = logging.getLogger(__name__)

# Default QSFP service port
DEFAULT_QSFP_PORT = 5910

# Default thrift timeout in milliseconds
_DEFAULT_TIMEOUT = 30000


# ── Media Interface Mapping ──────────────────────────────────────────────────

MEDIA_INTERFACE_MAP = {
    0: "UNKNOWN",
    1: "CWDM4_100G",
    2: "CR4_100G",
    3: "FR1_100G",
    4: "FR4_200G",
    5: "FR4_400G",
    6: "LR4_400G",
    7: "DR4_400G",
    8: "CR8_400G",
    9: "FR4_2x400G",
    10: "CR4_800G",
    11: "DR4_2x400G",
}


# ── Thrift Client Helpers ────────────────────────────────────────────────────


async def _get_agent_client(host: str):
    ip_addr = await hostnames.host_to_ip(host)
    if not ip_addr:
        raise RuntimeError(f"Unable to resolve IP for {host}")
    options = {"single_host": [ip_addr, DEFAULT_CTRL_PORT]}
    overrides = {
        "thrift_security": "required",
        "processing_timeout": str(_DEFAULT_TIMEOUT),
    }
    return get_client(FbossCtrl, "fboss.agent", options=options, overrides=overrides)


async def _get_qsfp_client(host: str):
    ip_addr = await hostnames.host_to_ip(host)
    if not ip_addr:
        raise RuntimeError(f"Unable to resolve IP for {host}")
    options = {"single_host": [ip_addr, DEFAULT_QSFP_PORT]}
    overrides = {
        "thrift_security": "required",
        "processing_timeout": str(_DEFAULT_TIMEOUT),
    }
    return get_client(
        QsfpService, "fboss.qsfp_service", options=options, overrides=overrides
    )


# ── Data Fetching ────────────────────────────────────────────────────────────


async def _fetch_lldp(host: str) -> t.List[LinkNeighborThrift]:
    async with await _get_agent_client(host) as client:
        return await client.getLldpNeighbors()


async def _fetch_ports(host: str) -> t.Mapping[int, PortInfoThrift]:
    async with await _get_agent_client(host) as client:
        return await client.getAllPortInfo()


async def _fetch_hw_port_stats(host: str) -> t.Mapping[str, t.Any]:
    async with await _get_agent_client(host) as client:
        return await client.getHwPortStats()


async def _fetch_transceivers(host: str) -> t.Mapping[int, TransceiverInfo]:
    async with await _get_qsfp_client(host) as client:
        return await client.getTransceiverInfo([])


# ── Main Query ───────────────────────────────────────────────────────────────


async def _query_switch_async(hostname: str) -> dict:
    try:
        lldp_entries, port_map, hw_stats_map, tcvr_map = await asyncio.gather(
            _fetch_lldp(hostname),
            _fetch_ports(hostname),
            _fetch_hw_port_stats(hostname),
            _fetch_transceivers(hostname),
        )
    except Exception as e:
        logger.exception(f"Failed to query {hostname}")
        return {"error": f"Cannot reach {hostname}: {e}"}

    if not lldp_entries:
        return {"error": f"No LLDP data from {hostname}"}

    # Build port name -> PortInfoThrift lookup
    ports_by_name: t.Dict[str, PortInfoThrift] = {}
    port_id_to_name: t.Dict[int, str] = {}
    for port_id, port_info in port_map.items():
        name = port_info.name
        ports_by_name[name] = port_info
        port_id_to_name[port_id] = name

    # Build port name -> TransceiverInfo lookup
    tcvrs_by_port: t.Dict[str, TransceiverInfo] = {}
    for port_id, port_info in port_map.items():
        tcvr_id = port_info.transceiverIdx
        if tcvr_id is not None and hasattr(tcvr_id, "transceiverId"):
            tid = tcvr_id.transceiverId
            if tid in tcvr_map:
                tcvrs_by_port[port_info.name] = tcvr_map[tid]

    links = []
    for entry in lldp_entries:
        local_port_id = entry.localPort
        local_port = port_id_to_name.get(local_port_id, "")
        if not local_port:
            local_port_name = getattr(entry, "localPortName", None)
            if local_port_name:
                local_port = local_port_name

        system_name = getattr(entry, "systemName", "") or ""
        peer = system_name.replace(".tfbnw.net", "")
        peer_port = entry.printablePortId or ""
        lldp_port_desc = getattr(entry, "portDescription", "") or ""

        port_info = ports_by_name.get(local_port, None)
        status = "unknown"
        speed = "--"
        profile = ""
        hw = {}
        expected = ""
        desc = ""
        cable_length = "--"

        if port_info is not None:
            oper_state = port_info.operState
            status = "up" if oper_state == 1 else "down"
            speed = (
                str(port_info.speedMbps)
                if hasattr(port_info, "speedMbps") and port_info.speedMbps
                else "--"
            )
            profile = port_info.profileID or ""
            expected = getattr(port_info, "expectedLLDPeerName", "") or ""
            desc = port_info.description or ""
            hw = {}
            hw_stats = hw_stats_map.get(local_port)
            if hw_stats:
                hw = {
                    "ingressBytes": getattr(hw_stats, "inBytes_", 0) or 0,
                    "egressBytes": getattr(hw_stats, "outBytes_", 0) or 0,
                    "inUnicastPkts": getattr(hw_stats, "inUnicastPkts_", 0) or 0,
                    "inErrorPkts": getattr(hw_stats, "inErrors_", 0) or 0,
                    "inDiscardPkts": getattr(hw_stats, "inDiscards_", 0) or 0,
                    "outDiscardPkts": getattr(hw_stats, "outDiscards_", 0) or 0,
                    "outCongestionDiscardPkts": getattr(
                        hw_stats, "outCongestionDiscardPkts_", 0
                    )
                    or 0,
                    "inCongestionDiscards": getattr(
                        hw_stats, "inCongestionDiscards_", 0
                    )
                    or 0,
                    "inPausePackets": getattr(hw_stats, "inPause_", 0) or 0,
                    "outPausePackets": getattr(hw_stats, "outPause_", 0) or 0,
                }
            cable_length = getattr(port_info, "cableLengthMeters", "--") or "--"

        role = ""
        role_src = lldp_port_desc or desc
        m = re.search(r"F=(\w+)", role_src)
        if m:
            role = m.group(1)

        fec = "--"
        if profile:
            if "RS544" in profile:
                fec = "RS544"
            elif "RS528" in profile:
                fec = "RS528"
            elif "RS" in profile:
                fec = "RS"

        tcvr = tcvrs_by_port.get(local_port)
        media_int = "UNKNOWN"
        vendor = ""
        serial = ""
        part_number = ""
        app_fw = ""
        dsp_fw = ""
        temperature = 0
        voltage = 0
        tx_power = []
        rx_power = []
        rx_snr = []
        current_ma = []
        temp_flags = {}
        vcc_flags = {}

        if tcvr is not None:
            tcvr_state = getattr(tcvr, "tcvrState", None)
            tcvr_stats = getattr(tcvr, "tcvrStats", None)

            if tcvr_state:
                is_present = getattr(tcvr_state, "present", False)
                if not is_present:
                    media_int = "ABSENT"
                else:
                    media_interface = getattr(tcvr_state, "moduleMediaInterface", None)
                    if media_interface is not None:
                        media_int = MEDIA_INTERFACE_MAP.get(
                            int(media_interface), "UNKNOWN"
                        )
                    else:
                        media_int = "UNKNOWN"

                vendor_info = getattr(tcvr_state, "vendor", None)
                if vendor_info:
                    vendor = (getattr(vendor_info, "name", "") or "").strip()
                    serial = getattr(vendor_info, "serialNumber", "") or ""
                    part_number = getattr(vendor_info, "partNumber", "") or ""

                cable_info = getattr(tcvr_state, "cable", None)
                if cable_info:
                    cable_length = getattr(cable_info, "length", "--") or "--"

            if tcvr_stats:
                sensor = getattr(tcvr_stats, "sensor", None)
                if sensor:
                    temp_sensor = getattr(sensor, "temp", None)
                    vcc_sensor = getattr(sensor, "vcc", None)
                    if temp_sensor:
                        temperature = getattr(temp_sensor, "value", 0) or 0
                        flags = getattr(temp_sensor, "flags", None)
                        if flags:
                            alarm = getattr(flags, "alarm", None)
                            temp_flags = {
                                "alarm_high": getattr(alarm, "high", False)
                                if alarm
                                else False,
                                "alarm_low": getattr(alarm, "low", False)
                                if alarm
                                else False,
                            }
                    if vcc_sensor:
                        voltage = getattr(vcc_sensor, "value", 0) or 0
                        vcc_flags_obj = getattr(vcc_sensor, "flags", None)
                        if vcc_flags_obj:
                            vcc_alarm = getattr(vcc_flags_obj, "alarm", None)
                            vcc_flags = {
                                "alarm_high": getattr(vcc_alarm, "high", False)
                                if vcc_alarm
                                else False,
                                "alarm_low": getattr(vcc_alarm, "low", False)
                                if vcc_alarm
                                else False,
                            }

                channels = getattr(tcvr_stats, "channels", []) or []
                for ch in channels:
                    ch_sensors = getattr(ch, "sensors", None)
                    if ch_sensors:
                        tx_pwr = getattr(ch_sensors, "txPwr", None)
                        rx_pwr = getattr(ch_sensors, "rxPwr", None)
                        rx_snr_val = getattr(ch_sensors, "rxSnr", None)
                        tx_bias = getattr(ch_sensors, "txBias", None)
                        if tx_pwr:
                            tx_power.append(getattr(tx_pwr, "value", 0) or 0)
                        if rx_pwr:
                            rx_power.append(getattr(rx_pwr, "value", 0) or 0)
                        if rx_snr_val:
                            rx_snr.append(getattr(rx_snr_val, "value", 0) or 0)
                        if tx_bias:
                            current_ma.append(getattr(tx_bias, "value", 0) or 0)

            if tcvr_state:
                module_status = getattr(tcvr_state, "status", None)
                if module_status:
                    fw_status = getattr(module_status, "fwStatus", None)
                    if fw_status:
                        app_fw = getattr(fw_status, "version", "") or ""
                        dsp_fw = getattr(fw_status, "dspFwVer", "") or ""

        links.append(
            {
                "localPort": local_port,
                "status": status,
                "speed": speed,
                "fec": fec,
                "peer": peer,
                "peerPort": peer_port,
                "expectedPeer": expected,
                "role": role,
                "mediaInterface": media_int,
                "vendor": vendor,
                "serial": serial,
                "partNumber": part_number,
                "appFw": app_fw,
                "dspFw": dsp_fw,
                "temperature": temperature,
                "voltage": voltage,
                "txPower": tx_power,
                "rxPower": rx_power,
                "rxSnr": rx_snr,
                "currentMA": current_ma,
                "tempFlags": temp_flags,
                "vccFlags": vcc_flags,
                "ingressBytes": hw.get("ingressBytes", 0),
                "egressBytes": hw.get("egressBytes", 0),
                "inUnicastPkts": hw.get("inUnicastPkts", 0),
                "inErrors": hw.get("inErrorPkts", 0),
                "inDiscards": hw.get("inDiscardPkts", 0),
                "outDiscards": hw.get("outDiscardPkts", 0),
                "outCongestionDiscards": hw.get("outCongestionDiscardPkts", 0),
                "inCongestionDiscards": hw.get("inCongestionDiscards", 0),
                "inPause": hw.get("inPausePackets", 0),
                "outPause": hw.get("outPausePackets", 0),
                "queueOutBytes": {},
                "cableLength": cable_length,
            }
        )

    links.sort(
        key=lambda x: [int(p) for p in re.findall(r"\d+", x["localPort"])] or [0]
    )
    return {"hostname": hostname, "links": links}


def query_switch(hostname: str) -> dict:
    """Synchronous wrapper for the async query."""
    return asyncio.run(_query_switch_async(hostname))


# ── ODS Query ────────────────────────────────────────────────────────────────

ODS_CHARTS = {
    "fec_correctable": "fboss.agent.{port}.fec_correctable_errors.sum.60",
    "fec_uncorrectable": "fboss.agent.{port}.fec_uncorrectable_errors.sum.60",
    "tx_los": "qsfp_service.qsfp.interface.{port}.txLos",
    "rx_los": "qsfp_service.qsfp.interface.{port}.rxLos",
}

# Per-channel charts: query channels 0-7 (4-lane and 8-lane optics)
ODS_MULTI_CHANNEL_CHARTS = {
    "tx_snr": "qsfp_service.qsfp.interface.{port}.txSnr.channel{ch}",
    "rx_snr": "qsfp_service.qsfp.interface.{port}.rxSnr.channel{ch}",
    "tx_power": "qsfp_service.qsfp.interface.{port}.txPwr.uW.channel{ch}",
    "rx_power": "qsfp_service.qsfp.interface.{port}.rxPwr.uW.channel{ch}",
}
