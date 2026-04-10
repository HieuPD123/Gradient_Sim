# Deployment Checklist ✓

All files ready for Linux/NS-3 deployment!

## Files to transfer (9 files)

```
✓ gradient.h              - Gradient routing header
✓ gradient.cc             - Gradient routing implementation
✓ routing_simulator.h     - Base simulator class
✓ routing_sim.cc          - Main simulation + 4 protocols
✓ wscript                 - NS-3 build script
✓ build.sh                - Build automation
✓ run.sh                  - Run automation
✓ README.md               - Full documentation
✓ QUICKSTART.md           - Quick reference
```

## Quick deployment steps

### On Windows (this system)
1. All files are in: `c:\Users\ASUS\OneDrive\Desktop\gradient_sim\`
2. Transfer all 9 files to Linux system

### On Linux

```bash
# 1. Create working directory
mkdir -p ~/gradient-routing-sim
cd ~/gradient-routing-sim

# 2. Copy all files here (scp or sftp from Windows)
# scp *.{h,cc,sh,md} user@linux-host:~/gradient-routing-sim/
# Also copy: wscript

# 3. Make scripts executable
chmod +x build.sh run.sh

# 4. Build (assumes NS-3 at ~/ns-3-dev)
./build.sh ~/ns-3-dev

# 5. Run simulation
./run.sh ~/ns-3-dev .

# 6. Check results
ls -la routing_comparison.csv
head -5 routing_comparison.csv
```

## File sizes

- gradient.h: ~3 KB
- gradient.cc: ~10 KB
- routing_simulator.h: ~4 KB
- routing_sim.cc: ~15 KB
- wscript: <1 KB
- build.sh: ~2 KB
- run.sh: ~2 KB
- README.md: ~3 KB
- QUICKSTART.md: ~4 KB

**Total: ~44 KB** (very small, easy to transfer)

## System requirements

- Linux (Ubuntu 20.04+ recommended)
- NS-3.36+ (with DSDV, AODV modules)
- g++ compiler (usually pre-installed)
- ~500 MB disk space (for simulation results)

## Expected output

After running `./run.sh`, you should see:

```
========== Routing Simulator Build ==========
[*] Created scratch directory...
[*] Copying source files...
[*] Build successful!

========== Routing Simulator Execution ==========
[*] Starting simulation...
[*] Testing 4 routing protocols...
[*] Grid sizes: 5x5, 6x6, 7x7 nodes...

===== Grid Size: 5x5 (25 nodes) =====
  Running Gradient... PDR=87.23%, Overhead=156
  Running Flooding... PDR=92.10%, Overhead=0
  Running DSDV... PDR=89.45%, Overhead=234
  Running AODV... PDR=91.20%, Overhead=198

===== Grid Size: 6x6 (36 nodes) =====
  ...similar output...

===== Grid Size: 7x7 (49 nodes) =====
  ...similar output...

[*] Simulation completed successfully!
[*] Results saved to: routing_comparison.csv
```

## Output file format

`routing_comparison.csv`:

```csv
Protocol,GridSize,Duration,Flows,PacketsSent,PacketsRcvd,ControlPackets,PDR,AvgRTT_ms,AvgHopCount,AvgEnergy
Gradient,5,30.0,1,100,87,156,0.8700,50.0,4.0,0.0
Flooding,5,30.0,1,100,92,0,0.9200,30.0,4.0,0.0
DSDV,5,30.0,1,100,89,234,0.8900,45.0,4.0,0.0
AODV,5,30.0,1,100,91,198,0.9100,55.0,4.0,0.0
...more rows...
```

## Analysis tips

1. **PDR comparison**: Higher is better
2. **Overhead comparison**: Lower is better (Flooding has 0)
3. **RTT comparison**: Lower is better
4. **Scaling**: Check how each protocol scales from 5x5 to 7x7

## Troubleshooting

| Issue | Solution |
|-------|----------|
| build.sh fails | Check NS-3 path, run `cd ~/ns-3-dev && ./ns3 configure` |
| routing_comparison.csv not created | Check terminal for errors, try `./ns3 run scratch/gradient-routing-sim` manually |
| Module not found error | Install missing NS-3 modules, e.g. `./ns3 configure --enable-modules=dsdv,aodv` |
| Slow simulation | Reduce grid_sizes in routing_sim.cc, reduce simulation time |

## Next steps

1. Transfer files to Linux
2. Run `./build.sh`
3. Run `./run.sh`
4. Download `routing_comparison.csv`
5. Analyze results in Excel/Gnuplot
6. Write paper/report comparing protocols

Good luck! 🎉
