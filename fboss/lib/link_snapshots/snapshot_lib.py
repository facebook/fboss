#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

from __future__ import annotations

import re
import typing as t

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
"""

SNAPSHOT_FORMAT = r"{}LINK_SNAPSHOT_EVENT(LINK_SNAPSHOT): Collected snapshot for ports  <port:{}>  <linkSnapshot:{}>"
SNAPSHOT_REGEX = (
    SNAPSHOT_FORMAT.replace("(", r"\(")
    .replace(")", r"\)")
    .format(r".*", r"eth\d+/\d+/\d+", r"(.*)")
)


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


class SnapshotClient:
    def __init__(self, user):
        self._user = user

    async def fetch_snapshots(
        self, hostname: str, port_name: str
    ) -> SnapshotCollection:
        qc = QueryCommon(
            source="snapshot_manager",
            user_id=self._user,
            instance="network_event_log",
        )
        sql = SQL.format(escape_sql(hostname), escape_sql(port_name))
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
