#!/bin/bash

# Build script for Gradient Routing Comparison Simulator
# Usage: ./build.sh [ns3-install-path]

NS3_PATH="${1:-$HOME/ns-3-dev}"

if [ ! -d "$NS3_PATH" ]; then
    echo "Error: NS-3 not found at $NS3_PATH"
    echo "Usage: ./build.sh [ns3-install-path]"
    echo "Example: ./build.sh ~/ns-3-dev"
    exit 1
fi

echo "========== Routing Simulator Build =========="
echo "NS-3 Path: $NS3_PATH"

# Copy files to NS-3 scratch directory
SCRATCH_DIR="$NS3_PATH/scratch/gradient-routing-sim"

if [ ! -d "$SCRATCH_DIR" ]; then
    mkdir -p "$SCRATCH_DIR"
    echo "[*] Created scratch directory: $SCRATCH_DIR"
fi

# Copy source files
echo "[*] Copying source files..."
cp gradient.h "$SCRATCH_DIR/"
cp gradient.cc "$SCRATCH_DIR/"
cp routing_simulator.h "$SCRATCH_DIR/"
cp routing_sim.cc "$SCRATCH_DIR/"

# Create wscript in scratch directory
cat > "$SCRATCH_DIR/wscript" << 'EOF'
# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

def build(bld):
    obj = bld.create_ns3_program(
        'routing-sim',
        ['core', 'network', 'internet', 'applications', 'mobility', 'lr-wpan', 'dsdv', 'aodv']
    )
    
    obj.source = [
        'gradient.cc',
        'routing_sim.cc',
    ]
    
    obj.includes = ['.']
EOF

echo "[*] Copied wscript"

# Build
echo "[*] Building with NS-3..."
cd "$NS3_PATH"

./ns3 configure --enable-examples --enable-tests || {
    echo "[!] NS-3 configure failed. Trying build anyway..."
}

./ns3 build scratch/gradient-routing-sim || {
    echo "[!] Build failed."
    exit 1
}

echo "[*] Build successful!"
echo ""
echo "To run simulation:"
echo "  cd $NS3_PATH"
echo "  ./ns3 run scratch/gradient-routing-sim"
echo ""
