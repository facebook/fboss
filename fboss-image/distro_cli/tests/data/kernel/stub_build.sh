#!/bin/bash
# Stub kernel build script for testing
# Creates a minimal kernel artifact tarball for testing purposes
set -e

# Parse arguments
OUTPUT_DIR="/output"

echo "Stub kernel build - creating test artifact"

# Create a minimal kernel RPM structure
mkdir -p "$OUTPUT_DIR"
cd "$OUTPUT_DIR"

# Create a dummy kernel RPM file
echo "dummy kernel rpm" >kernel-test.rpm

tar -cf kernel-test.rpms.tar kernel-test.rpm
echo "Stub kernel artifact created: $OUTPUT_DIR/kernel-test.rpms.tar"

# Clean up
rm kernel-test.rpm
