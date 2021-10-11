import typing as t
from datetime import datetime, timedelta

import fboss.lib.link_snapshots.snapshot_lib as snapshot_lib
from fboss.lib.link_snapshots.snapshot_lib import SNAPSHOT_FORMAT
from later.unittest.mock import AsyncContextManager, AsyncMock, patch
from libfb.py.asyncio.unittest import TestCase
from neteng.fboss.phy.phy.types import (
    LinkSnapshot,
    PhyInfo,
    DataPlanePhyChip,
    DataPlanePhyChipType,
    PhySideInfo,
    Side,
)
from neteng.fboss.transceiver.types import TransceiverInfo
from RockfortExpress import RockfortExpress as rfe
from thrift.py3 import serialize, Protocol

# Unfortunately, it doesn't seem like we'd really be able to test out the SQL
# filtering logic here, as that would need to be processed by an actual MySQL DB
# but we can at least unit test some of the other method logic

HOSTNAME = "test_host"
TCVR_ID = 1
PORT_ID = 2
PORT_NAME = "eth2/1/1"


def build_iphy_snapshot(timestamp: int, port_name: str) -> LinkSnapshot:
    chip = DataPlanePhyChip(name="PHY", type=DataPlanePhyChipType.IPHY, physicalID=0)
    line = PhySideInfo(side=Side.LINE)
    phy_info = PhyInfo(phyChip=chip, name=port_name, line=line, timeCollected=timestamp)
    return LinkSnapshot(phyInfo=phy_info)


def build_xphy_snapshot(timestamp: int, port_name: str) -> LinkSnapshot:
    chip = DataPlanePhyChip(name="PHY", type=DataPlanePhyChipType.IPHY, physicalID=0)
    line = PhySideInfo(side=Side.LINE)
    system = PhySideInfo(side=Side.SYSTEM)
    phy_info = PhyInfo(
        phyChip=chip, name=port_name, line=line, timeCollected=timestamp, system=system
    )
    return LinkSnapshot(phyInfo=phy_info)


def build_transceiver_snapshot(timestamp: int, tcvr_id: int) -> LinkSnapshot:
    tcvr_info = TransceiverInfo(port=tcvr_id, timeCollected=timestamp)
    return LinkSnapshot(transceiverInfo=tcvr_info)


class SnapshotLibTest(TestCase):
    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_recent_snapshots(self, mock_get_rfe_client):
        NUM_SNAPSHOTS = 10
        mock_snapshots = []
        for i in range(NUM_SNAPSHOTS):
            iphy_snapshot = build_iphy_snapshot(i, PORT_NAME)
            xphy_snapshot = build_xphy_snapshot(i, PORT_NAME)
            tcvr_snapshot = build_transceiver_snapshot(i, TCVR_ID)

            mock_snapshots.append(iphy_snapshot)
            mock_snapshots.append(xphy_snapshot)
            mock_snapshots.append(tcvr_snapshot)

        rfe_client = AsyncMock()
        mock_get_rfe_client.return_value = AsyncContextManager(
            return_value=rfe_client, instance=True
        )

        sql_result = rfe.SQLQueryResult(
            value=[
                [
                    SNAPSHOT_FORMAT.format(
                        "",
                        "eth2/1/1",
                        serialize(snapshot, protocol=Protocol.JSON).decode(),
                    )
                ]
                for snapshot in mock_snapshots
            ]
        )
        rfe_client.querySQL.return_value = sql_result
        client = await snapshot_lib.get_client()
        # We're using fetch_recent_snapshots for simplicity. We dont go to an
        # actual SQL engine so we never check the timestamps in these tests
        snapshot_collection = await client.fetch_recent_snapshots(HOSTNAME, PORT_NAME)
        iphy_snapshots, xphy_snapshots, tcvr_snapshots = snapshot_collection.unpack()

        def check_snapshots(snapshots: t.Mapping[int, t.Any]):
            for i in range(NUM_SNAPSHOTS):
                self.assertTrue(i in snapshots)
                snapshot = snapshots[i]
                self.assertTrue(snapshot is PhyInfo or snapshot is TransceiverInfo)
                self.assertEqual(snapshot.timeCollected, i)

        self.assertEqual(len(iphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(xphy_snapshots), NUM_SNAPSHOTS)
        self.assertEqual(len(tcvr_snapshots), NUM_SNAPSHOTS)

    # Just verify that we can call these functions without exceptions.
    # (The only functional differences between these can't really be tested
    # without calling out to a real sql engine)
    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_snapshots_around_time(self, mock_get_rfe_client):
        rfe_client = AsyncMock()
        mock_get_rfe_client.return_value = AsyncContextManager(
            return_value=rfe_client, instance=True
        )

        sql_result = rfe.SQLQueryResult(value=[])
        rfe_client.querySQL.return_value = sql_result
        client = await snapshot_lib.get_client()

        # test default param
        await client.fetch_snapshots_around_time(HOSTNAME, PORT_NAME, datetime.now())

        # test non-default param
        await client.fetch_snapshots_around_time(
            HOSTNAME, PORT_NAME, datetime.now(), timedelta(seconds=1)
        )

    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    async def test_fetch_snapshots_in_timespan(self, mock_get_rfe_client):
        rfe_client = AsyncMock()
        mock_get_rfe_client.return_value = AsyncContextManager(
            return_value=rfe_client, instance=True
        )

        sql_result = rfe.SQLQueryResult(value=[])
        rfe_client.querySQL.return_value = sql_result
        client = await snapshot_lib.get_client()

        await client.fetch_snapshots_in_timespan(
            HOSTNAME, PORT_NAME, datetime.now(), datetime.now()
        )
