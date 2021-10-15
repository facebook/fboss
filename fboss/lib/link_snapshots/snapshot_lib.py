#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

from __future__ import annotations

import enum
import re
import typing as t
from datetime import datetime, timedelta
from getpass import getuser

import MySQLdb
import paramiko
from libfb.py.employee import get_current_unix_user_fbid
from neteng.fboss.phy.phy.types import PhyInfo, LinkSnapshot
from neteng.fboss.transceiver.types import TransceiverInfo
from rfe.client_py3 import get_client as get_rfe_client
from rfe.RockfortExpress.types import QueryCommon
from thrift.py3 import deserialize, Protocol

T = t.TypeVar("T")

SQL = """
SELECT
    `msg`
FROM `network_event_log`
WHERE
    ((`device_name`) IN ('{}'))
    AND ((`originator`) IN ('wedge_agent', 'qsfp_service'))
    AND (CONTAINS(`msg`, ARRAY('linkSnapshot')))
    AND (CONTAINS(`msg`, ARRAY('<port:{}>')))
    AND `timestamp` BETWEEN {} and {}
"""

SNAPSHOT_FORMAT = r"{}LINK_SNAPSHOT_EVENT(LINK_SNAPSHOT): Collected snapshot for ports  <port:{}>  <linkSnapshot:{}>"
SNAPSHOT_REGEX = (
    SNAPSHOT_FORMAT.replace("(", r"\(")
    .replace(")", r"\)")
    .format(r".*", r"eth\d+/\d+/\d+", r"(.*)")
)

DEFAULT_TIME_RANGE = timedelta(hours=1)
FETCH_SNAPSHOT_LOG_TIMEOUT_SECONDS = 45
FETCH_SNAPSHOT_LOG_COMMAND = "zgrep 'LINK_SNAPSHOT_EVENT' /var/facebook/logs/fboss/archive/network_events.log-*.gz /var/facebook/logs/network_events.log | grep {} | grep -v sshd"


class Backend(enum.Enum):
    SCUBA = enum.auto()
    SSH = enum.auto()


class SnapshotCollection:
    def __init__(
        self,
        iphy_snaps: t.Mapping[int, PhyInfo],
        xphy_snaps: t.Mapping[int, PhyInfo],
        tcvr_snaps: t.Mapping[int, TransceiverInfo],
    ):
        self._iphy_snaps: t.Mapping[int, PhyInfo] = iphy_snaps
        self._xphy_snaps: t.Mapping[int, PhyInfo] = xphy_snaps
        self._tcvr_snaps: t.Mapping[int, TransceiverInfo] = tcvr_snaps

    # unpacks (iphy, xphy, transceiver) snapshots
    def unpack(
        self,
    ) -> t.Tuple[
        t.Mapping[int, PhyInfo],
        t.Mapping[int, PhyInfo],
        t.Mapping[int, TransceiverInfo],
    ]:
        return self._iphy_snaps, self._xphy_snaps, self._tcvr_snaps

    def empty(self) -> bool:
        return not (
            len(self._iphy_snaps) or len(self._xphy_snaps) or len(self._tcvr_snaps)
        )


def escape_sql(sql: str) -> str:
    return MySQLdb.escape_string(sql.encode()).decode()


# convert Datetime object to nanosecond timestamp
def datetime_to_ns(dt: datetime) -> int:
    return int(dt.timestamp() * 1e9)


def filter_timeseries(
    mapping: t.Mapping[int, T], time_start: datetime, time_end: datetime
) -> t.Mapping[int, T]:
    return {
        ts: v
        for ts, v in mapping.items()
        if time_start.timestamp() <= ts <= time_end.timestamp()
    }


