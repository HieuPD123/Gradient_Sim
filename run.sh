#!/bin/bash

# Run script for Gradient Routing Comparison Simulator
# Usage: ./run.sh [ns3-install-path] [output-dir]

NS3_PATH="${1:-$HOME/ns-3-dev}"
OUTPUT_DIR="${2:-.}"

if [ ! -d "$NS3_PATH" ]; then
    echo "Error: NS-3 not found at $NS3_PATH"
    echo "Usage: ./run.sh [ns3-install-path] [output-dir]"
    exit 1
fi

echo "========== Routing Simulator Execution =========="
echo "NS-3 Path: $NS3_PATH"
echo "Output Directory: $OUTPUT_DIR"
echo ""

cd "$NS3_PATH"

echo "[*] Starting simulation..."
echo "[*] Testing 4 routing protocols (Gradient, Flooding, DSDV, AODV)"
echo "[*] Grid sizes: 5x5, 6x6, 7x7 nodes"
echo "[*] Simulation time: 30 seconds each"
echo "[*] This may take several minutes..."
echo ""

./ns3 run scratch/gradient-routing-sim

if [ $? -eq 0 ]; then
    echo ""
    echo "[*] Simulation completed successfully!"
    echo "[*] Results saved to: routing_comparison.csv"
    echo ""
    
    # Try to copy results to output directory if different
    if [ "$OUTPUT_DIR" != "." ] && [ ! "$OUTPUT_DIR" == "" ]; then
        cp routing_comparison.csv "$OUTPUT_DIR/" 2>/dev/null
        echo "[*] Results copied to: $OUTPUT_DIR/routing_comparison.csv"
    fi
    
    echo "[*] You can now analyze the results:"
    echo "    - Open routing_comparison.csv in Excel/LibreOffice"
    echo "    - Or use Gnuplot for visualization"
    echo ""
else
    echo "[!] Simulation failed!"
    exit 1
fi
