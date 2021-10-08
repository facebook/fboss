#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

from __future__ import annotations

import re
import typing as t
from datetime import datetime, timedelta

import MySQLdb
from libfb.py.employee import get_current_unix_user_fbid
from neteng.fboss.phy.phy.types import PhyInfo, LinkSnapshot
from neteng.fboss.transceiver.types import TransceiverInfo
from rfe.client_py3 import get_client as get_rfe_client
from rfe.RockfortExpress.types import QueryCommon
from thrift.py3 import deserialize, Protocol

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


class SnapshotClient:
    def __init__(self, user):
        self._user = user

    # fetches snapshots within the past 3 hours
    async def fetch_recent_snapshots(
        self, hostname: str, port_name: str
    ) -> SnapshotCollection:
        return await self.fetch_snapshots_around_time(
            hostname, port_name, datetime.now(), timedelta(hours=3)
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
    ) -> SnapshotCollection:
        return await self.fetch_snapshots_in_timespan(
            hostname, port_name, timestamp - time_delta, timestamp + time_delta
        )

    async def fetch_snapshots_in_timespan(
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

        iphy_snapshots: t.Dict[int, PhyInfo] = {}
        xphy_snapshots: t.Dict[int, PhyInfo] = {}
        tcvr_snapshots: t.Dict[int, TransceiverInfo] = {}
        for row in scuba_entries.value:
            match = re.fullmatch(SNAPSHOT_REGEX, row[0])
            if match is None:
                raise Exception("link snapshot has invalid format")
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
