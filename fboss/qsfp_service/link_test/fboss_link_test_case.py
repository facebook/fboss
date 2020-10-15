#!/usr/bin/env python3
import time
from collections import defaultdict, namedtuple
from typing import DefaultDict, Dict, NamedTuple, Union

from fboss.fb_thrift_clients import FbossAgentClient, QsfpServiceClient
from libfb.py.decorators import memoize_forever, retryable
from neteng.fboss.ctrl.ttypes import PortInfoThrift, PortOperState, PortStatus
from neteng.netcastle.teams.fboss.link_tests.link_test_case_base import LinkTestCase
from util.constants import DEFAULT_AGENT_REMOTE_PORT, DEFAULT_THRIFT_TIMEOUT


class FbosslinkTestCase(LinkTestCase):
    """
    Fboss link test base class.
    """

    def setUp(self) -> None:
        super().setUp()
        self.log = self.logger
        self.setup_wedge_agent_log()
        self.setup_qsfp_service_log()
        self.use_agent_config("100G")
        self.start_wedge_agent()
        self.start_qsfp_service()
        time.sleep(120)

        self.test_name = self._testMethodName.upper()
        self.check_port_status()

    def tearDown(self) -> None:
        self.stop_wedge_agent()
        self.stop_qsfp_service()

    @retryable(num_tries=50, sleep_time=5, debug=False)
    def check_port_status(self, up_expected: bool = True):
        """
        Method to check all the port status
        """
        port_info = self._get_all_port_info()
        self.log.info("checking port status")
        if up_expected:

            down_ports = [
                port
                for port in port_info
                if not port_info[port].operState == PortOperState.UP
                # A hack here for now due to the link topology in the lab
                # at the moment. Will remove after wire the two corner ports up.
                and port_info[port].name not in ["eth2/1/1", "eth9/16/1"]
            ]
            self.assertFalse(
                down_ports,
                msg=f" Not all ports are up " f" Ports that are down:{down_ports}",
            )
        else:
            up_ports = [
                port
                for port in port_info
                if port_info[port].operState == PortOperState.DOWN
                # A hack here for now due to the link topology in the lab
                # at the moment. Will remove after wire the two corner ports up.
                and port_info[port].name not in ["eth2/1/1", "eth9/16/1"]
            ]
            self.assertFalse(
                up_ports,
                msg=f" Not all ports are down " f" Ports that are up:{up_ports}",
            )

    @memoize_forever
    def _get_all_interface_name_to_port_id_mapping(self) -> Dict[str, NamedTuple]:
        """
        Used to fetch the interface name to the internal Port ID mapping.
        """
        Port_info_tuple = namedtuple("Port_info_tuple", ["port_id", "vlan_id"])
        intf_map: dict = {}

        with self._get_fboss_agent_client(self.hostname) as client:
            port_info_result = client.getAllPortInfo()

        if not port_info_result:
            raise Exception(
                f"Empty Port Info result returned by getAllPortInfo() "
                f"for {self.hostname}"
            )

        for port_info in port_info_result.values():
            interface_name: str = port_info.name
            port_id: int = port_info.portId
            vlan_id: int = port_info.vlans[0] if port_info.vlans else None
            intf_map[interface_name] = Port_info_tuple(port_id=port_id, vlan_id=vlan_id)

        return intf_map

    def _get_all_transceiver_id_to_interface_name_mapping(
        self,
    ) -> Dict[str, NamedTuple]:
        """
        Used to fetch the transceiver id to interface name and portid mapping.
        """
        Transceiver_info = namedtuple("transceiver_info", ["interface_name", "port_id"])
        transceiver_map: DefaultDict = DefaultDict(NamedTuple)
        intf_map_result = self._get_all_interface_name_to_port_id_mapping()
        with self._get_fboss_agent_client(self.hostname) as client:
            port_status: Dict[int, PortStatus] = client.getPortStatus()
        for interface_name, port_info in intf_map_result.items():
            try:
                trans_id = port_status[port_info.port_id].transceiverIdx.transceiverId
                transceiver_map[trans_id] = Transceiver_info(
                    interface_name=interface_name, port_id=port_info.port_id
                )
            except Exception:
                self.logger.warning(
                    "Transciver id to interface name mapping"
                    f" failed for {interface_name} "
                )
        return transceiver_map

    def get_all_qsfp_dom_values(self) -> Dict[str, dict]:
        """
        Used to obtain the QSFP TransceiverInfo for all optics present.
        """
        all_interfaces_dom_values: DefaultDict[str, dict] = defaultdict(dict)
        trans_to_int_name_map: Dict[
            str, NamedTuple
        ] = self._get_all_transceiver_id_to_interface_name_mapping()
        with self._get_qsfp_service_client(self.hostname) as qsfp_client:
            qsfp_info = qsfp_client.getTransceiverInfo()
        if qsfp_info:
            for trans_id, trans_info in qsfp_info.items():
                intf_name = trans_to_int_name_map.get(trans_id).interface_name
                all_interfaces_dom_values[intf_name] = trans_info
        else:
            self.logger.warning(
                "Empty TransceiverInfo received from qsfp_service client"
            )
        return all_interfaces_dom_values

    def _get_formatted_build_info(
        self, fboss_client: Union[FbossAgentClient, QsfpServiceClient]
    ) -> Dict[str, str]:
        """
        Used to fetch the build info of the given client.
        """
        build_info_res: Dict[str, str] = fboss_client.getRegexExportedValues("build_.*")
        if not build_info_res:
            self.logger.warning(f"Empty build info for {self.hostname}. Please check!")
            return

        build_package_info = (
            build_info_res["build_package_info"]
            if "build_package_info" in build_info_res
            else "-"
        )

        formatted_build_info = {
            "Package Name": build_info_res["build_package_name"],
            "Package Info": build_package_info,
            "Package Version": build_info_res["build_package_version"],
            "Build details": {
                "Host": build_info_res["build_host"],
                "Time": build_info_res["build_time"],
                "User": build_info_res["build_user"],
                "Path": build_info_res["build_path"],
                "Platform": build_info_res["build_platform"],
                "Revision": build_info_res["build_revision"],
            },
        }
        return formatted_build_info

    def _get_all_port_info(self) -> Dict[int, PortInfoThrift]:
        client = self._get_fboss_agent_client(self.hostname)
        return client.getAllPortInfo()

    def _get_fboss_agent_client(self, hostname: str) -> FbossAgentClient:
        client = FbossAgentClient(
            hostname, port=DEFAULT_AGENT_REMOTE_PORT, timeout=DEFAULT_THRIFT_TIMEOUT
        )
        return client

    def _get_qsfp_service_client(self, hostname: str) -> QsfpServiceClient:
        qsfp_client = QsfpServiceClient(hostname, None, timeout=DEFAULT_THRIFT_TIMEOUT)
        return qsfp_client
