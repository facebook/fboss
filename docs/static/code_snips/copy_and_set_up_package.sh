# From the host that the container is running on, use the appropriate command to
# copy the FBOSS package directory or tarball to the switch:

# Copy the directory
scp -r /opt/app/FBOSS_DIR/tmp_bld_dir/fboss_bins-$PKG_ID root@$SWITCHNAME:/opt/

# Copy the tarball
scp /opt/app/FBOSS_DIR/tmp_bld_dir/fboss_bins.tar.zst root@$SWITCHNAME:/opt/



# On the switch, use the appropriate command to set up the package:

# If using directory
cd /opt
ln -s /opt/fboss_bins-$PKG_ID /opt/fboss

# If using tarball
cd /opt
mkdir fboss && mv fboss_bins.tar.zst fboss/
cd fboss && tar -xvf fboss_bins.tar.zst
