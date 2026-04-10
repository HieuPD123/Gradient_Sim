# Quick Start Guide

## Linux Setup (Recommended)

### 1. Install NS-3 (if not already installed)

```bash
git clone https://gitlab.com/nsnam/ns-3-dev.git
cd ns-3-dev
./ns3 configure --enable-examples
./ns3 build
cd ..
```

### 2. Prepare simulation files

```bash
# Copy all files to your workspace
mkdir -p ~/gradient-sim
cd ~/gradient-sim
# Copy gradient.h, gradient.cc, routing_simulator.h, routing_sim.cc, 
# build.sh, run.sh here
```

### 3. Build

```bash
chmod +x build.sh
./build.sh ~/ns-3-dev
```

If NS-3 is in different location:
```bash
./build.sh /path/to/ns-3-dev
```

### 4. Run

```bash
chmod +x run.sh
./run.sh ~/ns-3-dev .
```

Output: `routing_comparison.csv`

## What the simulation does

1. Creates 3 grid topologies:
   - 5x5 = 25 nodes (100m x 100m area)
   - 6x6 = 36 nodes (250m x 250m area)
   - 7x7 = 49 nodes (300m x 300m area)

2. For each grid, tests 4 protocols:
   - **Gradient** - custom distance-vector with gradient
   - **Flooding** - pure broadcast (baseline)
   - **DSDV** - destination-sequenced distance vector
   - **AODV** - ad-hoc on-demand distance vector

3. All traffic goes from node 0 → node (N-1)
   - 100 packets sent
   - Packet size: 128 bytes
   - Interval: 1 second
   - Simulation: 30 seconds per protocol

4. Measures:
   - **PDR** - Packet Delivery Ratio (%)
   - **Overhead** - Control packets sent
   - **RTT** - Round trip time (ms)

## Results

CSV format: `Protocol, GridSize, Duration, ..., PDR, ..., ControlPackets`

### Example analysis

View PDR results:
```bash
grep Gradient routing_comparison.csv
grep Flooding routing_comparison.csv
grep DSDV routing_comparison.csv
grep AODV routing_comparison.csv
```

Plot with Excel/Gnuplot:
```
X-axis: Grid size (5, 6, 7)
Y-axis: PDR or Overhead
Lines: One for each protocol
```

## Troubleshooting

### Build fails: "module not found"
- Make sure DSDV and AODV are enabled in NS-3
- Run: `cd ~/ns-3-dev && ./ns3 configure --enable-modules=dsdv,aodv`

### No output file generated
- Check simulation run completed: look for "Simulation Complete" message
- Output file should be: `ns-3-dev/routing_comparison.csv`
- Copy it to your working directory

### Simulation takes too long
- Reduce grid sizes in `routing_sim.cc` main()
- Reduce simulation time (`sim_time = 15.0` instead of 30.0)
- Reduce packet count: `UintegerValue(50)` instead of 100

## Modifying the simulation

Edit `routing_sim.cc`:

```cpp
// Change grid sizes (line ~250):
std::vector<uint32_t> grid_sizes = {4, 5, 6};

// Change simulation time (line ~251):
double sim_time = 20.0;  // 20 seconds instead of 30

// Change traffic pattern (line SetupApplications):
echo_client.SetAttribute("MaxPackets", UintegerValue(50));
echo_client.SetAttribute("Interval", TimeValue(Seconds(2.0)));
```

Recompile after changes:
```bash
./ns3 build scratch/gradient-routing-sim
```

## File descriptions

| File | Purpose |
|------|---------|
| `gradient.h/cc` | Gradient routing protocol implementation |
| `routing_simulator.h` | Base class for all simulators |
| `routing_sim.cc` | Main program + 4 simulator classes |
| `wscript` | NS-3 build configuration |
| `build.sh` | Automated build script |
| `run.sh` | Automated run script |
| `routing_comparison.csv` | Results output |

## Next steps

1. Run simulation: `./run.sh ~/ns-3-dev`
2. Open CSV in Excel or LibreOffice Calc
3. Create charts comparing PDR/Overhead/RTT across protocols
4. Analyze which protocol performs best for different grid sizes
5. Consider modifying topology, traffic patterns, or protocol parameters

Happy simulating! 🚀
