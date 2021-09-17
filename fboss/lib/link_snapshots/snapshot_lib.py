#!/usr/bin/env python3
# Copyright 2004-present Facebook. All Rights Reserved.

from __future__ import annotations

import re
import typing as t

import MySQLdb
from libfb.py.employee import get_current_unix_user_fbid
from neteng.fboss.phy.phy.types import LinkSnapshot
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

SNAPSHOT_FORMAT = r"{}FBOSS_EVENT\(LINK_SNAPSHOT\): Collected snapshot for ports  <port:{}>  <linkSnapshot:{}>"
SNAPSHOT_REGEX = SNAPSHOT_FORMAT.format(r".*", r"eth\d+/\d+/\d+", r"(.*)")


def escape_sql(sql: str) -> str:
    return MySQLdb.escape_string(sql.encode()).decode()


class SnapshotClient:
    def __init__(self, user):
        self._user = user

    async def fetch_snapshots(
        self, hostname: str, port_name: str
    ) -> t.Mapping[int, LinkSnapshot]:
        qc = QueryCommon(
            source="snapshot_manager",
            user_id=self._user,
            instance="network_event_log",
        )
        sql = SQL.format(escape_sql(hostname), escape_sql(port_name))
        async with get_rfe_client() as client:
            scubaEntries = await client.querySQL(qc, sql)

        result: t.Dict[int, LinkSnapshot] = {}
        for row in scubaEntries.value:
            match = re.fullmatch(SNAPSHOT_REGEX, row[0])
            if match is None:
                raise Exception("link snapshot has invalid format")
            else:
                snapshotStr = match.group(1)
            snapshot = deserialize(
                LinkSnapshot, snapshotStr.encode(), protocol=Protocol.JSON
            )
            if snapshot.type is LinkSnapshot.Type.phyInfo:
                ts = snapshot.phyInfo.timeCollected
            elif snapshot.type is LinkSnapshot.Type.transceiverInfo:
                ts = snapshot.transceiverInfo.timeCollected
            else:
                raise Exception("Invalid type for link snapshot: ", snapshot.type)
            if ts is None:
                raise Exception("No timeCollected timestamp found for snapshot")
            result[ts] = snapshot
        return result


# TODO(ccpowers): We might need to support a different FBID for the service
# once it's called from the NGT workflow like in D30830986
# We can see if this is necessary once we start filling in the decision tree
async def get_client() -> SnapshotClient:
    return SnapshotClient(get_current_unix_user_fbid())
