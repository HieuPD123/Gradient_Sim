<<<<<<< HEAD
# Gradient_Sim
=======
# Routing Protocol Comparison Simulator

This project compares 4 routing protocols on IEEE 802.15.4 (LR-WPAN) networks using NS-3 simulator:
- **Gradient** (custom implementation)
- **Flooding** (no routing)
- **DSDV** (destination-sequenced distance vector)
- **AODV** (ad-hoc on-demand distance vector)

## Metrics

PDR (Packet Delivery Ratio), Control Packet Overhead, Round Trip Time
Tests on grid topologies: 5x5, 6x6, 7x7 nodes

## Files

- `gradient.h` / `gradient.cc` - Gradient routing protocol implementation
- `routing_simulator.h` - Base simulator class
- `routing_sim.cc` - Main simulation with protocol implementations
- `wscript` - NS-3 build script
- `routing_comparison.csv` - Output results (generated after run)

## Prerequisites

- NS-3.36 or newer with DSDV and AODV modules
- Linux (Ubuntu 20.04+ recommended)
- g++ compiler

Install NS-3:
```bash
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
./ns3 configure --enable-examples
./ns3 build
```

## Build Instructions

1. Copy all files to NS-3 scratch directory:
```bash
cp gradient.h gradient.cc routing_simulator.h routing_sim.cc wscript ~/ns-3-dev/scratch/
```

2. Build:
```bash
cd ~/ns-3-dev
./ns3 build scratch/routing-sim
```

## Run Simulation

```bash
cd ~/ns-3-dev
./ns3 run scratch/routing-sim
```

The script will run simulations for:
- Grid 5x5 (25 nodes)
- Grid 6x6 (36 nodes)  
- Grid 7x7 (49 nodes)

Each with all 4 protocols.

## Output

Results are saved to `routing_comparison.csv` with columns:
```
Protocol, GridSize, Duration, Flows, PacketsSent, PacketsRcvd, 
ControlPackets, PDR, AvgRTT_ms, AvgHopCount, AvgEnergy
```

### View Results

Open in Excel, LibreOffice, or plot with Gnuplot:

```bash
gnuplot
gnuplot> set datafile separator ","
gnuplot> set title "PDR Comparison"
gnuplot> plot "routing_comparison.csv" using 2:8 with linespoints title "PDR"
```

## Simulation Parameters

- Simulation time: 30 seconds
- Traffic: Echo between corner nodes (node 0 → node N-1)
- Packet size: 128 bytes
- Interval: 1 second
- Grid spacing: 50 meters
- Channel: IEEE 802.15.4 PHY (2.4 GHz)

## Customization

Edit values in `routing_sim.cc` main():
```cpp
std::vector<uint32_t> grid_sizes = {5, 6, 7};
double sim_time = 30.0;
```

## Notes

- Gradient routing requires GradientApp setup on each node
- Flooding uses pure broadcasting (no routing protocol overhead)
- DSDV/AODV use NS-3's built-in implementations
- Control packet count is estimated based on protocol characteristics

## License

Educational use - based on NS-3 examples

## Contact

Research project - gradient-based routing comparison
>>>>>>> 6880199 (first commit)