class SnapshotClient:
    def __init__(self, user):
        self._user = user

    # fetches snapshots within the past 3 hours
    async def fetch_recent_snapshots(
        self,
        hostname: str,
        port_name: str,
        backend: Backend = Backend.SSH,
    ) -> SnapshotCollection:
        return await self.fetch_snapshots_around_time(
            hostname, port_name, datetime.now(), timedelta(hours=3), backend
        )

    # Fetch the snapshots posted between (timestamp - time_delta) and (timestamp + time_delta)
    # Note that this filters based on the time that the snapshot was posted,
    #   and not the time that the snapshot was collected
    async def fetch_snapshots_around_time(
        self,
        hostname: str,
        port_name: str,
        timestamp: datetime,
        time_delta: timedelta = DEFAULT_TIME_RANGE,
        backend: Backend = Backend.SSH,
    ) -> SnapshotCollection:
        return await self.fetch_snapshots_in_timespan(
            hostname, port_name, timestamp - time_delta, timestamp + time_delta, backend
        )

    async def fetch_snapshots_in_timespan(
        self,
        hostname: str,
        port_name: str,
        time_start: datetime,
        time_end: datetime,
        backend: Backend = Backend.SSH,
    ) -> SnapshotCollection:
        if backend == Backend.SSH:
            return await self.fetch_snapshots_in_timespan_via_ssh(
                hostname, port_name, time_start, time_end
            )
        elif backend == Backend.SCUBA:
            return await self.fetch_snapshots_in_timespan_via_scuba(
                hostname, port_name, time_start, time_end
            )
        else:
            raise Exception(
                f"Invalid backend passed to fetch_snapshots_in_timespan: {str(backend)}"
            )

    # TODO(ccpowers): We can remove this once Scuba resolves their issue
    # with OOM errors (T102995533 and T102701121)
    # Most of this code is copied from tech_support.py
    async def fetch_snapshots_in_timespan_via_ssh(
        self,
        hostname: str,
        port_name: str,
        time_start: datetime,
        time_end: datetime,
    ) -> SnapshotCollection:
        keyfile = f"/var/facebook/credentials/{getuser()}/ssh/id_rsa-cert.pub"
        ssh_conn = paramiko.SSHClient()
        ssh_conn.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        ssh_conn.connect(
            hostname=hostname,
            username="netops",
            password=None,
            key_filename=keyfile,
        )
        cmd = FETCH_SNAPSHOT_LOG_COMMAND.format(port_name)
        print("Running cmd:", cmd)
        stdin, stdout, stderr = ssh_conn.exec_command(
            cmd,
            timeout=FETCH_SNAPSHOT_LOG_TIMEOUT_SECONDS,
        )

        stdin.flush()
        output = stdout.read()
        stderr.read()
        exit_status = stdout.channel.recv_exit_status()
        if exit_status != 0:
            raise Exception(
                f"Fetch snapshot command exited with error code {exit_status}. Stderr: {stderr.read()}"
            )
        output = output.decode("utf-8", "replace").strip()
        collection = await self.process_snapshot_lines(output.split("\n"))

        iphy, xphy, tcvr = collection.unpack()
        iphy = filter_timeseries(iphy, time_start, time_end)
        xphy = filter_timeseries(xphy, time_start, time_end)
        tcvr = filter_timeseries(tcvr, time_start, time_end)

        return SnapshotCollection(iphy, xphy, tcvr)

    async def fetch_snapshots_in_timespan_via_scuba(
        self,
        hostname: str,
        port_name: str,
        time_start: datetime,
        time_end: datetime,
    ) -> SnapshotCollection:
        qc = QueryCommon(
            source="snapshot_manager",
            user_id=self._user,
            instance="network_event_log",
        )
        sql = SQL.format(
            escape_sql(hostname),
            escape_sql(port_name),
            datetime_to_ns(time_start),
            datetime_to_ns(time_end),
        )
        async with get_rfe_client() as client:
            scuba_entries = await client.querySQL(qc, sql)

        lines = [row[0] for row in scuba_entries.value]
        return await self.process_snapshot_lines(lines)

    async def process_snapshot_lines(self, lines: t.List[str]) -> SnapshotCollection:
        iphy_snapshots: t.Dict[int, PhyInfo] = {}
        xphy_snapshots: t.Dict[int, PhyInfo] = {}
        tcvr_snapshots: t.Dict[int, TransceiverInfo] = {}
        for line in lines:
            match = re.fullmatch(SNAPSHOT_REGEX, line)
            if match is None:
                raise Exception(
                    "link snapshot has invalid format\nSnapshot was: {}".format(line)
                )
            else:
                snapshot_str = match.group(1)

            snapshot = deserialize(
                LinkSnapshot, snapshot_str.encode(), protocol=Protocol.JSON
            )
            if snapshot.type is LinkSnapshot.Type.phyInfo:
                ts = snapshot.phyInfo.timeCollected
                if snapshot.phyInfo.system is None:
                    iphy_snapshots[ts] = snapshot.phyInfo
                else:
                    xphy_snapshots[ts] = snapshot.phyInfo
            elif snapshot.type is LinkSnapshot.Type.transceiverInfo:
                ts = snapshot.transceiverInfo.timeCollected
                if ts is None:
                    raise Exception("No timestamp found for transceiver snapshots")
                tcvr_snapshots[ts] = snapshot.transceiverInfo
            else:
                raise Exception("Invalid type for link snapshot: ", snapshot.type)
        return SnapshotCollection(iphy_snapshots, xphy_snapshots, tcvr_snapshots)


# TODO(ccpowers): We might need to support a different FBID for the service
# once it's called from the NGT workflow like in D30830986
# We can see if this is necessary once we start filling in the decision tree
async def get_client() -> SnapshotClient:
    return SnapshotClient(get_current_unix_user_fbid())
