import fboss.lib.link_snapshots.snapshot_lib as snapshot_lib
from fboss.lib.link_snapshots.snapshot_lib import SNAPSHOT_FORMAT
from later.unittest.mock import AsyncContextManager, AsyncMock, patch
from libfb.py.asyncio.unittest import async_test
from libfb.py.testutil import BaseFacebookTestCase
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


class SnapshotLibTest(BaseFacebookTestCase):
    def setUp(self):
        pass

    def build_iphy_snapshot(
        self, timestamp: int, hostname: int, port_id: int, port_name: str
    ) -> LinkSnapshot:
        chip = DataPlanePhyChip(
            name="PHY", type=DataPlanePhyChipType.IPHY, physicalID=0
        )
        line = PhySideInfo(side=Side.LINE)
        phy_info = PhyInfo(
            phyChip=chip, name=port_name, line=line, timeCollected=timestamp
        )
        return LinkSnapshot(phyInfo=phy_info)

    def build_transceiver_snapshot(
        self, timestamp: int, hostname: str, tcvr_id: int
    ) -> LinkSnapshot:
        tcvr_info = TransceiverInfo(port=tcvr_id)
        return LinkSnapshot(transceiverInfo=tcvr_info)

    @patch("fboss.lib.link_snapshots.snapshot_lib.get_rfe_client")
    @async_test
    async def fetch_snapshots(self, mock_get_rfe_client):
        """
        sql query resulting in empty result
        """

        NUM_SNAPSHOTS = 10
        HOSTNAME = "test_host"
        TCVR_ID = 0xAAAAAAAA
        PORT_ID = 0xBBBBBBBB

        mock_snapshots = []
        for i in range(NUM_SNAPSHOTS):
            tcvr_snapshot = self.build_transceiver_snapshot(i, HOSTNAME, TCVR_ID)
            iphy_snapshot = self.build_iphy_snapshot(i, HOSTNAME, PORT_ID)

            mock_snapshots.append(tcvr_snapshot)
            mock_snapshots.append(iphy_snapshot)

        rfe_client = AsyncMock()
        mock_get_rfe_client.return_value = AsyncContextManager(
            return_value=rfe_client, instance=True
        )

        sql_result = rfe.SQLQueryResult(
            value=[
                [
                    SNAPSHOT_FORMAT.format(
                        "",
                        serialize(snapshot, protocol=Protocol.JSON).decode(),
                        "eth2/1/1",
                    )
                ]
                for snapshot in mock_snapshots
            ]
        )
        rfe_client.querySQL.return_value = sql_result
        client = await snapshot_lib.get_client()
        snapshots = await client.fetch_transceiver_snapshots(HOSTNAME, TCVR_ID)

        self.assertEqual(len(snapshots), NUM_SNAPSHOTS)
        num_tcvr_snapshots = 0
        num_iphy_snapshots = 0
        for i in range(NUM_SNAPSHOTS):
            self.assertTrue(i in snapshots)
            snapshot = snapshots[i]
            if snapshot.type is LinkSnapshot.Type.transceiverInfo:
                self.assertEqual(snapshot.transceiverInfo.timeCollected, i)
                num_tcvr_snapshots += 1
            elif snapshot.type is LinkSnapshot.Type.phyInfo:
                self.assertEqual(snapshot.phyInfo.timeCollected, i)
                num_iphy_snapshots += 1
            else:
                raise Exception("Invalid type for snapshot: ", snapshot.type)
        self.assertEqual(num_tcvr_snapshots, NUM_SNAPSHOTS)
        self.assertEqual(num_iphy_snapshots, NUM_SNAPSHOTS)
