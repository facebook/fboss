# On the container used for building FBOSS:

# Clean any existing packages
rm -rf /var/FBOSS/tmp_bld_dir/fboss_bins*

# Navigate to the FBOSS repository
cd /var/FBOSS/fboss

# Creates a package directory with prefix /var/FBOSS/tmp_bld_dir/fboss_bins
./fboss/oss/scripts/package-fboss.py --copy-root-libs --scratch-path /var/FBOSS/tmp_bld_dir/

# or

# Creates a tarball called "fboss_bins.tar.zst" under /var/FBOSS/tmp_bld_dir/
./fboss/oss/scripts/package-fboss.py --copy-root-libs --scratch-path /var/FBOSS/tmp_bld_dir/ --compress
