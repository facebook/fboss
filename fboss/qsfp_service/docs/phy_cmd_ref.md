# PHY Command Reference

This webpage lists down the important PHY commands for the FBOSS platforms. The FBOSS switches have two kind of Physical Layer (PHY) devices. The switching ASIC NPU has the internal PHY which is also called IPHY. Every FBOSS platform switching ASIC has the internal phy (IPHY).Some of the FBOSS platforms have dedicated external PHY chips on board and this is called External PHY (XPHY). The platforms: Yamp, Elbert, Minipack, Fuji have the external PHY. The following section describes some of the debug commands for these PHY.


### 1. To get the PHY related information

**fboss2 show interface phy**

```
Example:
```

### 2. To get the NPU IPHY related information

**fboss2 show interface phy phyChipType asic**
```
Example:
```

### 3. To get the external PHY related info

**fboss2 show interface phy phyChipType xphy**
```
Example:
```
